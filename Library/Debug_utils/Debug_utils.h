/**
 * Gestion des messages de debug et initialisation de UART (Serial) define USE_UART
 * Gestion du dump du crash
 * Define de debug
 * SERIAL_DEBUG	: Message de debug sur le port série (baud 115200)
 * LOG_DEBUG	: Enregistre le debug dans un fichier log
 *
 * Analyse de messages reçus par UART
 */
//#define SERIAL_DEBUG
//#define LOG_DEBUG
//#define USE_SAVE_CRASH   // Permet de sauvegarder les données du crash

#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"

// Baud par défaut
#ifndef UART_BAUD
#define UART_BAUD	115200
#endif

// Délai de 5 s avant de démarrer, permet de lancer la console UART quand on debogue
#define	WAIT_SETUP	5000

// For CheckUARTMessage
// The char that begin a message
#define BEGIN_DATA	0x02
// The char that end a message
#define END_DATA	0x03
// The size of the buffer for the message received
#define BUFFER_SIZE	1024
// The buffer that contain the message received
extern volatile char UART_Message_Buffer[];
// End Text string
#define LOG_ETX_STR  	"\03"

// To create a basic task to analyse UART message
#ifdef UART_USE_TASK
#define UART_DATA_TASK	{true, "UART_Task", 4096, 8, 10, Core1, UART_Task_code}
void UART_Task_code(void *parameter);
#endif

// Affiche le nom du sketch et des infos sur le démarrage
String getSketchName(const String the_path, bool extra = false);

#ifdef ESP32
void WatchDog_Init(uint32_t WDT_TIMEOUT_ms);
#endif

void SERIAL_Initialization(int baud = UART_BAUD);

// Pour le debug dans le log
void print_debug(String mess, bool ln = true);
void print_debug(const char *mess, bool ln = true);
void print_debug(char val, bool ln = true);
void print_debug(int val, bool ln = true);
void print_debug(float val, bool ln = true);
void print_millis(void);
#ifdef USE_SAVE_CRASH
void init_and_print_crash(void);
void print_crash(void);
#endif

/**
 * Some UART functions used in another library
 */
bool CheckUARTMessage(HardwareSerial *Serial_Message = &Serial);
bool BasicAnalyseMessage(void);
void printf_message_to_UART(const char *mess, bool balise = true, HardwareSerial *Serial_Message = &Serial);
void printf_message_to_UART(const String &mess, bool balise = true, HardwareSerial *Serial_Message = &Serial);

// Other utilitary function
char* Search_Balise(uint8_t *data, const char *B_Begin, const char *B_end, char *value,
		uint16_t *len);
