#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#ifdef ESP32
#include "hal/adc_types.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_continuous.h>
#endif

#include "Arduino.h"
#include <stdint.h>

// The adc value of the zero of the sin wave
#define ADC_ZERO	1902

// To create a basic task to compute mean every 1 ms
#ifdef ADC_USE_TASK
#define ADC_DATA_TASK(start)	{(start), "ADC_Task", 1024, 5, 1, CoreAny, ADC_Task_code}
void ADC_Task_code(void *parameter);
#endif

bool ADC_Initialize_OneShot(uint8_t gpio);
bool ADC_Initialize_Continuous(uint8_t gpio);
void ADC_Begin(void);

// Get Talema rms current
double ADC_GetTalemaCurrent(void);
