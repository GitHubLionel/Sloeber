#include "SSR.h"

// Pour faire un simple test du zéro-cross : clignotement de led
//#define SIMPLE_ZC_TEST

#ifdef ESP8266
// Select a Timer Clock
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               true           // for medium time and medium accurate timer
#define USING_TIM_DIV256              false            // for longest timer but least accurate. Default

#include "Timer_utils.h"

// Le timer avec interruption qui pilote le SSR
TimerInterrupt Timer_SSR;
#define TIMERMUX_ENTER()	void()
#define TIMERMUX_EXIT()	void()
#define ZCMUX_ENTER()	void()
#define ZCMUX_EXIT()	void()
#elif ESP32
// Voir https://deepbluembedded.com/esp32-timers-timer-interrupt-tutorial-arduino-ide/
// Le numéro du timer utilisé [0..3]
#define TIMER_NUM	0
hw_timer_t *Timer_SSR = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define TIMERMUX_ENTER()	portENTER_CRITICAL_ISR(&timerMux)
#define TIMERMUX_EXIT()	portEXIT_CRITICAL_ISR(&timerMux);
portMUX_TYPE ZCMux = portMUX_INITIALIZER_UNLOCKED;
#define ZCMUX_ENTER()	portENTER_CRITICAL_ISR(&ZCMux)
#define ZCMUX_EXIT()	portEXIT_CRITICAL_ISR(&ZCMux);
// Le channel pour la led SSR en PWM
#define LED_CHANNEL	0
#endif

#define DEBUG_SSR           0          // Affichage de P_100 et SSR_COUNT

// Les handle des pins et du timer
volatile uint8_t SSR_PIN;
volatile int8_t LED_PIN = -1;

volatile uint32_t Count_CS_ZC = 0; // Un compteur des tops ZC
volatile bool Top_200ms = false;   // Un top 200 ms à utiliser avec la fonction ZC_Top200ms()
volatile bool Top_CS_ZC = false;   // Top ZC

#define HALF_PERIOD_us    10000    // Fréquence de 50 Hz => période de 20 ms, top Cirrus 10 ms
#define SSR_TURN_ON_us    50       // Temps nécessaire au SSR pour démarrer en us
#define TRY_MAX           10       // Le nombre max d'essai pour réduire le surplus

// Si le délais est supérieur à DELAY_MAX, on arrête le SSR
// Correspond à environ 2%
#define DELAY_MAX	9000	// HALF_PERIOD_us - 1000
// Si le délais est inférieur à DELAY_MIN, on met quand même un petit delais. Utile ?
#define DELAY_MIN	100

// Le SSR est allumé suivant le pourcentage de la charge qu'on veut avoir
// On limite entre P_MIN et P_MAX %
#define P_MIN      0.0
#define P_MAX    100.0

// La puissance de la charge divisée par sa tension nominale
float Dump_Power = 0.0;
float Dump_Power_Relatif = 0.0;

// Nombre de pas avant démarrage du SSR normalement entre [0 .. HALF_PERIOD_us]
// mais restreint à [DELAY_MIN .. DELAY_MAX]
volatile uint32_t SSR_COUNT = DELAY_MAX;

// Le pourcentage d'utilisation du SSR
volatile float P_100 = 0.0;

volatile bool Tim_Interrupt_Enabled = false;
volatile bool New_Timer_Parameters = false;
// Indique si le SSR est activé ou pas
volatile bool Is_SSR_enabled = false;

// L'action en cours sur le SSR
SSR_Action_typedef current_action = SSR_Action_OFF;

// La puissance cible en version dimmer
float Dimme_Power = 0.0;

// La tension et la puissance en cours fournies par le Cirrus ou autre
extern float Cirrus_voltage;
extern float Cirrus_power_signed;
extern bool CIRRUS_get_rms_data(volatile float *uRMS, volatile float *pRMS);
// Fonction de gestion du SSR en mode dimme ou surplus
volatile Gestion_SSR_TypeDef Gestion_SSR_CallBack = NULL;

// ********************************************************************************
// Private functions
// ********************************************************************************

// Actualisation des paramètres du Timer
void SSR_Update_Dimme_Timer();
void SSR_Update_Surplus_Timer();

// Gestion du Timer
void SSR_Enable_Timer_Interrupt(bool enable);
void SSR_Start_Timer(void);
void SSR_Stop_Timer(void);

// Fonction de DEBUG_SSR
extern void PrintTerminal(const char *text);
void PrintVal(const char *text, float val, bool integer)
{
	char buffer[50];
	if (integer)
		sprintf(buffer, "%s: %d\r\n", text, (uint32_t) val);
	else
		sprintf(buffer, "%s: %.3f\r\n", text, val);
	PrintTerminal(buffer);
}

// Function for DEBUG_SSR message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// ********************************************************************************
// Callback
// ********************************************************************************

// Non-speed critical bits
//#pragma GCC optimize ("Os")

// Action directe sur le pin, sans test
#define USE_DIRECT_PIN

// N'est pas disponible avec ESP32
#ifdef ESP32
#undef USE_DIRECT_PIN
#endif

#ifdef USE_DIRECT_PIN
#define SET_PIN_HIGH(pin)	GPOS = (1 << (pin))  // set pin HIGH
#define SET_PIN_LOW(pin)	GPOC = (1 << (pin))  // set pin LOW
#else
#define SET_PIN_HIGH(pin)	digitalWrite(pin, HIGH)  // set pin HIGH
#define SET_PIN_LOW(pin)	digitalWrite(pin, LOW)   // set pin LOW
#endif

// Send SSR pulse after delay
void IRAM_ATTR onTimerSSR(void)
{
	TIMERMUX_ENTER();
	if (Is_SSR_enabled)
	{
		if (Top_CS_ZC)
		{
			SET_PIN_HIGH(SSR_PIN);
			Top_CS_ZC = false;
#ifdef ESP8266
//			Timer_SSR.stopTimer();
			Timer_SSR.setInterval(SSR_TURN_ON_us);
			Timer_SSR.startTimer();
#elif ESP32
//			timerStop(Timer_SSR);
			timerAlarmWrite(Timer_SSR, SSR_TURN_ON_us, true);
			timerWrite(Timer_SSR, 0);
//			timerRestart(Timer_SSR);
#endif
		}
		else
		{
			SET_PIN_LOW(SSR_PIN);

#ifdef ESP8266
			if (LED_PIN != -1)
				SET_PIN_HIGH(LED_PIN);
#elif ESP32
			timerAlarmDisable(Timer_SSR);
//				ledcWrite(LED_CHANNEL, 128);
#endif
		}
	}
	TIMERMUX_EXIT();
}

#ifdef SIMPLE_ZC_TEST
// Toutes les secondes, on allume la led pendant 100 ms
void IRAM_ATTR onCirrusZC(void)
{
	Count_CS_ZC++;
	if (!Top_200ms)
		Top_200ms = (Count_CS_ZC % 20 == 0);

	if (Count_CS_ZC == 100) // 1 s
	{
		SET_PIN_HIGH(LED_PIN);
		Count_CS_ZC = 0;
	}
	else
		if (Count_CS_ZC == 10) // 100 ms
			SET_PIN_LOW(LED_PIN);
}
#else
// Zero cross was fired
// Note : on est obligé de redéfinir l'interval à chaque fois car il est modifié
// à chaque fois même si New_Timer_Parameters est false
void IRAM_ATTR onCirrusZC(void)
{
	ZCMUX_ENTER();
	Count_CS_ZC++;
	if (!Top_200ms)
		Top_200ms = (Count_CS_ZC % 20 == 0);

	if (Is_SSR_enabled && Tim_Interrupt_Enabled)
	{
		Top_CS_ZC = true;
#ifdef ESP8266
//		Timer_SSR.stopTimer();
		Timer_SSR.setInterval(SSR_COUNT);
		Timer_SSR.startTimer();
		// Plus le délai est long, plus la led sera éteinte longtemps
		if (LED_PIN != -1)
			SET_PIN_LOW(LED_PIN);
#elif ESP32
//		timerStop(Timer_SSR);
		timerAlarmWrite(Timer_SSR, SSR_COUNT, true);
		timerWrite(Timer_SSR, 0);
		timerAlarmEnable(Timer_SSR);
//		timerRestart(Timer_SSR);
		if (LED_PIN != -1)
			ledcWrite(LED_CHANNEL, 1023 - SSR_COUNT / 9); // (int)(255*P_100/100.0)
#endif
	}
	ZCMUX_EXIT();
}
#endif

uint32_t ZC_Get_Count(void)
{
	return Count_CS_ZC;
}

bool ZC_Top200ms(void)
{
	bool top = Top_200ms;
	Top_200ms = false;
	return top;
}

void SetLedPinLow(void)
{
	if (LED_PIN != -1)
#ifdef ESP8266
		SET_PIN_LOW(LED_PIN);
#else
		ledcWrite(LED_CHANNEL, 0);
#endif
}

// ********************************************************************************
// Initialisation
// ********************************************************************************

/**
 * SSR_Init : initialisation du SSR avec les callback zéro-cross et timer
 * ZC_Pin : le pin zéro-cross
 * SSR_Pin : le pin actionnant le SSR
 * LED_Pin : le pin de la led indiquant le status du SSR (default -1)
 * Le SSR est désactivé.
 * D'abord définir le type d'action avec SSR_Action() puis appeler SSR_Enable() pour l'activer.
 */
void SSR_Init(uint8_t ZC_Pin, uint8_t SSR_Pin, int8_t LED_Pin)
{
	// Interruption zero-cross Cirrus, callback onCirrusZC
	// Zero cross pin INPUT_PULLUP
	pinMode(ZC_Pin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(ZC_Pin), onCirrusZC, RISING);

	// Pilotage du pin du SSR
	SSR_PIN = SSR_Pin;
	pinMode(SSR_Pin, OUTPUT);
	SET_PIN_LOW(SSR_Pin);

	// Initialisation timer SSR, callback onTimerSSR
	// Au top du zéro-cross, attend SSR_COUNT us pour lancer le pulse SSR
#ifdef ESP8266
	if (Timer_SSR.setInterval(SSR_COUNT, onTimerSSR, false))
	{
		print_debug("Starting Timer_SSR_50us OK");
	}
	else
		print_debug("Error starting Timer_SSR_50us");
#elif ESP32
	Timer_SSR = timerBegin(TIMER_NUM, 80, true);  // Pour une clock de 80 MHz => tick de 1 us
	if (Timer_SSR)
	{
		timerAttachInterrupt(Timer_SSR, &onTimerSSR, true);
//		timerAlarmWrite(Timer_SSR, SSR_COUNT, true);
//		timerAlarmEnable(Timer_SSR);
		print_debug("Starting Timer_SSR_50us OK");
	}
	else
		print_debug("Error starting Timer_SSR_50us");
#endif

	// Led associé au SSR
	LED_PIN = LED_Pin;
	if (LED_PIN != -1)
	{
		pinMode(LED_PIN, OUTPUT);
		SET_PIN_LOW(LED_PIN);
#ifndef SIMPLE_ZC_TEST
#ifdef ESP32
		// Configure le channel
		ledcSetup(LED_CHANNEL, 1000, 10);  // 10 bits = 1024

		// Attache le channel sur le pin led
		ledcAttachPin(LED_PIN, LED_CHANNEL);
#endif
#endif
	}

	// Charge par défaut
	// La puissance relative à sa tension nominale = courant !
	Dump_Power = 1000.0;
	Dump_Power_Relatif = Dump_Power / 220.0; // Normalement c'est 230.0
}

/**
 * Fonction essayant de déterminer la puissance de la charge.
 * Dans le principe, on détermine la tension et la puissance en cours,
 * puis on démarre la charge à fond. Par différence, on déduit la puissance de la charge.
 * Doit être exécutée avant de lancer la régulation.
 * Si on connait la charge, on peut directement utiliser SSR_Set_Dump_Power
 */
float SSR_Compute_Dump_power(float default_Power)
{
#define count_max   20
	uint8_t count = count_max;
	float urms, prms;
	float cumul_p = 0.0;
	float cumul_u = 0.0;
	float initial_p, initial_u, final_p, final_u, dump;

	// On récupère la puissance et la tension initiale sur 3 s
	CIRRUS_get_rms_data(&urms, &prms);
	cumul_p = prms;
	cumul_u = urms;
	while ((count--) != 0)
	{
		delay(150);
		CIRRUS_get_rms_data(&urms, &prms);
		cumul_p += prms;
		cumul_u += urms;
	}

	initial_p = cumul_p / count_max;
	initial_u = cumul_u / count_max;
	PrintVal("Puissance sans charge", initial_p, false);

	// On allume le SSR et on refait une mesure
	SSR_Action(SSR_Action_FULL);
	SSR_Enable();
	delay(2000); // Pour stabiliser

	CIRRUS_get_rms_data(&urms, &prms);
	cumul_p = prms;
	cumul_u = urms;
	count = count_max;
	while ((count--) != 0)
	{
		delay(150);
		CIRRUS_get_rms_data(&urms, &prms);
		cumul_p += prms;
		cumul_u += urms;
	}

	final_p = cumul_p / count_max;
	final_u = cumul_u / count_max;

	// On éteint le SSR
	SSR_Action(SSR_Action_OFF);
	delay(2000); // Pour stabiliser

	dump = (final_p - initial_p) / ((initial_u + final_u) / 2.0);
	// La charge ne devait pas être branchée, on utilise la puissance par défaut
	if (dump < 0.5)
		Dump_Power_Relatif = default_Power / 230.0;
	PrintVal("Puissance de la charge relative", Dump_Power_Relatif, false);
	return Dump_Power_Relatif;
}

/**
 * Défini l'action du SSR :
 * 	- SSR_Action_OFF: éteint le SSR
 * 	- SSR_Action_FULL: SSR à 100%
 * 	- SSR_Action_Percent: SSR à un certain pourcentage quelque soit la puissance de la charge
 * 	- SSR_Action_Surplus: SSR afin d'avoir zéro surplus
 * 	- SSR_Action_Dimme: SSR de façon à avoir seulement un pourcentage de la charge
 * Arrête le SSR s'il est actif.
 * Appeler SSR_Enable() pour (re)démarrer le SSR ou mettre restart à true
 *
 */
void SSR_Action(SSR_Action_typedef do_action, bool restart)
{
	float pourcent;

	current_action = do_action;
	New_Timer_Parameters = false;
	SSR_Disable();

	switch (current_action)
	{
		case SSR_Action_OFF:
		{
			SSR_Stop_Timer();
			Gestion_SSR_CallBack = NULL;
			New_Timer_Parameters = SSR_Set_Percent(0);
			break;
		}
		case SSR_Action_FULL:
		{
			Gestion_SSR_CallBack = NULL;
			New_Timer_Parameters = SSR_Set_Percent(100);
			SSR_Start_Timer();
			if (restart)
				SSR_Enable();
			break;
		}
		case SSR_Action_Percent:
		{
			Gestion_SSR_CallBack = NULL;
			New_Timer_Parameters = SSR_Set_Percent(10);
			SSR_Start_Timer();
			if (restart)
				SSR_Enable();
			break;
		}
		case SSR_Action_Surplus:
		{
			// On démarre à zéro pourcent
			New_Timer_Parameters = SSR_Set_Percent(0);
			Gestion_SSR_CallBack = &SSR_Update_Surplus_Timer;
			SSR_Start_Timer();
			if (restart)
				SSR_Enable();
			break;
		}
		case SSR_Action_Dimme:
		{
			// Détermine le pourcentage de démarrage
			if (Dump_Power_Relatif > 0.0)
				pourcent = 100 * (Dimme_Power / (Dump_Power_Relatif * 230.0));
			else
				pourcent = 0.0;
			New_Timer_Parameters = SSR_Set_Percent(pourcent);
			Gestion_SSR_CallBack = &SSR_Update_Dimme_Timer;
			SSR_Start_Timer();
			if (restart)
				SSR_Enable();
			break;
		}
	}
}

/**
 * Renvoie l'action en cours
 */
SSR_Action_typedef SSR_Get_Action(void)
{
	return current_action;
}

// Fonction pour dimmer une puissance cible
void SSR_Set_Dimme_Target(float target)
{
	float pourcent;

	Dimme_Power = target;
	// Détermine le pourcentage de démarrage
	if (Dump_Power_Relatif > 0.0)
		pourcent = 100 * (Dimme_Power / (Dump_Power_Relatif * 230.0));
	else
		pourcent = 0.0;
	New_Timer_Parameters = SSR_Set_Percent(pourcent);
}

float SSR_Get_Dimme_Target(void)
{
	return Dimme_Power;
}

/**
 * Fonction pour définir directement la charge
 * On suppose que la puissance est donnée pour une tension de 230 V
 */
void SSR_Set_Dump_Power(float dump)
{
	Dump_Power = fabs(dump);
	Dump_Power_Relatif = Dump_Power / 230.0;
}

float SSR_Get_Dump_Power(void)
{
	return Dump_Power;
}

/**
 * Renvoie TRUE si le SSR est actif
 */
bool SSR_Get_StateON(void)
{
	return Is_SSR_enabled;
}

void SSR_Enable(void)
{
	Is_SSR_enabled = true;
}

void SSR_Disable(void)
{
	Is_SSR_enabled = false;
	Top_CS_ZC = false;
	// Etre sûr qu'il est low
	SET_PIN_LOW(SSR_PIN);
	SetLedPinLow();
}

// ********************************************************************************
// Control functions
// ********************************************************************************

bool SSR_Set_Percent(float percent)
{
	if (percent < P_MIN)
		percent = P_MIN;
	else
		if (percent > P_MAX)
			percent = P_MAX;

	// On change seulement si on a une différence supérieure à 1%
	bool percent_change = (fabs(P_100 - percent) > 0.01);

	if (percent_change)
	{
		// Calcul du delais en us
		uint32_t delay = lround(fabs(acos(percent / 50.0 - 1.0)) / M_PI * HALF_PERIOD_us);
		SSR_Enable_Timer_Interrupt((bool) (delay < DELAY_MAX));

		if (delay < DELAY_MIN)
			delay = DELAY_MIN;
		SSR_COUNT = delay;

		P_100 = percent;

#if DEBUG_SSR
    PrintVal("Pourcentage %", P_100, false);
    PrintVal("SSR_COUNT ", SSR_COUNT, true);
#endif
	}
	return percent_change;
}

/**
 * Get the actual percent in [0..100]
 */
float SSR_Get_Percent(void)
{
	return P_100;
}

// ********************************************************************************
// Private action
// ********************************************************************************

void SSR_Enable_Timer_Interrupt(bool enable)
{
	if (enable)
	{
		Tim_Interrupt_Enabled = true;
	}
	else
	{
		Tim_Interrupt_Enabled = false;
		SetLedPinLow();
	}
}

void SSR_Start_Timer(void)
{
	SSR_Enable_Timer_Interrupt(true);
}

void SSR_Stop_Timer(void)
{
	SSR_Enable_Timer_Interrupt(false);
}

// ********************************************************************************
// SSR action
// ********************************************************************************

void SSR_Update_Dimme_Timer()
{
#define DELTA_TARGET   2
#define EPSILON        0.2

	// Aucun calcul si le SSR n'est pas actif
	if (!Is_SSR_enabled)
		return;

	float target = Cirrus_power_signed - Dimme_Power;

	if (target < -DELTA_TARGET)
	{
		New_Timer_Parameters = SSR_Set_Percent(P_100 + EPSILON);
	}
	else
		if (target > DELTA_TARGET)
		{
			New_Timer_Parameters = SSR_Set_Percent(P_100 - EPSILON);
		}
}

volatile float Integral = 0.0;
volatile float LastError = 0.0;
volatile float Error, Derivative, Output, Percent;
volatile float TotalOutput = 0.0;
uint8_t Over_Count = 0;

void SSR_Update_Surplus_Timer()
{
	// Aucun calcul si le SSR n'est pas actif
	if (!Is_SSR_enabled)
		return;

//  float Kp = 0.2, Ki = 0.0001, Kd = 0.0;  // Bon Kd 0 ou 1
	float Kp = 0.2, Ki = 0.0001;  // Kd = 0.0;
	float Dt = 200.0;
	float SetPoint = 0.0;
	float New_Dump_Power = Cirrus_voltage * Dump_Power_Relatif;

	// calculate the difference between the desired value and the actual value
	Error = SetPoint - Cirrus_power_signed;
	// track error over time, scaled to the timer interval
	Integral += (Error * Dt);
	// determine the amount of change from the last time checked
//  Derivative = (Error - LastError) / Dt;
	// calculate how much drive the output in order to get to the
	// desired setpoint.
	Output = (Kp * Error) + (Ki * Integral); // + (Kd * Derivative);
	// remember the error for the next time around.
//  LastError = Error;

	TotalOutput += (Output / 2);

	if (TotalOutput < 0)
	{
		TotalOutput = 0.0;
		Over_Count++;
	}
	else
		if (TotalOutput > New_Dump_Power)
		{
			TotalOutput = New_Dump_Power;
			Over_Count++;
		}
		else
			Over_Count = 0;

	// On est toujours en erreur, on réinitialise
	if (Over_Count > TRY_MAX)
	{
		Integral = 0.0;
		LastError = 0.0;
		TotalOutput = 0.0;
		Over_Count = 0;
	}

	New_Timer_Parameters = SSR_Set_Percent(100.0 * TotalOutput / New_Dump_Power);
}

// ********************************************************************************
// End of file
// ********************************************************************************
