#pragma once
#include "Arduino.h"
#include <stdint.h>

#define RELAY_MAX	5

bool Relay_Initialize(uint8_t nbRelay, const uint8_t gpio[]);

void Set_Relay_State(uint8_t n, bool state);
bool Get_Relay_State(uint8_t n);
