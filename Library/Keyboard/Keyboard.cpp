#include "Keyboard.h"

#ifdef ESP32
#include "hal/adc_types.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_continuous.h>
#endif

#ifdef KEYBOARD_USE_TASK
#include "Tasks_utils.h"
#endif

const char *Btn_Texte[BTN_MAX] = {"NO btn pressed", "K1 pressed", "K2 pressed", "K3 pressed",
		"K4 pressed"};

// Variables clavier
static uint16_t Low_sampling[BTN_MAX];  // tableau de stockage des valeurs basses

#ifdef ESP8266
uint8_t Keyboard_Channel;
#else
adc_oneshot_unit_handle_t adc1_handle = nullptr; // @suppress("Type cannot be resolved")
adc_channel_t Keyboard_Channel;
#endif
static uint8_t Btn_Count = 0;
static bool Keyboard_Initialized = false;
volatile Btn_Action Btn_Clicked = Btn_NOP;
volatile uint16_t last_ADC = 0;

static uint32_t ADC_res = 1023;  // Echantillonnage 10 bits

KeyBoard_Click_cb KBClick_cb = NULL;

void Btn_Definition_1B();
void Btn_Definition_2B();
void Btn_Definition_3B();
void Btn_Definition_4B();
Btn_Action RawToBtn(uint16_t val);

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
#ifdef ESP32
adc_channel_t GPIO_NUM_toADCChannel(uint8_t gpio)
{
#if CONFIG_IDF_TARGET_ESP32
	switch (gpio)
	{
		case GPIO_NUM_36:
			return ADC_CHANNEL_0;
		case GPIO_NUM_37:
			return ADC_CHANNEL_1;
		case GPIO_NUM_38:
			return ADC_CHANNEL_2;
		case GPIO_NUM_39:
			return ADC_CHANNEL_3;
		case GPIO_NUM_32:
			return ADC_CHANNEL_4;
		case GPIO_NUM_33:
			return ADC_CHANNEL_5;
		case GPIO_NUM_34:
			return ADC_CHANNEL_6;
		case GPIO_NUM_35:
			return ADC_CHANNEL_7;
	}
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
	switch (gpio)
	{
		case GPIO_NUM_1:
			return ADC_CHANNEL_0;
		case GPIO_NUM_2:
			return ADC_CHANNEL_1;
		case GPIO_NUM_3:
			return ADC_CHANNEL_2;
		case GPIO_NUM_4:
			return ADC_CHANNEL_3;
		case GPIO_NUM_5:
			return ADC_CHANNEL_4;
		case GPIO_NUM_6:
			return ADC_CHANNEL_5;
		case GPIO_NUM_7:
			return ADC_CHANNEL_6;
		case GPIO_NUM_8:
			return ADC_CHANNEL_7;
		case GPIO_NUM_9:
			return ADC_CHANNEL_8;
		case GPIO_NUM_10:
			return ADC_CHANNEL_9;
	}
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2
	switch (gpio)
	{
		case GPIO_NUM_0:
			return ADC_CHANNEL_0;
		case GPIO_NUM_1:
			return ADC_CHANNEL_1;
		case GPIO_NUM_2:
			return ADC_CHANNEL_2;
		case GPIO_NUM_3:
			return ADC_CHANNEL_3;
		case GPIO_NUM_4:
			return ADC_CHANNEL_4;
	}
#elif CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2
	switch (gpio)
	{
		case GPIO_NUM_0:
			return ADC_CHANNEL_0;
		case GPIO_NUM_1:
			return ADC_CHANNEL_1;
		case GPIO_NUM_2:
			return ADC_CHANNEL_2;
		case GPIO_NUM_3:
			return ADC_CHANNEL_3;
		case GPIO_NUM_4:
			return ADC_CHANNEL_4;
		case GPIO_NUM_5:
			return ADC_CHANNEL_5;
		case GPIO_NUM_6:
			return ADC_CHANNEL_6;
	}
#endif // CONFIG_IDF_TARGET_*
	return ADC_CHANNEL_0;
}

bool GPIO_NUM_toADCChannel(uint8_t gpio, adc_channel_t *adc_channel)
{
	adc_channel_t channel[1];
	adc_unit_t adc_unit = ADC_UNIT_1;
	esp_err_t err = ESP_OK;
	uint8_t pins[] = {gpio};

	err = adc_continuous_io_to_channel(pins[0], &adc_unit, &channel[0]);
	if (err != ESP_OK)
	{
		log_e("Pin %u is not ADC pin!", pins[0]);
		return false;
	}
	if (adc_unit != 0)
	{
		log_e("Only ADC1 pins are supported in continuous mode!");
		return false;
	}
	*adc_channel = channel[0];
	return true;
}

void InitADC(adc_channel_t channel)
{
	//-------------ADC1 Init---------------//
	adc_oneshot_unit_init_cfg_t init_config1 = { // @suppress("Type cannot be resolved")
			.unit_id = ADC_UNIT_1,
			.clk_src = ADC_RTC_CLK_SRC_DEFAULT,
			.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle)); // @suppress("Invalid arguments")

	//-------------ADC1 Config---------------//
	adc_oneshot_chan_cfg_t config = { // @suppress("Type cannot be resolved")
			.atten = ADC_ATTEN_DB_12,
			.bitwidth = ADC_BITWIDTH_DEFAULT, // default width is max supported width
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel, &config)); // @suppress("Invalid arguments")
}

#endif

void Keyboard_Init_Base(uint8_t pin, uint8_t nbButton)
{
#ifdef ESP8266
	Keyboard_Channel = pin;
#else
	GPIO_NUM_toADCChannel(pin, &Keyboard_Channel);
	InitADC(Keyboard_Channel);
#endif
	Keyboard_Initialized = false;
	Btn_Count = nbButton;
}

/**
 * Initialisation du clavier.
 * channel : ADC pin
 * nbButton : number of button, between 1 to 4
 * sampling : 10, 12 or 16 bits
 * Ne pas oublier de mettre la fonction Keyboard_UpdateTime() dans le loop principal
 */
void Keyboard_Initialize(uint8_t pin, uint8_t nbButton, ADC_Sampling sampling,
		const KeyBoard_Click_cb &kbClick)
{
	Keyboard_Init_Base(pin, nbButton);
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
}

/**
 * Initialisation du clavier.
 * channel : ADC pin
 * nbButton : number of button, max 4
 * interval : intervals to be considered, max to min : nbButton + 1 values
 * Ne pas oublier de mettre la fonction Keyboard_UpdateTime() dans le loop principal
 */
void Keyboard_Initialize(uint8_t pin, uint8_t nbButton, const uint16_t interval[],
		const KeyBoard_Click_cb &kbClick)
{
	Keyboard_Init_Base(pin, nbButton);
	KBClick_cb = kbClick;

	ADC_res = interval[0];
	for (uint8_t i = 0; i < Btn_Count; i++)
	{
		Low_sampling[i] = interval[i + 1];
	}
	Keyboard_Initialized = true;
}

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
	static unsigned long Start_click = 0;
	static uint64_t cumul = 0;
	static uint64_t count = 0;
	uint16_t raw = 0;

	if (Keyboard_Initialized)
	{
		// Each 10 ms
		if ((millis() - lastTimeRead) > 10)
		{
			lastTimeRead = millis();

			// Read raw adc value and test is in the minimal interval
			if (Btn_Click_Val(&raw))
			{
				// We start a cumul
				if (count == 0)
					Start_click = lastTimeRead;

				cumul += raw;
				count++;

				// Ok, we have a real click
				if (lastTimeRead - Start_click > DEBOUNCING_MS)
				{
					Btn_Clicked = RawToBtn(cumul / count);
					cumul = 0;
					count = 0;
				}
			}
			else
			{
				// raw is not significatif or debouncing time is not reached
				cumul = 0;
				count = 0;
			}
			if (KBClick_cb)
			{
				KBClick_cb(Btn_Clicked);
			  Btn_Clicked = Btn_NOP;
			}
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
	*Btn = Btn_Clicked;
	Btn_Clicked = Btn_NOP;  // On "mange" la valeur
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

Btn_Action Btn_Click()
{
	uint8_t i;
	Btn_Action btn = Btn_NOP;
	uint16_t val = Btn_Click_Val();

	// On a appuyé sur un bouton
	if (val > Low_sampling[Btn_Count - 1])
	{ // si différent de 0
		for (i = 0; i < Btn_Count; i++)
		{ // parcours des valeurs
			if (val >= Low_sampling[i])
			{ // test si supérieur à valeur concernée
				btn = (Btn_Action) (Btn_Count - i);
				break; //sortie de boucle
			}
		}
	}
	last_ADC = val;
	return btn;
}

Btn_Action RawToBtn(uint16_t val)
{
	uint8_t i;
	Btn_Action btn = Btn_NOP;

	// On a appuyé sur un bouton
	if (val > Low_sampling[Btn_Count - 1])
	{ // si différent de 0
		for (i = 0; i < Btn_Count; i++)
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

#define RAW_CUMUL	10
/**
 * Return the raw value of the ADC
 */
inline uint16_t Btn_Click_Val()
{
#ifdef ESP8266
	return analogRead(Keyboard_Channel);
#else
	int raw = 0;
	int cumul = 0;
	// First value is discarded
	for (int i = 0; i < RAW_CUMUL; i++)
		adc_oneshot_read(adc1_handle, Keyboard_Channel, &raw); // @suppress("Invalid arguments")
	for (int i = 0; i < RAW_CUMUL; i++)
	{
		//	delayMicroseconds(150); // ADC take about 130 us
		adc_oneshot_read(adc1_handle, Keyboard_Channel, &raw); // @suppress("Invalid arguments")
		cumul += raw;
	}
	return cumul / RAW_CUMUL;
#endif
}

/**
 * Return true if the raw value of the ADC is superior of the minimun value of the interval
 */
inline bool Btn_Click_Val(uint16_t *value)
{
#ifdef ESP8266
	*value = analogRead(Keyboard_Channel);
	return (*value > Low_sampling[Btn_Count - 1]);
#else
	int raw = 0;
	int cumul = 0;
	// First value is discarded
	for (int i = 0; i < RAW_CUMUL; i++)
		adc_oneshot_read(adc1_handle, Keyboard_Channel, &raw); // @suppress("Invalid arguments")
	for (int i = 0; i < RAW_CUMUL; i++)
	{
		//	delayMicroseconds(150); // ADC take about 130 us
		adc_oneshot_read(adc1_handle, Keyboard_Channel, &raw); // @suppress("Invalid arguments")
		cumul += raw;
	}
	*value = cumul / RAW_CUMUL;
	return (*value > Low_sampling[Btn_Count - 1]);
#endif
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
void __attribute__((weak)) UserKeyboardAction(Btn_Action Btn_Clicked)
{
	if (Btn_Clicked != Btn_NOP)
		Serial.println(Btn_Texte[Btn_Clicked]);
}

void KEYBOARD_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("KEYBOARD_Task");

	static int64_t lastTimeRead = esp_timer_get_time();
	static int64_t Start_click = 0;
	static uint64_t cumul = 0;
	static uint64_t count = 0;
	uint16_t raw = 0;

	for (EVER)
	{
		if (Keyboard_Initialized)
		{
			lastTimeRead = esp_timer_get_time();

			// Read raw adc value and test is in the minimal interval
			if (Btn_Click_Val(&raw))
			{
				// We start a cumul
				if (count == 0)
					Start_click = lastTimeRead;

				cumul += raw;
				count++;

				// Ok, we have a real click
				if (lastTimeRead - Start_click > DEBOUNCING_US)
				{
					Btn_Clicked = RawToBtn(cumul / count);
					cumul = 0;
					count = 0;
				}
			}
			else
			{
				// raw is not significatif or debouncing time is not reached
				cumul = 0;
				count = 0;
			}
			UserKeyboardAction(Btn_Clicked);
			Btn_Clicked = Btn_NOP;

		}
		END_TASK_CODE();
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
