#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>

// The adc value of the zero of the sin wave for rms current on channel 2
#define ADC_ZERO	1900  // typical value

bool ADC_Initialize_OneShot(std::initializer_list<uint8_t> gpios, bool action_zero = false);
bool ADC_Initialize_Continuous(std::initializer_list<uint8_t> gpios, bool action_zero = false);
void ADC_Begin(int zero = ADC_ZERO);
uint16_t ADC_Read0(void);

// Get Talema rms current
float ADC_GetTalemaCurrent();
float ADC_GetTalemaPower();
float ADC_GetZero(uint32_t *count);
