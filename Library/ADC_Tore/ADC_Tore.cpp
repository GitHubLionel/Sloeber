#include "ADC_Tore.h"

#if defined(ESP8266) | defined(KEYBOARD_ESP32_ARDUINO)
uint8_t ADC_gpio;
#else
//#include "driver/adc.h"
#include "esp_adc_cal.h"
//#include "esp_adc/adc_cali.h"
//#include "esp_adc/adc_cali_scheme.h"
extern adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t ADC_Channel;
static esp_adc_cal_characteristics_t *adc_chars;

#define NO_OF_SAMPLES   64
#define DEFAULT_VREF    1100
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;
//static const adc_channel_t channel = ADC_CHANNEL_3;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#endif


hw_timer_t *Timer_Tore = NULL;
uint32_t Read_Delay = 1000;	// 1 ms

int cumul2 = 0;
uint32_t count_ADC = 0;
uint32_t count_top = 0;
const uint8_t count_period = 20; // 10 periodes

extern volatile SemaphoreHandle_t topZC_Semaphore;
double current2 = 0.0;
double talema = 0.0;

volatile SemaphoreHandle_t ADC_Current_Semaphore;

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
 * Timer callback
 * Read continuously ADC
 */
void IRAM_ATTR onTimerTore(void)
{
  int raw = 0;
#if defined(ESP8266) | defined(KEYBOARD_ESP32_ARDUINO)
  raw = analogRead(ADC_gpio) - 1887;
#else
	int adc_reading = 0;
//	//Multisampling
//	for (int i = 0; i < NO_OF_SAMPLES; i++)
//	{
////			adc_reading += adc1_get_raw((adc1_channel_t)ADC_Channel);
//		int adc_raw;
//			adc_oneshot_read(adc1_handle, ADC_Channel, &adc_raw);
//			adc_reading += adc_raw;
//	}
//	adc_reading /= NO_OF_SAMPLES;
	adc_oneshot_read(adc1_handle, ADC_Channel, &adc_reading);
	raw = adc_reading; // esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
#endif

	count_ADC++;
  cumul2 += raw * raw;

  if (xSemaphoreTake(topZC_Semaphore, 0) == pdTRUE)
  {
  	count_top++;
  	if (count_top == count_period)
  	{
  		current2 = (double) cumul2 / count_ADC;
  		count_top = 0;
  		cumul2 = 0;
  		talema =  (0.0126 * sqrt(current2) - 0.0586 + 0.03);
  		xSemaphoreGiveFromISR(ADC_Current_Semaphore, NULL);
  	}
  }
}

/**
 * Initialisation du relais.
 * gpio : values of gpio
 */
bool ADC_Initialize(uint8_t gpio)
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

	adc_oneshot_chan_cfg_t config = { // @suppress("Type cannot be resolved")
				.atten = ADC_ATTEN_DB_12,
				.bitwidth = ADC_BITWIDTH_DEFAULT, // default width is max supported width
		};
		adc_oneshot_config_channel(adc1_handle, ADC_Channel, &config);

		//Characterize ADC
		adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
		esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

//	adc1_config_channel_atten(Keyboard_Channel, atten);
#endif

	Timer_Tore= timerBegin(1000000); // Fixe la fréquence à 1 MHz => tick de 1 us
	timerAttachInterrupt(Timer_Tore, &onTimerTore);

	ADC_Current_Semaphore = xSemaphoreCreateBinary();

	return true;
}

void ADC_Begin(void)
{
	timerRestart(Timer_Tore);
	timerAlarm(Timer_Tore, Read_Delay, true, 0);
}

double ADC_GetCurrent(void)
{

	return talema;
//	  return sqrt(current2);
}


// ********************************************************************************
// End of file
// ********************************************************************************
