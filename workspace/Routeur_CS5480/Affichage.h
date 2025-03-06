#ifndef __AFFICHAGE_H__
#define __AFFICHAGE_H__

#include "Arduino.h"

// La définition de la tache pour l'affichage oled
#define DISPLAY_DATA_TASK	{condCreate, "DISPLAY_Task", 4096, 4, 1000, Core0, Display_Task_code}
void Display_Task_code(void *parameter);

#endif /* __AFFICHAGE_H__ */
