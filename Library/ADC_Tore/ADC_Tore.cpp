#include "ADC_Tore.h"

#ifdef ADC_USE_TASK
#include "Tasks_utils.h"
#endif

#if defined(ESP8266) | defined(KEYBOARD_ESP32_ARDUINO)
uint8_t ADC_gpio;
#else

//#include "hal/adc_types.h"
#include "soc/soc_caps.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_continuous.h>

// Defined in Keyboard unit
extern adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t ADC_Channel;
#endif

// We do a mean over PERIOD_COUNT period (100 ms)
#define PERIOD_COUNT	5

typedef enum
{
	adcUndef,
	adcOnseShot,
	adcContinuous
} ADC_Status_typedef;

ADC_Status_typedef ADC_Status = adcUndef;

// Timer to get data in OneShot mode
hw_timer_t *Timer_Tore = NULL;
uint32_t Timer_Read_Delay = 100;	// 0.1 ms

// ZC semaphore from Cirrus
extern volatile SemaphoreHandle_t topZC_Semaphore;

// Critical section for ADC
static portMUX_TYPE ADC_Mux = portMUX_INITIALIZER_UNLOCKED;

// To avoid division
#define MEAN_WAVE_CUMUL	true

// ADC cumul
volatile int ADC_cumul = 0;
volatile int ADC_cumul_count = 0;
volatile int ADC_cumul_count_int = 0;

// ZC operation
#if MEAN_WAVE_CUMUL == true
volatile int current_cumul_int = 0;
volatile int raw_current_int = 0;
volatile int raw_current_count = 1; // To avoid division by zero at beginning
#else
volatile int current_cumul = 0;
volatile int raw_current = 0;
#endif

volatile int current_cumul_count = 0;

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

// Mean computation
void Compute_Mean_Wave(void);

// ********************************************************************************
// Initialization functions in Oneshot mode
// ********************************************************************************

/**
 * Timer callback
 * Read continuously ADC
 */
void IRAM_ATTR onTimerTore(void)
{
	int v_shifted = 0;
#if defined(ESP8266) | defined(KEYBOARD_ESP32_ARDUINO)
  v_shifted = analogRead(ADC_gpio) - ADC_ZERO;
	ADC_cumul = ADC_cumul + (v_shifted * v_shifted);
	ADC_cumul_count = ADC_cumul_count + 1;
#else
	int adc_reading = 0;
	if (adc_oneshot_read(adc1_handle, ADC_Channel, &adc_reading) == ESP_OK)
	{
		v_shifted = adc_reading - ADC_ZERO;
		ADC_cumul = ADC_cumul + (v_shifted * v_shifted);
		ADC_cumul_count = ADC_cumul_count + 1;
	}
#endif

	Compute_Mean_Wave();
}

/**
 * Initialisation du relais.
 * gpio : values of gpio
 */
bool ADC_Initialize_OneShot(uint8_t gpio)
{
#if defined(ESP8266) | defined(KEYBOARD_ESP32_ARDUINO)
	ADC_gpio = gpio;
	pinMode(gpio, INPUT);
#else
//	GPIO_NUM_toADCChannel(pin, &Keyboard_Channel);
//	InitADC(Keyboard_Channel, &adc1_handle);

	adc_unit_t adc_unit;
	adc_oneshot_io_to_channel(gpio, &adc_unit, &ADC_Channel);

	if (ADC_Channel == ADC_CHANNEL_3)
		printf("ADC_CHANNEL_3 OK \n");

	//-------------ADC1 Config---------------
	adc_oneshot_chan_cfg_t config = { // @suppress("Type cannot be resolved")
			.atten = ADC_ATTEN_DB_12,
			.bitwidth = ADC_BITWIDTH_DEFAULT, // default width is max supported width
	};
	adc_oneshot_config_channel(adc1_handle, ADC_Channel, &config);

	//-------------ADC1 Calibration Init-----
  // Only to have voltage

#endif

	Timer_Tore = timerBegin(1000000); // Fixe la fréquence à 1 MHz => tick de 1 us
	timerAttachInterrupt(Timer_Tore, &onTimerTore);

	ADC_Status = adcOnseShot;
	return true;
}

// ********************************************************************************
// Initialization functions in Continuous mode
// ********************************************************************************

void IRAM_ATTR ADC_Complete()
{
	static adc_continuous_data_t *ADC_result = NULL;
	if (analogContinuousRead(&ADC_result, 0))
	{
		int v_shifted = ADC_result[0].avg_read_raw - ADC_ZERO;
		ADC_cumul = ADC_cumul + (v_shifted * v_shifted);
		ADC_cumul_count = ADC_cumul_count + 1;
	}
	Compute_Mean_Wave();
}

bool ADC_Initialize_Continuous(uint8_t gpio)
{
#define CONVERSIONS_PER_PIN 5
	uint8_t adc_pins[] = {gpio};
	ADC_Status = adcContinuous;

	// Optional for ESP32: Set the resolution to 9-12 bits (default is 12 bits)
	analogContinuousSetWidth(12);

	// Optional: Set different attenaution (default is ADC_11db)
	analogContinuousSetAtten(ADC_11db);

	// Setup ADC Continuous with following input:
	// array of pins, count of the pins, how many conversions per pin in one cycle will happen, sampling frequency, callback function
	return analogContinuous(adc_pins, 1, CONVERSIONS_PER_PIN, 20000, &ADC_Complete);
}

// ********************************************************************************
// Mean wave computation with Zero cross
// ********************************************************************************

#if MEAN_WAVE_CUMUL == true
void IRAM_ATTR Compute_Mean_Wave(void)
{
	static bool ZC_First_Top = false;

	if (xSemaphoreTake(topZC_Semaphore, 0) == pdTRUE)
	{
		if (!ZC_First_Top)
			// First 1/2 alternance
			ZC_First_Top = true;
		else
		{
			// Cumul over one period
			current_cumul_int = current_cumul_int + ADC_cumul;
			ADC_cumul_count_int = ADC_cumul_count_int + ADC_cumul_count;
			ADC_cumul = 0;
			ADC_cumul_count = 0;

			// We cumul 5 periods
			current_cumul_count = current_cumul_count + 1;
			if (current_cumul_count == PERIOD_COUNT)
			{
				portENTER_CRITICAL(&ADC_Mux);
				raw_current_int = current_cumul_int;
				raw_current_count = ADC_cumul_count_int;
				portEXIT_CRITICAL(&ADC_Mux);
				current_cumul_int = 0;
				ADC_cumul_count_int = 0;
				current_cumul_count = 0;
			}
			ZC_First_Top = false;
		}
	}
}
#else
void IRAM_ATTR Compute_Mean_Wave(void)
{
	static bool ZC_First_Top = false;

	if (xSemaphoreTake(topZC_Semaphore, 0) == pdTRUE)
	{
		if (!ZC_First_Top)
			// First 1/2 alternance
			ZC_First_Top = true;
		else
		{
			// Cumul over one period
			current_cumul = current_cumul + ADC_cumul / ADC_cumul_count;
			ADC_cumul = 0;
			ADC_cumul_count = 0;

			// We cumul 5 periods
			current_cumul_count = current_cumul_count + 1;
			if (current_cumul_count == PERIOD_COUNT)
			{
				portENTER_CRITICAL(&ADC_Mux);
				raw_current = current_cumul / PERIOD_COUNT;
				portEXIT_CRITICAL(&ADC_Mux);
				current_cumul = 0;
				current_cumul_count = 0;
			}
			ZC_First_Top = false;
		}
	}
}
#endif

void ADC_Begin(void)
{
	if (ADC_Status == adcOnseShot)
	{
		timerRestart(Timer_Tore);
		timerAlarm(Timer_Tore, Timer_Read_Delay, true, 0);
	}
	else
		if (ADC_Status == adcContinuous)
			// Start ADC Continuous conversions
			analogContinuousStart();
}

double ADC_GetTalemaCurrent(void)
{
	double Talema_current;
	portENTER_CRITICAL(&ADC_Mux);
#if MEAN_WAVE_CUMUL == true
	Talema_current = 0.0125 * sqrt((double)raw_current_int / raw_current_count) - 0.018157097;
#else
	Talema_current = 0.0125 * sqrt(raw_current) - 0.018157097;
#endif
	portEXIT_CRITICAL(&ADC_Mux);
	return Talema_current;
}

// ********************************************************************************
// Basic Task function to get data in continuous mode
// ********************************************************************************
/**
 * A basic Task to get data in continuous mode
 */
#ifdef ADC_USE_TASK

void ADC_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("ADC_Task");
	for (EVER)
	{
		Compute_Mean_Wave();
		END_TASK_CODE();
	}
}
#endif

// ********************************************************************************
// End of file
// ********************************************************************************
