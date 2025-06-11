/*
 * PCF8574_utils.cpp
 */

#include "PCF8574_utils.h"
#include "Tasks_utils.h"

// Function interrupt
volatile SemaphoreHandle_t PCF8574_Semaphore;
void IRAM_ATTR keyPressedOnPCF8574()
{
	// Just signal the click
	xSemaphoreGiveFromISR(PCF8574_Semaphore, NULL);
}

// PCF8574 instance
static PCF8574 *pcf8574 = NULL;

// List of INPUT Mode
static uint8_t input_Modes[8] = {};
static uint8_t input_count = 0;

// List of OUTPUT Mode
static uint8_t output_Modes[8] = {};
static uint8_t output_count = 0;

// List of led pin
static uint8_t pin_led[8] = {};
static uint8_t led_count = 0;

/**
 * Initialization of the PCF8574
 * @Param address: the I2C address of the PCF8574 (for example 0x20, 0x38)
 * @Param interruptPin: the GPIO where the interrupt pin is connected
 * @Param pinModes: list of the modes (INPUT or OUTPUT) of the 8 pins of the PCF8574
 * @Param fonc: the callback function called when an input pin change state
 */
bool PCF8574_Initialization(uint8_t address, uint8_t interruptPin, PCF8574_Modes pinModes, PCF8574_int_cb fonc)
{
	if (fonc)
		pcf8574 = new PCF8574(address, interruptPin, fonc);
	else
		pcf8574 = new PCF8574(address, interruptPin, keyPressedOnPCF8574);

	// Create the semaphore for the input
	PCF8574_Semaphore = xSemaphoreCreateBinary();

	uint8_t i = 0;
	for (auto mode : pinModes)
	{
		pcf8574->pinMode(i, mode);
		if (mode == INPUT)
		{
			input_Modes[input_count] = i;
			input_count++;
		}
		if (mode == OUTPUT)
		{
			output_Modes[output_count] = i;
			output_count++;
		}
		i++;
	}

	return pcf8574->begin();
}

/**
 * Definition of OUTPUT who are led
 * @Param pinLeds: list of booleans indicating if the pin is an output led (true) or no (false)
 */
void PCF8574_SetPinLed(PCF8574_Modes pinLeds)
{
	uint8_t i = 0;
	for (auto led : pinLeds)
	{
		if (led)
		{
			pin_led[led_count] = i;
			led_count++;
		}
		i++;
	}
}

/**
 * Toggle leds in a circular way.
 * @Param use_output_list: if true use all the pins who are OUTPUT else use the list
 * of pins defined by PCF8574_SetPinLed() function.
 */
void PCF8574_CircularToggleLed(bool use_output_list)
{
	static int toggleLed = 0;

	if (use_output_list)
	{
		pcf8574->digitalWrite(output_Modes[toggleLed], HIGH);
		if (++toggleLed == output_count)
			toggleLed = 0;
		pcf8574->digitalWrite(output_Modes[toggleLed], LOW);
	}
	else
	{
		pcf8574->digitalWrite(pin_led[toggleLed], HIGH);
		if (++toggleLed == led_count)
			toggleLed = 0;
		pcf8574->digitalWrite(pin_led[toggleLed], LOW);
	}
}

/**
 * Update led
 * @Param ledpin: the attached pin of the led
 * @Param state: true = led is ON, false = led is OFF
 */
void PCF8574_UpdateLed(uint8_t ledpin, bool state)
{
	(state) ? pcf8574->digitalWrite(ledpin, LOW) : pcf8574->digitalWrite(ledpin, HIGH);
}

// ********************************************************************************
// Task function for INPUT pin
// ********************************************************************************

// Sample function. Should be redefined by the user
void __attribute__((weak)) PCF8574_Keyboard_Action(uint8_t btn_id)
{
	switch (btn_id)
	{
		case 1:
			Serial.println("Btn_DOWN");
			break;
		case 2:
			Serial.println("Btn_OK");
			break;
		case 3:
			Serial.println("Btn_UP");
			break;
		default:
			Serial.println("Btn_Other");
	}
}

void PCF8574_KEY_Task_code(void *parameter)
{
	BEGIN_TASK_CODE("PCF8574_KEY_Task");

	for (EVER)
	{
		if (xSemaphoreTake(PCF8574_Semaphore, 0) == pdTRUE)
		{
			PCF8574::DigitalInput di = pcf8574->digitalReadAll();
			// Use the fact that DigitalInput is like an array of 8 uint8_t
			uint8_t *pdi;
			for (uint8_t i = 0; i < input_count; i++)
			{
				pdi = &di.p0 + input_Modes[i];
				if (*pdi == 0)
				{
					PCF8574_Keyboard_Action(i + 1);
					break;
				}
			}
		}
		END_TASK_CODE(false);
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
