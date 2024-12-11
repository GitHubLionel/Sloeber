/**
 * Some usefull functions to manage I2C display
 * Display can be : LCD (I2C), OLED (1306) or TFT (1327, SH1107)
 * So you must use the directive according your display :
 * USE_LCD, OLED_SSD1306, OLED_SSD1327 or OLED_SH1107
 * All this display use I2C
 */
#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include <stdint.h>

// Définition du support d'affichage
//#define USE_LCD
//#define OLED_SSD1306
//#define OLED_SSD1327
//#define OLED_SH1107

#ifdef USE_LCD
#include "LCD_I2C.h"
#endif

#ifdef OLED_SSD1306
#include "Fonts.h"
#include "SSD1306.h"
#endif

#ifdef OLED_SSD1327
#include "Fonts.h"
#include "SSD1327.h"
#include "SSD1327_Test.h"
#endif

#ifdef OLED_SH1107
#include "SH1107.h"
#endif

#if defined(OLED_SSD1306)
#warning "INFO : OLED_SSD1306 defined !"
#endif

#if defined(OLED_SSD1327)
#warning "INFO : OLED_SSD1327 defined !"
#endif

#if defined(OLED_SH1107)
#warning "INFO : OLED_SH1107 defined !"
#endif

#if ((!defined(USE_LCD)) && (!defined(OLED_SSD1306)) && (!defined(OLED_SSD1327)) && (!defined(OLED_SH1107)))
#define DEFAULT_OUTPUT
#else
#define OLED_DEFINED
#endif

// Print terminal to be defined elsewhere
extern void PrintTerminal(const char *text);

bool IHM_Initialization(uint8_t address, bool test);
bool IHM_IsDisplayOff(void);
void IHM_Print0(const char *text);
void IHM_Print(uint8_t line, const char *text, bool update_screen = false);
void IHM_Display(void);
void IHM_Clear(bool refresh = false);
void IHM_TimeOut_Display(uint32_t time);
void IHM_ToggleDisplay(void);
void IHM_CheckTurnOff(void);
void IHM_DisplayOn(void);
void IHM_DisplayOff(void);
void IHM_IPAddress(const char *ip, uint16_t waitAndClear_ms = 0);
