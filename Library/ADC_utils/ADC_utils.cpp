#include "ADC_utils.h"

#if defined(ESP8266) | defined(ADC_USE_ARDUINO)
uint8_t ADC_gpio[2];
#else

//#include "hal/adc_types.h"
#include "soc/soc_caps.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_continuous.h>

adc_oneshot_unit_handle_t adc1_handle; // @suppress("Type cannot be resolved")
adc_channel_t ADC_Channel[2];
#endif

// IRAM_ATTR is necessary for timer or callback
#ifdef ADC_USE_TASK
#define ADC_INTO_IRAM
#else
#define ADC_INTO_IRAM IRAM_ATTR
#endif

typedef enum
{
	adcUndef,
	adcOnseShot,
	adcContinuous
} ADC_Status_typedef;

ADC_Status_typedef ADC_Status = adcUndef;

// Read prototype function
typedef bool (*ADC_Read_Mode_Function)(int*, int*);

// Pointer to the read function against oneshot or continuous
ADC_Read_Mode_Function ADC_Read_Mode = NULL;

// Do code prototype function
typedef void (*Do_Code_action)(void);
Do_Code_action Action = NULL;

// Timer to get data in OneShot mode
#ifndef ADC_USE_TASK
hw_timer_t *Timer_Tore = NULL;
uint32_t Timer_Read_Delay = 100;	// 0.1 ms
#endif

// The zero of the ADC for mean wave
int ADC_zero = ADC_ZERO;

// Two channel used
volatile bool Use_Two_Channel = false;

// A global variable to know if ADC has been initialized
volatile bool ADC_Initialized = false;

// Critical section for ADC
static portMUX_TYPE ADC_Tore_Mux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE ADC_Keyboard_Mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * All next variables are for Tore computation
 */

// We do a mean over PERIOD_COUNT period (100 ms)
#define PERIOD_COUNT	5

// ZC semaphore from Cirrus
extern volatile SemaphoreHandle_t topZC_Semaphore;

// To avoid division
#define MEAN_WAVE_CUMUL	false

// ADC cumul
volatile int ADC_cumul = 0;
volatile int ADC_cumul_count = 0;
volatile int ADC_cumul_count_int = 0;
volatile int ADC_Value0 = 0;

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
// Get raw ADC values
// do_Code: get raw value for first channel and compute mean wave (sigma) for second channel if exist
// do_Code_Zero: get raw value for first channel and compute mean for second channel if exist
// ********************************************************************************

void ADC_INTO_IRAM do_Code(void)
{
	int result0, result1;
	if (ADC_Read_Mode(&result0, &result1))
	{
		portENTER_CRITICAL(&ADC_Keyboard_Mux);
		ADC_Value0 = result0;
		portEXIT_CRITICAL(&ADC_Keyboard_Mux);

		if (Use_Two_Channel)
		{
			int v_shifted = result1 - ADC_zero;
			ADC_cumul = ADC_cumul + (v_shifted * v_shifted);
			ADC_cumul_count = ADC_cumul_count + 1;
		}
	}

	if (Use_Two_Channel)
	  Compute_Mean_Wave();
}

void ADC_INTO_IRAM do_Code_Zero(void)
{
	int result0, result1;
	if (ADC_Read_Mode(&result0, &result1))
	{
		portENTER_CRITICAL(&ADC_Keyboard_Mux);
		ADC_Value0 = result0;
		portEXIT_CRITICAL(&ADC_Keyboard_Mux);
		if (Use_Two_Channel)
		{
			portENTER_CRITICAL(&ADC_Tore_Mux);
			ADC_cumul = ADC_cumul + result1;
			ADC_cumul_count = ADC_cumul_count + 1;
			portEXIT_CRITICAL(&ADC_Tore_Mux);
		}
	}
}

#ifdef ADC_USE_TASK
void task_function(void*)
{
	for (;;)
	{
		Action();
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

#endif

// ********************************************************************************
// Initialization functions in Oneshot mode
// ********************************************************************************

/**
 * Read function of all channels in OneShot mode
 */
bool ADC_INTO_IRAM ADC_Read_OneShot(int *result_ch0, int *result_ch1)
{
#if defined(ESP8266) | defined(ADC_USE_ARDUINO)
	*result_ch0 = analogRead(ADC_gpio[0]);
	if (Use_Two_Channel)
	  *result_ch1 = analogRead(ADC_gpio[1]);
	return true;
#else
	bool success = (adc_oneshot_read(adc1_handle, ADC_Channel[0], result_ch0) == ESP_OK); // @suppress("Invalid arguments")
	if (success && Use_Two_Channel)
		success = (adc_oneshot_read(adc1_handle, ADC_Channel[1], result_ch1) == ESP_OK); // @suppress("Invalid arguments")
	return success;
#endif
}

/**
 * Initialize ADC in OnseShot mode
 * The first gpio is used in the ADC_Read0 function. This raw value, no treatment.
 * Set action_zero to true when you want to determine the zero for the second channel
 */
bool ADC_Initialize_OneShot(std::initializer_list<uint8_t> gpios, bool action_zero)
{
#if defined(ESP8266) | defined(ADC_USE_ARDUINO)
	uint8_t i = 0;
	for (auto gpio : gpios)
	{
		ADC_gpio[i] = gpio;
		pinMode(gpio, INPUT);
		i++;
	}

#else
	//-------------ADC1 Init---------------
	adc_oneshot_unit_init_cfg_t init_config1 = { // @suppress("Type cannot be resolved")
			.unit_id = ADC_UNIT_1,
			.clk_src = ADC_RTC_CLK_SRC_DEFAULT, // = ADC_RTC_CLK_SRC_RC_FAST
			.ulp_mode = ADC_ULP_MODE_DISABLE, // ADC_ULP_MODE_RISCV (for S2+ version)
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle)); // @suppress("Invalid arguments")

	//-------------ADC1 Config-------------
	adc_oneshot_chan_cfg_t config = { // @suppress("Type cannot be resolved")
			.atten = ADC_ATTEN_DB_12,
			.bitwidth = ADC_BITWIDTH_DEFAULT, // default width is max supported width
	};

	uint8_t i = 0;
	adc_unit_t adc_unit;
	for (auto gpio : gpios)
	{
		adc_oneshot_io_to_channel(gpio, &adc_unit, &ADC_Channel[i]); // @suppress("Invalid arguments")
		adc_oneshot_config_channel(adc1_handle, ADC_Channel[i], &config); // @suppress("Invalid arguments")
		i++;
	}

	//-------------ADC1 Calibration Init-----
	// Only to have voltage

#endif

	ADC_Status = adcOnseShot;
	ADC_Read_Mode = &ADC_Read_OneShot;
	Use_Two_Channel = (gpios.size() == 2);

	if (action_zero)
	  Action = &do_Code_Zero;
	else
		Action = &do_Code;
	ADC_Initialized = true;

#ifndef ADC_USE_TASK
	Timer_Tore = timerBegin(1000000); // Fixe la fréquence à 1 MHz => tick de 1 us
	timerAttachInterrupt(Timer_Tore, Action);
#endif

	return true;
}

// ********************************************************************************
// Initialization functions in Continuous mode
// ********************************************************************************

/**
 * Read function of all channels in Continuous mode
 */
bool ADC_INTO_IRAM ADC_Read_Continuous(int *result_ch0, int *result_ch1)
{
	adc_continuous_data_t *ADC_result = NULL;
	bool success = analogContinuousRead(&ADC_result, 0);
	if (success)
	{
		*result_ch0 = ADC_result[0].avg_read_raw;
		if (Use_Two_Channel)
			*result_ch1 = ADC_result[1].avg_read_raw;
	}
	return success;
}

/**
 * Initialize ADC in continuous mode
 * The first gpio is used in the ADC_Read0 function. This raw value, no treatment.
 * Set action_zero to true when you want to determine the zero  for the second channel
 */
bool ADC_Initialize_Continuous(std::initializer_list<uint8_t> gpios, bool action_zero)
{
#define CONVERSIONS_PER_PIN 64
	ADC_Status = adcContinuous;
	ADC_Read_Mode = &ADC_Read_Continuous;
	Use_Two_Channel = (gpios.size() == 2);
	if (action_zero)
	  Action = &do_Code_Zero;
	else
		Action = &do_Code;
	ADC_Initialized = true;

	// Optional for ESP32: Set the resolution to 9-12 bits (default is 12 bits)
	analogContinuousSetWidth(12);

	// Optional: Set different attenaution (default is ADC_11db)
	analogContinuousSetAtten(ADC_11db);

	// Setup ADC Continuous with following input:
	// array of pins, count of the pins, how many conversions per pin in one cycle will happen, sampling frequency, callback function
#ifdef ADC_USE_TASK
	return analogContinuous((uint8_t *)data(gpios), gpios.size(), CONVERSIONS_PER_PIN, 20000, NULL); // 80000
#else
	return analogContinuous((uint8_t*) data(gpios), gpios.size(), CONVERSIONS_PER_PIN, 80000, Action);
#endif
}

/**
 * Start ADC reading
 * The zero parameter is used for the second channel to compute rms current
 */
void ADC_Begin(int zero)
{
	ADC_zero = zero;

	if (ADC_Status == adcUndef)
		return;

	if (ADC_Status == adcOnseShot)
	{
#ifndef ADC_USE_TASK
		timerRestart(Timer_Tore);
		timerAlarm(Timer_Tore, Timer_Read_Delay, true, 0);
#endif
	}
	else
		if (ADC_Status == adcContinuous)
		{
			// Start ADC Continuous conversions
			analogContinuousStart();
		}
#ifdef ADC_USE_TASK
	xTaskCreatePinnedToCore(task_function, NULL, 4096 * 2, NULL, 6, NULL, 0); // ESP_TASK_PRIO_MAX / 2
#endif
}

uint16_t ADC_Read0(void)
{
	uint32_t value;
	portENTER_CRITICAL(&ADC_Keyboard_Mux);
	value = (uint16_t) ADC_Value0;
	portEXIT_CRITICAL(&ADC_Keyboard_Mux);
	return value;
}

// ********************************************************************************
// Mean wave computation with Zero cross
// ********************************************************************************

#if MEAN_WAVE_CUMUL == true
void ADC_INTO_IRAM Compute_Mean_Wave(void)
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
				portENTER_CRITICAL(&ADC_Tore_Mux);
				raw_current_int = current_cumul_int;
				raw_current_count = ADC_cumul_count_int;
				portEXIT_CRITICAL(&ADC_Tore_Mux);
				current_cumul_int = 0;
				ADC_cumul_count_int = 0;
				current_cumul_count = 0;
			}
			ZC_First_Top = false;
		}
	}
}
#else
void ADC_INTO_IRAM Compute_Mean_Wave(void)
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
			if (ADC_cumul_count != 0)
			{
				current_cumul = current_cumul + ADC_cumul / ADC_cumul_count;
				current_cumul_count = current_cumul_count + 1;
			}
			ADC_cumul = 0;
			ADC_cumul_count = 0;

			// We cumul PERIOD_COUNT periods
			if (current_cumul_count == PERIOD_COUNT)
			{
				portENTER_CRITICAL(&ADC_Tore_Mux);
				raw_current = current_cumul / PERIOD_COUNT;
				portEXIT_CRITICAL(&ADC_Tore_Mux);
				current_cumul = 0;
				current_cumul_count = 0;
			}
			ZC_First_Top = false;
		}
	}
}
#endif

float ADC_GetTalemaCurrent(void)
{
	int raw;
	portENTER_CRITICAL(&ADC_Tore_Mux);
#if MEAN_WAVE_CUMUL == true
	if (raw_current_count != 0)
	  raw = (double)raw_current_int / raw_current_count;
	else
		raw = 0;
#else
	raw = raw_current;
#endif
	portEXIT_CRITICAL(&ADC_Tore_Mux);

	float Talema_current;
#if MEAN_WAVE_CUMUL == true
	Talema_current = 0.0125 * sqrt(raw) - 0.008;
#else
	Talema_current = 0.0125 * sqrt(raw); //0.018157097
#endif

	return Talema_current;
}

float ADC_GetZero(uint32_t *count)
{
	float zero = 0;
	portENTER_CRITICAL(&ADC_Tore_Mux);
	if (ADC_cumul_count != 0)
		zero = (float) ADC_cumul / ADC_cumul_count;
	*count = ADC_cumul_count;
	ADC_cumul = 0;
	ADC_cumul_count = 0;
	portEXIT_CRITICAL(&ADC_Tore_Mux);
	return zero;
}

// ********************************************************************************
// End of file
// ********************************************************************************
