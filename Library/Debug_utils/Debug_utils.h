/**
 * Gestion des messages de debug et initialisation de UART (Serial) define USE_UART
 * Gestion du dump du crash
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash

#pragma once

#include "Arduino.h"

// Baud par défaut
#define UART_BAUD	115200

// Délai de 5 s avant de démarrer, permet de lancer la console UART quand on debogue
#define	WAIT_SETUP	5000

// Affiche le nom du sketch et des infos sur le démarrage
String getSketchName(const String the_path, bool extra = false);

void SERIAL_Initialization(int baud = UART_BAUD);

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
