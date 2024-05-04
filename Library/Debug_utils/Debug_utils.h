#pragma once

#include "Arduino.h"

// Délai de 5 s avant de démarrer, permet de lancer la console UART quand on debogue
#define	WAIT_SETUP	5000

// Pour le debug dans le log
void print_debug(String mess, bool ln = true);
void print_debug(const char *mess, bool ln = true);
void print_debug(int val, bool ln = true);
void print_debug(float val, bool ln = true);
void print_millis(void);
#ifdef USE_SAVE_CRASH
void init_and_print_crash(void);
void print_crash(void);
#endif
