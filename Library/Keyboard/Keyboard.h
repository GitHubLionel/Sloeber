/**
 * This library help the gestion of a keyboard who is several buttons
 * connected on one ADC.
 * When clicked, each button produce a different voltage who is analyzed to determine the button.
 */
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

typedef enum
{
	Btn_NOP = 0,
	Btn_K1,
	Btn_K2,
	Btn_K3,
	Btn_K4,
	BTN_MAX
} Btn_Action;

typedef enum
{
	ADC_10bits = 1023,
	ADC_12bits = 4095,
	ADC_16bits = 65535
} ADC_Sampling;

extern const char *Btn_Texte[BTN_MAX];

// Debouncing time
#ifndef DEBOUNCING_MS
#define DEBOUNCING_MS      200  // Minimum time in ms between two button click readings
#endif
#ifndef DEBOUNCING_US
#define DEBOUNCING_US      200000  // Minimum time in us between two button click readings
#endif

// To create a basic task to check TeleInfo every 1 s
#ifdef KEYBOARD_USE_TASK
#define KEYBOARD_DATA_TASK(start)	{(start), "KEYBOARD_Task", 4096, 10, 10, Core1, KEYBOARD_Task_code}
void KEYBOARD_Task_code(void *parameter);
void UserKeyboardAction(Btn_Action Btn_Clicked);
#endif

// Callback to be executed if button is clicked
typedef void (*KeyBoard_Click_cb)(Btn_Action Btn);

// Initialisation du clavier
void Keyboard_Initialize(uint8_t pin, uint8_t nbButton, ADC_Sampling sampling = ADC_10bits,
		const KeyBoard_Click_cb &kbClick = NULL);
void Keyboard_Initialize(uint8_t pin, uint8_t nbButton, const uint16_t interval[],
		const KeyBoard_Click_cb &kbClick = NULL);
// La mise à jour en fond de tache à mettre dans la boucle loop
void Keyboard_UpdateTime(void);
// La fonction de test pour voir si on a appuyé sur le clavier
bool Check_Keyboard(Btn_Action *Btn);
void SetKeyBoardCallback(const KeyBoard_Click_cb &kbClick);

// fonctions annexes
Btn_Action Btn_Click();
const char* Btn_Click_Name();
uint16_t Btn_Click_Val();
bool Btn_Click_Val(uint16_t *value);
void Btn_Check_Config(bool infinite = false);
