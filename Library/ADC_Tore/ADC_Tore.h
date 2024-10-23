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

bool ADC_Initialize(uint8_t gpio);

void ADC_Begin(void);
double ADC_GetCurrent(void);
