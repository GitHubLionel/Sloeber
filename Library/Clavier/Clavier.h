#pragma once
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

// Initialisation du clavier à faire APRES l'initialisation ADC
void Clavier_Init(uint8_t channel, uint8_t nbButton, ADC_Sampling sampling = ADC_10bits);
void Clavier_Init(uint8_t channel, uint8_t nbButton, uint16_t interval[]);
// La mise à jour en fond de tache à mettre dans l'évènement HAL_SYSTICK_Callback()
void Clavier_UpdateTime();
// La fonction de test pour voir si on appuyé sur le clavier
bool Check_Clavier(Btn_Action *Btn);

// fonctions annexes
Btn_Action Btn_Click();
const char *Btn_Click_Name();
uint16_t Btn_Click_Val();
void Btn_Check_Config(bool infinite = false);
