#include "Keyboard.h"
#if defined(ESP32) & defined(KEYBOARD_WITH_ADC)
#include "ADC_utils.h"
#endif

#ifdef KEYBOARD_USE_TASK
#include "Tasks_utils.h"
#define DEBOUNCING_STEP	DEBOUNCING_US
#else
#define DEBOUNCING_STEP	DEBOUNCING_MS
#endif

const char *Btn_Texte[BTN_MAX] = {"NO btn pressed", "K1 pressed", "K2 pressed", "K3 pressed",
		"K4 pressed"};

// Variables clavier
static uint16_t Low_sampling[BTN_MAX];  // tableau de stockage des valeurs basses

static uint8_t Btn_Count = 0;
static bool Keyboard_Initialized = false;
volatile Btn_Action Btn_Clicked = Btn_NOP;
volatile Btn_Action Last_Btn_Clicked = Btn_NOP;
volatile uint16_t last_ADC = 0;

static uint32_t ADC_res = 1023;  // Echantillonnage 10 bits

KeyBoard_Click_cb KBClick_cb = NULL;

#if defined(ESP8266) | (!defined(KEYBOARD_WITH_ADC))
bool ADC_Initialized = true;
#if !defined(KEYBOARD_ADC_GPIO)
#define KEYBOARD_ADC_GPIO	A0
#endif
#else
// ADC must be initialized before initialize keyboard
extern volatile bool ADC_Initialized;
#endif

void Btn_Definition_1B();
void Btn_Definition_2B();
void Btn_Definition_3B();
void Btn_Definition_4B();
Btn_Action RawToBtn(uint16_t val);
void Check_Btn_Clicked(unsigned long time);

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// ********************************************************************************
// Local initialization functions
// ********************************************************************************

/**
 * Initialisation du clavier.
 * nbButton : number of button, between 1 to 4
 * sampling : 10, 12 or 16 bits
 * Ne pas oublier de mettre la fonction Keyboard_UpdateTime() dans le loop principal
 * IMPORTANT: ADC must be initialized before
 */
bool Keyboard_Initialize(uint8_t nbButton, ADC_Sampling sampling, const KeyBoard_Click_cb &kbClick)
{
#if defined(ESP8266) | (!defined(KEYBOARD_WITH_ADC))
	// For ESP8266, we have only D0
	pinMode(KEYBOARD_ADC_GPIO, INPUT);
#endif
	Keyboard_Initialized = false;
	Btn_Count = nbButton;
	KBClick_cb = kbClick;

	switch (sampling)
	{
		case ADC_10bits:
			ADC_res = 1023;
			break;
		case ADC_12bits:
			ADC_res = 4095;
			break;
		case ADC_16bits:
			ADC_res = 65535;
			break;
	}

	// Definition clavier
	switch (Btn_Count)
	{
		case 1:
			Btn_Definition_1B();
			break;
		case 2:
			Btn_Definition_2B();
			break;
		case 3:
			Btn_Definition_3B();
			break;
		case 4:
			Btn_Definition_4B();
			break;
		default:
			print_debug("Erreur définition clavier.");
	}

	if (!ADC_Initialized)
	{
		print_debug("ADC not initialized.");
		Keyboard_Initialized = false;
	}
	else
		Keyboard_Initialized = true;

	return Keyboard_Initialized;
}

/**
 * Initialisation du clavier.
 * nbButton : number of button, max 4
 * interval : intervals to be considered, max to min : nbButton + 1 values
 * Ne pas oublier de mettre la fonction Keyboard_UpdateTime() dans le loop principal
 * IMPORTANT: ADC must be initialized before if we have the define KEYBOARD_WITH_ADC
 */
bool Keyboard_Initialize(uint8_t nbButton, const uint16_t interval[],
		const KeyBoard_Click_cb &kbClick)
{
#if defined(ESP8266) | (!defined(KEYBOARD_WITH_ADC))
	// For ESP8266, we have only D0
	pinMode(KEYBOARD_ADC_GPIO, INPUT);
#endif
	Keyboard_Initialized = false;
	Btn_Count = nbButton;
	KBClick_cb = kbClick;

	ADC_res = interval[0];
	for (uint8_t i = 0; i < Btn_Count; i++)
	{
		Low_sampling[i] = interval[i + 1];
	}
	if (!ADC_Initialized)
	{
		print_debug("ADC not initialized.");
		Keyboard_Initialized = false;
	}
	else
		Keyboard_Initialized = true;

	return Keyboard_Initialized;
}

#if defined(ESP8266) | (!defined(KEYBOARD_WITH_ADC))
uint16_t ADC_Read0(void)
{
	return analogRead(KEYBOARD_ADC_GPIO);
}
#endif

/**
 * Set callback for action when button is clicked
 */
void SetKeyBoardCallback(const KeyBoard_Click_cb &kbClick)
{
	KBClick_cb = kbClick;
}

/**
 * Function that update de keyboard status.
 * This function should be present in the main loop
 * Action if button is clicked can be executed in two way (exclusive) :
 * - call Check_Keyboard regularly
 * - with a callback
 */
void Keyboard_UpdateTime(void)
{
	static unsigned long lastTimeRead = millis();
	unsigned long now;

	if (Keyboard_Initialized)
	{
		// Each 10 ms
		now = millis();
		if ((now - lastTimeRead) > 10)
		{
			lastTimeRead = now;
			Check_Btn_Clicked(lastTimeRead);
		}
	}
}

/**
 * Check le clavier
 * Renvoie true si un bouton a été appuyé
 * Btn : le numéro du bouton
 * Fonction à tester régulièrement dans la boucle principale
 */
bool Check_Keyboard(Btn_Action *Btn)
{
	*Btn = Last_Btn_Clicked;
	Last_Btn_Clicked = Btn_NOP;  // On "mange" la valeur
	return (bool) (*Btn != Btn_NOP);
}

// ********************************************************************************
// Definition clavier
// ********************************************************************************

void Btn_Definition_1B()
{
	// raw adc value :
	// K1 pressed ==> val = 1024

	Low_sampling[0] = 90 * (ADC_res / 100);  //1000;

	Keyboard_Initialized = true;
}

void Btn_Definition_2B()
{
	// raw adc value :
	// K1 pressed ==> val = 534
	// K2 pressed ==> val = 900

	Low_sampling[0] = 90 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 600;

	Keyboard_Initialized = true;
}

void Btn_Definition_3B()
{
	// raw adc value :
	// K1 pressed ==> val = 534
	// K2 pressed ==> val = 794
	// K3 pressed ==> val = 1024

	Low_sampling[0] = 90 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 650;
	Low_sampling[2] = 40 * (ADC_res / 100); // 450;

	Keyboard_Initialized = true;
}

void Btn_Definition_4B()
{
	// raw adc value :
	// K1 pressed ==> val = 447
	// K2 pressed ==> val = 663
	// K3 pressed ==> val = 888
	// K4 pressed ==> val = 1024

	Low_sampling[0] = 80 * (ADC_res / 100); // 920;
	Low_sampling[1] = 60 * (ADC_res / 100); // 650;
	Low_sampling[2] = 40 * (ADC_res / 100); // 450;
	Low_sampling[3] = 30 * (ADC_res / 100); // 300;

	Keyboard_Initialized = true;
}

// ********************************************************************************
// Gestion clavier
// ********************************************************************************

/**
 * Check if button was clicked.
 * time is in us when use task (esp_timer_get_time() function). Use DEBOUNCING_US.
 * time is in ms otherwise. Use DEBOUNCING_MS.
 */
void Check_Btn_Clicked(unsigned long time)
{
	static unsigned long Start_click = 0;
	static uint32_t Btn_Clicked_count = 0;
	static uint64_t cumul = 0;
	static uint64_t count = 0;
	uint16_t raw = 0;

	// Read raw adc value and test is in the minimal interval
	if (Btn_Click_Val(&raw))
	{
		// We start a cumul
		if (count == 0)
			Start_click = time;

		cumul += raw;
		count++;

		// Ok, we have a real click
		if (time - Start_click > DEBOUNCING_STEP)
		{
			Btn_Clicked = RawToBtn(cumul / count);
			if (Last_Btn_Clicked == Btn_Clicked)
				Btn_Clicked_count++;
			else
			{
				Last_Btn_Clicked = Btn_Clicked;
				Btn_Clicked_count = 1;
			}

			if (KBClick_cb)
				KBClick_cb(Btn_Clicked, Btn_Clicked_count);
			cumul = 0;
			count = 0;
		}
	}
	else
	{
		// raw is not significatif
		cumul = 0;
		count = 0;
		Btn_Clicked_count = 0;
		Btn_Clicked = Btn_NOP;
	}
}

Btn_Action Btn_Click()
{
	last_ADC = Btn_Click_Val();
	return RawToBtn(last_ADC);
}

Btn_Action RawToBtn(uint16_t val)
{
	Btn_Action btn = Btn_NOP;

	// On a appuyé sur un bouton
	if (val > Low_sampling[Btn_Count - 1])
	{ // si différent de 0
		for (uint8_t i = 0; i < Btn_Count; i++)
		{ // parcours des valeurs
			if (val >= Low_sampling[i])
			{ // test si supérieur à valeur concernée
				btn = (Btn_Action) (Btn_Count - i);
				break; //sortie de boucle
			}
		}
	}
	return btn;
}

/**
 * Return the name of the button clicked
 */
const char* Btn_Click_Name()
{
	return Btn_Texte[Btn_Click()];
}

/**
 * Return the raw value of the ADC
 */
uint16_t Btn_Click_Val()
{
	return ADC_Read0();
}

/**
 * Return true if the raw value of the ADC is superior of the minimun value of the interval
 */
bool Btn_Click_Val(uint16_t *value)
{
	*value = ADC_Read0();
	return (*value > Low_sampling[Btn_Count - 1]);
}

/**
 * Test de l'interval de détection des boutons
 * Relevé de mesures pendant 10 secondes (défaut) ou boucle infinie (infinite = true)
 */
void Btn_Check_Config(bool infinite)
{
	char buffer_deb[20] = {0};
	char buffer[50] = {0};
	uint32_t Check_count = 0;

	print_debug("\r\n ** Info Keyboard **");
	sprintf(buffer_deb, "[%d - ", (unsigned int) ADC_res);
	for (uint8_t i = 0; i < Btn_Count; i++)
	{
		sprintf(buffer, "%s%d] ==> %s", buffer_deb, Low_sampling[i], Btn_Texte[Btn_Count - i]);
		print_debug(buffer);
		sprintf(buffer_deb, "[%d - ", Low_sampling[i]);
	}
	sprintf(buffer, "%s0] ==> %s\r\n", buffer_deb, Btn_Texte[0]);
	print_debug(buffer);

	// Run the test
	print_debug("Check Keyboard :");
	while (1)
	{
		sprintf(buffer, "%s ==> val = %d", Btn_Click_Name(), last_ADC);
		print_debug(buffer);

		delay(200);
		Check_count++;
		if (!infinite)
		{
			if (Check_count > 50)
				break;
		}
	}
}

// ********************************************************************************
// Basic Task function to check KeyBoard
// ********************************************************************************
/**
 * A basic Task to check KeyBoard
 */
#ifdef KEYBOARD_USE_TASK

/**
 * This functions should be redefined elsewhere with your analyse
 */
void __attribute__((weak)) UserKeyboardAction(Btn_Action Btn_Clicked, uint32_t count)
{
	if (Btn_Clicked != Btn_NOP)
	{
		Serial.println(Btn_Texte[Btn_Clicked]);
		Serial.println(count);
	}
}

void KEYBOARD_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("KEYBOARD_Task");

	// Set user keyboard callback action
	KBClick_cb = &UserKeyboardAction;

	for (EVER)
	{
		if (Keyboard_Initialized)
		{
			Check_Btn_Clicked(esp_timer_get_time());
		}
		END_TASK_CODE(false);
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
