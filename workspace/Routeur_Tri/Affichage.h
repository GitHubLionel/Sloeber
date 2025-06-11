#ifndef __AFFICHAGE_H__
#define __AFFICHAGE_H__

#include "Arduino.h"

// La définition de la tache pour l'affichage oled
#define DISPLAY_DATA_TASK	{condCreate, "DISPLAY_Task", 4096, 4, 1000, CoreAny, Display_Task_code}
void Display_Task_code(void *parameter);

#define SSR_LED_TASK	{condCreate, "SSR_LED_Task", 4096, 4, 100, CoreAny, SSR_LED_Task_code}
void SSR_LED_Task_code(void *parameter);

void UpdateLedRelayFacade(void);
void UpdateLedSSR(uint16_t val);

#endif /* __AFFICHAGE_H__ */
