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
#elif ESP32
// Voir https://deepbluembedded.com/esp32-timers-timer-interrupt-tutorial-arduino-ide/ pour ESP32 2.0.x
// https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html pour ESP32 3.0.x
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
// Le numéro du timer utilisé [0..3]
#define TIMER_NUM	0
#endif
hw_timer_t *Timer_SSR = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#define TIMERMUX_ENTER()	portENTER_CRITICAL_ISR(&timerMux)
#define TIMERMUX_EXIT()	portEXIT_CRITICAL_ISR(&timerMux);
volatile SemaphoreHandle_t topZC_Semaphore;
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
// Le channel pour la led SSR en PWM
#define LED_CHANNEL	0
#endif
#endif  // ESP32

#define DEBUG_SSR           0          // Affichage de P_100 et SSR_COUNT

// Les handle des pins et du timer
volatile uint8_t SSR_PIN;
volatile int8_t LED_PIN = -1;

#ifdef SIMPLE_ZC_TEST
#if !defined(ZERO_CROSS_TOP_Xms)
#define ZERO_CROSS_TOP_Xms	20
#endif
#endif

#if defined(ZERO_CROSS_TOP_Xms)
volatile uint32_t Count_CS_ZC = 0; // Un compteur des tops ZC
volatile bool Top_Xms = false;   // Un top X ms à utiliser avec la fonction ZC_Top_Xms()
#endif
volatile bool Top_CS_ZC_Mux = false;   // Top ZC

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

// Le surplus cible du SSR pour le PID
float SSR_Target = 0.0;

// Le pourcentage pour ce mode
float SSR_Percent = 10.0;

// Nombre de pas avant démarrage du SSR normalement entre [0 .. HALF_PERIOD_us]
// mais restreint à [DELAY_MIN .. DELAY_MAX]
volatile uint32_t SSR_COUNT = DELAY_MAX;

// Le pourcentage d'utilisation du SSR
volatile float P_100 = 0.0;

// Timer params
volatile bool Tim_Interrupt_Enabled = false;
// Indique si le SSR est activé ou pas
volatile bool Is_SSR_enabled = false;
volatile bool Is_SSR_enabled_Mux = false;

// L'action en cours sur le SSR
SSR_Action_typedef current_action = SSR_Action_OFF;

// La puissance cible en version dimmer
float Dimme_Power = 0.0;

// For PID
static float Integral = 0.0;
//	static float LastError = 0.0;
static float TotalOutput = 0.0;
static uint8_t Over_Count = 0;

// La tension et la puissance en cours fournies par le Cirrus ou autre
extern float Cirrus_voltage;
extern float Cirrus_power_signed;
extern bool CIRRUS_get_rms_data(float *uRMS, float *pRMS);
// Fonction de gestion du SSR en mode dimme ou surplus
volatile Gestion_SSR_TypeDef Gestion_SSR_CallBack = NULL;

// ********************************************************************************
// Private functions
// ********************************************************************************

bool Set_Percent(float percent, bool start_timer = true);

// Actualisation des paramètres du Timer
void SSR_Update_Dimme_Timer();
void SSR_Update_Surplus_Timer();

// Gestion du Timer
void SSR_Enable_Timer_Interrupt(bool enable);
void SSR_Start_Timer(void);
void SSR_Stop_Timer(void);

inline void Restart_PID(void)
{
	// Reset parameters
	Integral = 0.0;
//	LastError = 0.0;
	TotalOutput = 0.0;
	Over_Count = 0;
}

// Fonction de DEBUG_SSR
extern void PrintTerminal(const char *text);
void PrintVal(const char *text, float val, bool integer)
{
	char buffer[50];
	if (integer)
		sprintf(buffer, "%s: %d\r\n", text, (int) val);
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

/**
 * Led command
 */
void IRAM_ATTR SetLedPinLow(uint16_t val = 0)
{
	if (LED_PIN != -1)
#ifdef ESP8266
		SET_PIN_LOW(LED_PIN);
	(void) val;
#else
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
		ledcWrite(LED_CHANNEL, val);
#else
		ledcWrite(LED_PIN, val);
#endif
#endif // ESP32
}

/**
 * Initialize the timer and start it for once time
 */
void IRAM_ATTR startTimerAndTrigger(uint32_t delay)
{
#ifdef ESP8266
	Timer_SSR.setInterval(delay);
	Timer_SSR.startTimer();
#elif ESP32
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
	timerWrite(Timer_SSR, 0);
	timerAlarmWrite(Timer_SSR, delay, false);
	timerAlarmEnable(Timer_SSR);
	timerStart(Timer_SSR);
#else
	timerRestart(Timer_SSR);
	timerAlarm(Timer_SSR, delay, false, 0);
#endif
#endif // ESP32
}

/**
 * Timer callback
 * Send SSR pulse after delay
 * Timer is stoped after SSR pulse (for ESP32, autoreload = false)
 */
void IRAM_ATTR onTimerSSR(void)
{
	TIMERMUX_ENTER();
	if (Is_SSR_enabled_Mux)
	{
		if (Top_CS_ZC_Mux)
		{
			SET_PIN_HIGH(SSR_PIN);
			Top_CS_ZC_Mux = false;
			startTimerAndTrigger(SSR_TURN_ON_us);
		}
		else
		{
			SET_PIN_LOW(SSR_PIN);

#ifdef ESP8266
			if (LED_PIN != -1)
				SET_PIN_HIGH(LED_PIN);
			Timer_SSR.stopTimer();
#endif
		}
	}
	TIMERMUX_EXIT();
}

#ifdef SIMPLE_ZC_TEST
/**
 * Zero cross interrupt callback
 * Simple test to validate the zero cross interrupt
 * Each second, we turn on the led for 100 ms
 */
void IRAM_ATTR onCirrusZC(void)
{
	// Zero cross count for Top_Xms
	Count_CS_ZC = Count_CS_ZC + 1; // ++ deprecated with volatile
	if (!Top_Xms)
		Top_Xms = (Count_CS_ZC % ZERO_CROSS_TOP_Xms == 0);

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
/**
 * Zero cross interrupt callback
 * Define and start the SSR timer according SSR_COUNT delay
 * Note: we must redefine it even if SSR_COUNT has not changed (New_Timer_Parameters is false)
 * because the delay has been changed in TimerSSR callback
 */
void IRAM_ATTR onCirrusZC(void)
{
#ifdef ESP32
  // Give a semaphore that we can used to synchronise with ZC cirrus
  xSemaphoreGiveFromISR(topZC_Semaphore, NULL);
#endif

	TIMERMUX_ENTER();

#if defined(ZERO_CROSS_TOP_Xms)
	// Zero cross count for Top_Xms
	Count_CS_ZC = Count_CS_ZC + 1; // ++ deprecated with volatile
	if (!Top_Xms)
		Top_Xms = (Count_CS_ZC % ZERO_CROSS_TOP_Xms == 0);
#endif

	if (Is_SSR_enabled_Mux)
	{
		if (Tim_Interrupt_Enabled)
		{
			Top_CS_ZC_Mux = true;
			startTimerAndTrigger(SSR_COUNT);

			// Plus le délai est long, plus la led sera éteinte longtemps
			SetLedPinLow(1023 - SSR_COUNT / 9);
		}
	}
	TIMERMUX_EXIT();
}
#endif // SIMPLE_ZC_TEST

#if defined(ZERO_CROSS_TOP_Xms)
/**
 * Renvoie le nombre de zéro cross depuis le démarrage
 */
uint32_t ZC_Get_Count(void)
{
	TIMERMUX_ENTER();
	uint32_t count = Count_CS_ZC;
	TIMERMUX_EXIT();
	return count;
}

/**
 * Cette fonction retourne true si 200 ms se sont écoulé depuis sa dernière interrogation
 * Les 200 ms sont calculés par le comptage des tops zéro cross du cirrus, callback onCirrusZC()
 */
bool ZC_Top_Xms(void)
{
	TIMERMUX_ENTER();
	bool top = Top_Xms;
	Top_Xms = false;
	TIMERMUX_EXIT();
	return top;
}
#endif

// ********************************************************************************
// Initialisation
// ********************************************************************************

/**
 * SSR_Initialize : initialisation du SSR avec les callback zéro-cross et timer
 * ZC_Pin : le pin zéro-cross
 * SSR_Pin : le pin actionnant le SSR
 * LED_Pin : le pin de la led indiquant le status du SSR (default -1)
 * Le SSR est désactivé.
 * D'abord définir le type d'action avec SSR_Action() puis appeler SSR_Enable() pour l'activer.
 */
void SSR_Initialize(uint8_t ZC_Pin, uint8_t SSR_Pin, int8_t LED_Pin)
{
	bool timer_OK = false;

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
	timer_OK = Timer_SSR.setInterval(SSR_COUNT, onTimerSSR, false);
#elif ESP32
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
	Timer_SSR = timerBegin(TIMER_NUM, 80, true);  // Pour une clock de 80 MHz => tick de 1 us
	timerAttachInterrupt(Timer_SSR, &onTimerSSR, true);
#else
	Timer_SSR = timerBegin(1000000); // Fixe la fréquence à 1 MHz => tick de 1 us
	timerAttachInterrupt(Timer_SSR, &onTimerSSR);
#endif
	timer_OK = (Timer_SSR != NULL);

  // Create semaphore to inform us when the zero cross has fired
	topZC_Semaphore = xSemaphoreCreateBinary();
#endif // ESP32

	if (timer_OK)
		print_debug("Starting Timer_SSR OK");
	else
		print_debug("Error starting Timer_SSR");

	// Led associé au SSR
	LED_PIN = LED_Pin;
	if (LED_PIN != -1)
	{
		pinMode(LED_PIN, OUTPUT);
		SET_PIN_LOW(LED_PIN);
#ifndef SIMPLE_ZC_TEST
#ifdef ESP32
#if defined(ESP_IDF_VERSION) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)) // ESP32 2.0.x
		// Configure le channel
		ledcSetup(LED_CHANNEL, 1000, 10);  // 10 bits = 1024

		// Attache le channel sur le pin led
		ledcAttachPin(LED_PIN, LED_CHANNEL); // ESP32 2.0.11
#else
		ledcAttach(LED_PIN, 1000, 10);
#endif
#endif // ESP32
#endif // SIMPLE_ZC_TEST
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
 * Note : utilise le channel 1 du cirrus
 */
float SSR_Compute_Dump_power(float default_Power)
{
#define count_max   20
	float urms, prms;
	float cumul_p = 0.0;
	float cumul_u = 0.0;
	float initial_p, initial_u, final_p, final_u;

	// Save current action
	SSR_Action_typedef action = current_action;

	// On récupère la puissance et la tension initiale sur 3 s
	CIRRUS_get_rms_data(&urms, &prms);
	cumul_p = prms;
	cumul_u = urms;
	for (int i = 1; i < count_max; i++)
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
	SSR_Action(SSR_Action_FULL, true);
	delay(2000); // Pour stabiliser

	CIRRUS_get_rms_data(&urms, &prms);
	cumul_p = prms;
	cumul_u = urms;
	for (int i = 1; i < count_max; i++)
	{
		delay(150);
		CIRRUS_get_rms_data(&urms, &prms);
		cumul_p += prms;
		cumul_u += urms;
	}

	// On éteint le SSR
	SSR_Action(SSR_Action_OFF);

	final_p = cumul_p / count_max;
	final_u = cumul_u / count_max;

	Dump_Power_Relatif = (final_p - initial_p) / ((initial_u + final_u) / 2.0);

	// La charge ne devait pas être branchée, on utilise la puissance par défaut
	if (Dump_Power_Relatif < 0.5)
	{
		if (default_Power != 0.0)
			Dump_Power_Relatif = default_Power / 230.0;
		else
			Dump_Power_Relatif = Dump_Power / 230.0;
	}
	PrintVal("Puissance de la charge", final_p - initial_p, false);
	PrintVal("Résistance de la charge", (final_u * final_u) /(final_p - initial_p), false);
	PrintVal("Puissance de la charge relative", Dump_Power_Relatif, false);

	// Restaure current action
	SSR_Action(action);

	return final_p - initial_p;
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
	current_action = do_action;
	SSR_Disable();

	switch (current_action)
	{
		case SSR_Action_OFF:
		case SSR_Action_FULL:
		case SSR_Action_Percent:
		{
			Gestion_SSR_CallBack = NULL;
			break;
		}
		case SSR_Action_Surplus:
		{
			Gestion_SSR_CallBack = &SSR_Update_Surplus_Timer;
			break;
		}
		case SSR_Action_Dimme:
		{
			Gestion_SSR_CallBack = &SSR_Update_Dimme_Timer;
			break;
		}
	}

	if ((current_action != SSR_Action_OFF) && restart)
		SSR_Enable();
}

/**
 * Renvoie l'action en cours
 */
SSR_Action_typedef SSR_Get_Action(void)
{
	return current_action;
}

/**
 * Renvoie l'état du SSR
 * SSR_OFF: éteint
 * SSR_ON: allumé mais pas actif
 * SSR_ON_ACTIF; allumé actif
 */
SSR_State_typedef SSR_Get_State(void)
{
	if (!Is_SSR_enabled)
		return SSR_OFF;
	else
	{
		if (Tim_Interrupt_Enabled)
			return SSR_ON_ACTIF;
		else
			return SSR_ON;
	}
}

void SSR_Enable(void)
{
	Restart_PID();
	Is_SSR_enabled_Mux = true;

	// Restaure initial percent
	P_100 = 0;
	switch (current_action)
	{
		case SSR_Action_OFF:
		{
			Set_Percent(0);
			break;
		}
		case SSR_Action_FULL:
		{
			Set_Percent(100);
			break;
		}
		case SSR_Action_Percent:
		{
			Set_Percent(SSR_Percent);
			break;
		}
		case SSR_Action_Surplus:
		{
			Restart_PID();
			// On démarre à zéro pourcent
			Set_Percent(0);
			break;
		}
		case SSR_Action_Dimme:
		{
			float pourcent;
			// Détermine le pourcentage de démarrage
			if (Dump_Power_Relatif > 0.0)
				pourcent = 100 * (Dimme_Power / (Dump_Power_Relatif * 230.0));
			else
				pourcent = 0.0;
			Set_Percent(pourcent);
			break;
		}
	}

	Is_SSR_enabled = true;
}

void SSR_Disable(void)
{
	Is_SSR_enabled = false;
	TIMERMUX_ENTER();
	Is_SSR_enabled_Mux = Is_SSR_enabled;
	Top_CS_ZC_Mux = false;
	TIMERMUX_EXIT();
	delay(10);
	SSR_Stop_Timer();
	// Etre sûr qu'il est low
	SET_PIN_LOW(SSR_PIN);
	SetLedPinLow();
}

// ********************************************************************************
// Control functions
// ********************************************************************************

/**
 * Fonction pour définir directement la charge
 * On suppose que la puissance est donnée pour une tension de 230 V
 */
void SSR_Set_Dump_Power(float dump)
{
	bool isrunning = !(SSR_Get_State() == SSR_OFF);
	SSR_Disable();
	Dump_Power = fabs(dump);
	Dump_Power_Relatif = Dump_Power / 230.0;

	if (isrunning)
		SSR_Enable();
}

float SSR_Get_Dump_Power(void)
{
	return Dump_Power;
}

/**
 * The target surplus
 */
void SSR_Set_Target(float target)
{
	bool isrunning = !(SSR_Get_State() == SSR_OFF);
	SSR_Disable();
	SSR_Target = target;

	if (isrunning)
		SSR_Enable();
}

float SSR_Get_Target(void)
{
	return SSR_Target;
}

void SSR_Set_Percent(float percent)
{
	bool isrunning = !(SSR_Get_State() == SSR_OFF);
	SSR_Disable();
	SSR_Percent = percent;

	if (isrunning)
		SSR_Enable();
}

float SSR_Get_Percent(void)
{
	return SSR_Percent;
}

/**
 * Get the actual percent in [0..100]
 */
float SSR_Get_Current_Percent(void)
{
	return P_100;
}

// Fonction pour dimmer une puissance cible
void SSR_Set_Dimme_Target(float target)
{
	bool isrunning = !(SSR_Get_State() == SSR_OFF);
	SSR_Disable();
	Dimme_Power = target;

	if (isrunning)
		SSR_Enable();
}

float SSR_Get_Dimme_Target(void)
{
	return Dimme_Power;
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

/**
 * Compute SSR_COUNT
 * Enable or disable timer according SSR_COUNT
 * return true if percent has changed
 */
bool Set_Percent(float percent, bool start_timer)
{
	if (percent < P_MIN)
		percent = P_MIN;
	else
		if (percent > P_MAX)
			percent = P_MAX;

	// On change seulement si on a une différence supérieure à 1%
	if (fabs(P_100 - percent) > 0.01)
	{
		// Calcul du delais en us
		uint32_t delay = lround(fabs(acos(percent / 50.0 - 1.0)) / M_PI * HALF_PERIOD_us);
		if (start_timer)
			SSR_Enable_Timer_Interrupt((bool) (delay < DELAY_MAX));

		if (delay < DELAY_MIN)
			delay = DELAY_MIN;
		SSR_COUNT = delay;

		P_100 = percent;

#if DEBUG_SSR
    PrintVal("Pourcentage %", P_100, false);
    PrintVal("SSR_COUNT ", SSR_COUNT, true);
#endif

    return true;
	}
	return false;
}

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
		Set_Percent(P_100 + EPSILON);
	}
	else
		if (target > DELTA_TARGET)
		{
			Set_Percent(P_100 - EPSILON);
		}
}

/**
 * PID controler to control the surplus allowed
 * Don't use derivative part (Kd = 0)
 */
void SSR_Update_Surplus_Timer()
{
	// Aucun calcul si le SSR n'est pas actif
	if (!Is_SSR_enabled)
		return;

//  const float Kp = 0.2, Ki = 0.0001, Kd = 0.0;  // Bon Kd 0 ou 1
	const float Kp = 0.2, Ki = 0.0001;  // Kd = 0.0;
	// Timer interval: need to be adjusted ?
	const float Dt = 100.0;
	// Desired value: here we want zero to have zero surplus
	float SetPoint = SSR_Target;
	float New_Dump_Power = Cirrus_voltage * Dump_Power_Relatif;

	// calculate the difference between the desired value and the actual value
	float Error = SetPoint - Cirrus_power_signed;
	// track error over time, scaled to the timer interval
	Integral += (Error * Dt);
	// determine the amount of change from the last time checked
//  float Derivative = (Error - LastError) / Dt;
	// calculate how much drive the output in order to get to the
	// desired setpoint.
	float Output = (Kp * Error) + (Ki * Integral); // + (Kd * Derivative);
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
		Restart_PID();

	Set_Percent(100.0 * TotalOutput / New_Dump_Power);
}

// ********************************************************************************
// End of file
// ********************************************************************************
