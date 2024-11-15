#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include <stdint.h>
#include <initializer_list>

// The adc value of the zero of the sin wave
#define ADC_ZERO	1900  // 1889		// 1898

bool ADC_Initialize_OneShot(std::initializer_list<uint8_t> gpios);
bool ADC_Initialize_Continuous(std::initializer_list<uint8_t> gpios);
void ADC_Begin(void);
uint16_t ADC_Read0(void);

// Get Talema rms current
double ADC_GetTalemaCurrent(void);
double ADC_GetZero(uint32_t *count);
