/*
 * PCF8574_utils.h
 */

#ifndef PCF8574_UTILS_H_
#define PCF8574_UTILS_H_

#include "Arduino.h"
#include <initializer_list>
#include "PCF8574.h"

typedef void (*PCF8574_int_cb)(void);

// Mode definition of alls 8 pins P0 .. P7
// Mode can be: INPUT or OUTPUT
typedef std::initializer_list<uint8_t> PCF8574_Modes;

/**
 * Task definition
 * Check every 20 ms if PCF8574 input pins has changed.
 * If true, call PCF8574_Keyboard_Action() callback function
 */
#define PCF8574_KEY_TASK	{condCreate, "PCF8574_KEY_Task", 4096, 10, 20, CoreAny, PCF8574_KEY_Task_code}
void PCF8574_KEY_Task_code(void *parameter);

bool PCF8574_Initialization(uint8_t address, uint8_t interruptPin, PCF8574_Modes pinModes, PCF8574_int_cb fonc = NULL);
void PCF8574_SetPinLed(PCF8574_Modes pinLeds);

void PCF8574_CircularToggleLed(bool use_output_list);
void PCF8574_UpdateLed(uint8_t ledpin, bool state);

#endif /* PCF8574_UTILS_H_ */
