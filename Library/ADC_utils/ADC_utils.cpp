#include "ADC_utils.h"
#include "Debug_utils.h"

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

// Timer to get data in OneShot mode when we don't use task
#ifndef ADC_USE_TASK
hw_timer_t *Timer_Tore = NULL;
uint32_t Timer_Read_Delay = 200;	// 0.2 ms
#endif

// The action selected
ADC_Action_Enum Current_Action;

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

// To debug the count of the cumul
//#define DEBUG_CUMUL_COUNT

// ADC value for the first channel used for keyboard
volatile int ADC_Value0 = 0;
// ADC value for the second channel used for Talema
volatile int ADC_Value1 = 0;
// Just for test
volatile int ADC_Error_count = 0;

// ADC cumul
volatile int ADC_cumul = 0;
volatile int ADC_sinus_cumul = 0;
volatile int ADC_cumul_count = 0;

#ifdef DEBUG_CUMUL_COUNT
volatile int ADC_cumul_count_cumul = 0;
volatile int ADC_cumul_count_total = 0;
#endif

// ZC operation
#if MEAN_WAVE_CUMUL == true
volatile int cumul_count_int = 0;
volatile int current_cumul_int = 0;
volatile int sinus_cumul_int = 0;
volatile int raw_current_int = 0;
volatile int raw_power_int = 0;
volatile int raw_count_int = 1; // To avoid division by zero at beginning
#else
volatile int current_cumul = 0;
volatile int raw_current = 0;
volatile int power_cumul = 0;
volatile int raw_power = 0;
#endif

volatile int period_cumul_count = 0;

// Sinusoïde de référence
#define SINUS_PRECISION	1000.0
int *sinus, *p_sinus;  // Tableau du sinus et pointeur de parcourt du tableau
bool Fill_Sinus_Ref(void);

// Function for debug message, may be redefined elsewhere
//void __attribute__((weak)) print_debug(const char *mess, bool ln = true)
//{
//	// Just to avoid compile warning
//	(void) mess;
//	(void) ln;
//}

// Mean computation
void Compute_Mean_Wave(void);
bool SetActionCode(ADC_Action_Enum action);

// ********************************************************************************
// Get raw ADC values
// do_Code_Raw: get raw value for first channel and second channel if exist
// do_Code_Zero: get raw value for first channel and compute mean for second channel if exist
// do_Code_Sigma: get raw value for first channel and compute mean wave (sigma) for second channel if exist
// ********************************************************************************

void ADC_INTO_IRAM do_Code_Raw(void)
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
			ADC_Value1 = result1;
			portEXIT_CRITICAL(&ADC_Tore_Mux);
		}
	}
	else
		ADC_Error_count = ADC_Error_count + 1;
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
	else
		ADC_Error_count = ADC_Error_count + 1;
}

void ADC_INTO_IRAM do_Code_Sigma(void)
{
	int result0, result1, v_shifted;
	if (ADC_Read_Mode(&result0, &result1))
	{
		portENTER_CRITICAL(&ADC_Keyboard_Mux);
		ADC_Value0 = result0;
		portEXIT_CRITICAL(&ADC_Keyboard_Mux);

		if (Use_Two_Channel)
		{
			v_shifted = result1 - ADC_zero;
			ADC_cumul = ADC_cumul + (v_shifted * v_shifted);
			ADC_sinus_cumul = ADC_sinus_cumul + (v_shifted * (*p_sinus));
			ADC_cumul_count = ADC_cumul_count + 1;
			p_sinus++;
		}
	}

	// Not in ADC_Read_Mode to make the computation even if ADC read failed
	if (Use_Two_Channel)
		Compute_Mean_Wave();
}

bool SetActionCode(ADC_Action_Enum action)
{
	switch (action)
	{
		case adc_Raw:
			Action = &do_Code_Raw;
			break;
		case adc_Zero:
			Action = &do_Code_Zero;
			break;
		case adc_Sigma:
			Action = &do_Code_Sigma;
			break;
		default:
			return false;
	};
	Current_Action = action;
	return true;
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
bool ADC_Initialize_OneShot(std::initializer_list<uint8_t> gpios, ADC_Action_Enum action)
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

	if (!SetActionCode(action))
		return false;

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
bool ADC_Initialize_Continuous(std::initializer_list<uint8_t> gpios, ADC_Action_Enum action)
{
#define CONVERSIONS_PER_PIN 64
	ADC_Status = adcContinuous;
	ADC_Read_Mode = &ADC_Read_Continuous;
	Use_Two_Channel = (gpios.size() == 2);

	if (!SetActionCode(action))
		return false;

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
 * If we use the two channel, then :
 * - The zero parameter is used to determine the zero of the ADC that correspond to the zero current
 * - The reference sinus wave is used to compute the power
 */
void ADC_Begin(int zero)
{
	ADC_zero = zero;

	if (Use_Two_Channel && (Current_Action == adc_Sigma))
	{
		if (!Fill_Sinus_Ref())
			return;
	}

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
	xTaskCreatePinnedToCore(task_function, NULL, 4096 * 2, NULL, ESP_TASK_PRIO_MAX / 2, NULL, 0); // ESP_TASK_PRIO_MAX / 2
#endif
}

int ADC_Read0(void)
{
	int value;
	portENTER_CRITICAL(&ADC_Keyboard_Mux);
	value = ADC_Value0;
	portEXIT_CRITICAL(&ADC_Keyboard_Mux);
	return value;
}

int ADC_Read1(void)
{
	int value;
	portENTER_CRITICAL(&ADC_Tore_Mux);
	value = ADC_Value1;
	portEXIT_CRITICAL(&ADC_Tore_Mux);
	return value;
}

int ADC_Get_Error(void)
{
	return ADC_Error_count;
}

// ********************************************************************************
// Mean wave computation with Zero cross
// ********************************************************************************

/**
 * Fill reference sinusoïde array for a voltage of 1 V
 * We multiply by SINUS_PRECISION to have integer precision when we convert float to int
 */
bool Fill_Sinus_Ref(void)
{
	const float SQRT2 = 1.41421356;
	const float TwoPI = 6.28318531;
	uint16_t period;
	uint16_t sampling;

#ifdef ADC_USE_TASK
	period = 20; // Task every 1 ms
#else
	// 100 for Timer_Read_Delay = 200, 200 for Timer_Read_Delay = 100
	period = 20000 / Timer_Read_Delay;
#endif

	sampling = period + 2;  // + 2 si on dépasse un peu, histoire de se donner un peu de marge !

	sinus = (int*) malloc(sampling * sizeof(int));
	if (sinus == NULL)
	{
		print_debug("Sinus allocation error.\r\n");
		p_sinus = NULL;
		return false;
	}
	else
	{
		for (int i = 0; i < sampling; i++)
		{
			sinus[i] = (int) (SQRT2 * sin(TwoPI * (float) i / (float) period) * SINUS_PRECISION);
//			print_debug(sinus[i]);
		}
		p_sinus = &sinus[0];
	}
	return true;
}

#if MEAN_WAVE_CUMUL == true
void ADC_INTO_IRAM Compute_Mean_Wave(void)
{
	static bool ZC_First_Top = false;

	if (xSemaphoreTake(topZC_Semaphore, 0) == pdTRUE)
	{
		if (!ZC_First_Top)
		{
			// First 1/2 alternance
			ZC_First_Top = true;
			p_sinus = &sinus[0];
		}
		else
		{
			// Cumul over one period
			current_cumul_int = current_cumul_int + ADC_cumul;
			sinus_cumul_int = sinus_cumul_int + ADC_sinus_cumul;
			cumul_count_int = cumul_count_int + ADC_cumul_count;
#ifdef DEBUG_CUMUL_COUNT
			ADC_cumul_count_cumul = ADC_cumul_count_cumul + ADC_cumul_count;
#endif
			ADC_cumul = 0;
			ADC_sinus_cumul = 0;
			ADC_cumul_count = 0;

			// We cumul PERIOD_COUNT periods
			period_cumul_count = period_cumul_count + 1;
			if (period_cumul_count == PERIOD_COUNT)
			{
				portENTER_CRITICAL(&ADC_Tore_Mux);
				raw_current_int = current_cumul_int;
				raw_power_int = sinus_cumul_int;
				raw_count_int = cumul_count_int;
				portEXIT_CRITICAL(&ADC_Tore_Mux);
				current_cumul_int = 0;
				sinus_cumul_int = 0;
				cumul_count_int = 0;
				period_cumul_count = 0;
#ifdef DEBUG_CUMUL_COUNT
				ADC_cumul_count_total = ADC_cumul_count_cumul;
				ADC_cumul_count_cumul = 0;
#endif
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
		{
			// First 1/2 alternance
			ZC_First_Top = true;
			p_sinus = &sinus[0];
		}
		else
		{
			// Cumul over one period
			if (ADC_cumul_count != 0)
			{
				current_cumul = current_cumul + ADC_cumul / ADC_cumul_count;
				power_cumul = power_cumul + ADC_sinus_cumul / ADC_cumul_count;
				period_cumul_count = period_cumul_count + 1;
			}
#ifdef DEBUG_CUMUL_COUNT
			ADC_cumul_count_cumul = ADC_cumul_count_cumul + ADC_cumul_count;
#endif
			ADC_cumul = 0;
			ADC_sinus_cumul = 0;
			ADC_cumul_count = 0;

			// We cumul PERIOD_COUNT periods
			if (period_cumul_count == PERIOD_COUNT)
			{
				portENTER_CRITICAL(&ADC_Tore_Mux);
				raw_current = current_cumul / PERIOD_COUNT;
				raw_power = power_cumul / PERIOD_COUNT;
				portEXIT_CRITICAL(&ADC_Tore_Mux);
				current_cumul = 0;
				power_cumul = 0;
				period_cumul_count = 0;
#ifdef DEBUG_CUMUL_COUNT
				ADC_cumul_count_total = ADC_cumul_count_cumul;
				ADC_cumul_count_cumul = 0;
#endif
			}
			ZC_First_Top = false;
		}
	}
}
#endif

float ADC_GetTalemaCurrent()
{
	portENTER_CRITICAL(&ADC_Tore_Mux);
#if MEAN_WAVE_CUMUL == true
	float raw;
	if (raw_count_int != 0)
	  raw = (double)raw_current_int / raw_count_int;
	else
		raw = 0;
#else
	int raw = raw_current;
#endif
	portEXIT_CRITICAL(&ADC_Tore_Mux);
#ifdef DEBUG_CUMUL_COUNT
	print_debug(ADC_cumul_count_total);
#endif

	return 0.0125 * sqrt(raw);
}

// We assume that power is always positive
float ADC_GetTalemaPower()
{
	portENTER_CRITICAL(&ADC_Tore_Mux);
#if MEAN_WAVE_CUMUL == true
	float raw;
	if (raw_count_int != 0)
	  raw = (double)abs(raw_power_int) / raw_count_int;
	else
		raw = 0;
#else
	int raw = abs(raw_power);
#endif
	portEXIT_CRITICAL(&ADC_Tore_Mux);
#ifdef DEBUG_CUMUL_COUNT
	print_debug(ADC_cumul_count_total);
#endif

	return 0.0125 * (float) raw / SINUS_PRECISION;  // 0.0131
}
/**
 * Return the zero of the ADC and the number of accumulation of data
 */
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
