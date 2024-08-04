
/* Includes ------------------------------------------------------------------*/
#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef ESP32
// I2C0 : GPIO22(SCL) et GPIO21(SDA)
// I2C1 : GPIO12(SCL) et GPIO14(SDA)
#define PIN_WIRE_SDA	21 // 14
#define PIN_WIRE_SCL  22 // 12
#endif

volatile uint8_t Display_offset_line = 0;
uint32_t Oled_timeout = 600; // 10 minutes
int Oled_timeout_count = 600; // 10 minutes
bool TurnOff = false;

/* Private function prototypes -----------------------------------------------*/

/**
 * Initialization of the display
 * address : the address of the I2C port, generally 0x3C other 0x78
 * test : display a small animation text
 */
bool IHM_Initialization(uint8_t address, bool test)
{
	bool success = true;
#ifdef OLED_SSD1306
	success = SSD1306_Init(PIN_WIRE_SDA, PIN_WIRE_SCL, address);
	if (success && test)
	  SSD1306_Test_Screen();
#endif

#ifdef OLED_SSD1327
	success = SSD1327_Init(PIN_WIRE_SDA, PIN_WIRE_SCL, address);
	if (success && test)
    SSD1327_Test_Screen();
#endif

#ifdef OLED_SH1107
	success = SH1107_Init(PIN_WIRE_SDA, PIN_WIRE_SCL, address);
	if (success && test)
		SH1107_Test_Screen();
#endif

#ifdef DEFAULT_OUTPUT
	(void) address;
	(void) test;
#endif
	return success;
}

// *****************************************************************************
// Gestion IHM
// *****************************************************************************

#ifdef OLED_SSD1327
extern void PrintTerminal(const char *text);
void SSD1327_Print(const String &str)
{
//  printf(str.c_str());
  PrintTerminal(str.c_str());
}
#endif

/**
 * Print the text in the first position
 */
void IHM_Print0(const char *text)
{
#ifdef USE_LCD
  LCD_I2C_Str(1, 1 ,text, true);
#endif
#ifdef OLED_SSD1306
	SSD1306_GotoXY(0, 0);
	SSD1306_PutString(text, Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();
#endif
#ifdef OLED_SSD1327
	SSD1327_String({0, 20}, text, &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_DisplayUpdated();
#endif
#ifdef OLED_SH1107
	SH1107_Fill(0x0, 0);
	SH1107_WriteString(0, 0, 2, (char*) text, FONT_NORMAL, 0);
	SH1107_DumpBuffer();
#endif
#ifdef DEFAULT_OUTPUT
  // Par défaut envoie dans la console
  PrintTerminal(text);
#endif
}

/**
 * Transfer the text to the device. For some devices (Oled, TFT), you need to refresh it
 * with de Display_IHM function or set update_screen to true (default false).
 * Text is printed in the beginning of the line.
 */
void IHM_Print(uint8_t line, const char *text, bool update_screen)
{
#ifdef DEFAULT_OUTPUT
  // Par défaut envoie dans la console
  PrintTerminal(text);
  (void) line;
  (void) update_screen;
#endif
  // Si l'écran est éteint, on ne fait rien
  if (TurnOff)
  	return;
  line += Display_offset_line;
#ifdef USE_LCD
  LCD_I2C_Str(line+1, 1 ,text, true);
#endif
#ifdef OLED_SSD1306
  SSD1306_GotoXY(0,10*line);
  SSD1306_PutString(text, Font_7x10, SSD1306_COLOR_WHITE);
  if (update_screen)
  	SSD1306_UpdateScreen();
#endif
#ifdef OLED_SSD1327
  SSD1327_String(TPoint(0, 12*line), text, &Font12, FONT_BACKGROUND, SSD1327_WHITE);
  if (update_screen)
  	SSD1327_DisplayUpdated();
#endif
#ifdef OLED_SH1107
	SH1107_WriteString(0, 0, line, (char*) text, FONT_NORMAL, 0);
  if (update_screen)
  	SH1107_DumpBuffer();
#endif
}

/**
 * Refresh the device
 */
void IHM_Display(void)
{
  // Actualise l'écran
#ifdef OLED_SSD1306
  SSD1306_UpdateScreen();
#endif
#ifdef OLED_SSD1327
  SSD1327_Display();
#endif
#ifdef OLED_SH1107
	SH1107_DumpBuffer();
#endif
}

/**
 * Clear the memory device but not the display.
 * Call IHM_Display() to refresh the display or set refresh to true (default false).
 */
void IHM_Clear(bool refresh)
{
  // Efface la mémoire de l'écran mais ne rafraichit pas le oled
#ifdef USE_LCD
  LCD_I2C_Clear_Screen();
#endif
#ifdef OLED_SSD1306
  SSD1306_Fill(SSD1306_COLOR_BLACK);
#endif
#ifdef OLED_SSD1327
  SSD1327_Clear(SSD1327_BACKGROUND);
#endif
#ifdef OLED_SH1107
  SH1107_Fill(0x0, 0);
#endif
  if (refresh)
  	IHM_Display();
}

/**
 * Display managment on/off : set the timeout
 */
void IHM_TimeOut_Display(uint32_t time)
{
	Oled_timeout = time;
	Oled_timeout_count = time;
	TurnOff = false;
}

/**
 * toggle On/Off the display
 */
void IHM_ToggleDisplay(void)
{
#ifdef OLED_SSD1306
	SSD1306_ToggleOnOff();
#endif
#ifdef OLED_SSD1327
	SSD1327_ToggleOnOff();
#endif
#ifdef OLED_SH1107
	SH1107_ToggleOnOff();
#endif
	// Re-init timeout
	IHM_TimeOut_Display(Oled_timeout);
}

/**
 * Check if timeout is raised to turn off the display
 */
void IHM_CheckTurnOff(void)
{
	if (!TurnOff)
	{
		TurnOff = (Oled_timeout_count-- == 0);
		if (TurnOff)
			IHM_DisplayOff();
	}
}

/**
 * Turn On the display and re-init the timeout
 */
void IHM_DisplayOn(void)
{
#ifdef OLED_SSD1306
	SSD1306_ON();
#endif
#ifdef OLED_SSD1327
	SSD1327_ON();
#endif
#ifdef OLED_SH1107
	SH1107_ON();
#endif
	IHM_TimeOut_Display(Oled_timeout);
}

/**
 * Turn off the display
 */
void IHM_DisplayOff(void)
{
#ifdef OLED_SSD1306
	SSD1306_OFF();
#endif
#ifdef OLED_SSD1327
	SSD1327_OFF();
#endif
#ifdef OLED_SH1107
	SH1107_OFF();
#endif
	TurnOff = true;
}

/**
 * Display the IPAddress
 */
void IHM_IPAddress(const char *ip)
{
#ifdef USE_LCD
  LCD_I2C_Str(1, 1 ,ip, true);
#endif
#ifdef OLED_SSD1306
  SSD1306_Clear_Screen();
	SSD1306_GotoXY(0, 0);
	SSD1306_PutString(ip, Font_7x10, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();
//	delay(2000);
//	SSD1306_Clear_Screen();
#endif
#ifdef OLED_SSD1327
	SSD1327_String({0, 20}, ip, &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_DisplayUpdated();
//	delay(2000);
//	SSD1327_Clear(SSD1327_BACKGROUND);
//	SSD1327_Display();
#endif
#ifdef OLED_SH1107
	SH1107_Fill(0x0, 0);
	SH1107_WriteString(0, 0, 2, (char*) ip, FONT_NORMAL, 0);
	SH1107_DumpBuffer();
//	delay(2000);
//  SH1107_Fill(0x0, 1);
#endif
#ifdef DEFAULT_OUTPUT
  (void) ip;
#endif
}

// *****************************************************************
// ***** Fin
// *****************************************************************

