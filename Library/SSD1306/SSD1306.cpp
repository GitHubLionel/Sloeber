/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>

   ----------------------------------------------------------------------
     Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#include "SSD1306.h"
#include <Wire.h>

/* Absolute value */
#define ABS(x)   ((x) > 0 ? (x) : -(x))

/* SSD1306 data buffer */
#define BUFFER_SIZE  (SSD1306_RAM_WIDTH * SSD1306_HEIGHT / 8)
static uint8_t SSD1306_Buffer[BUFFER_SIZE];
static uint8_t *SSD1306_Buffer_ptr = NULL;

#define SSD1306_COMMAND   0x00
#define SSD1306_DATA      0x40

/* Private SSD1306 structure */
typedef struct {
	uint16_t CurrentX;
	uint16_t CurrentY;
	uint8_t Inverted;
	uint8_t Initialized;
} SSD1306_TypeDef;

/* Private variable */
static uint16_t SSD1306_I2C_ADDR;
static SSD1306_TypeDef SSD1306;
static bool DisplayIsOn = true;

// ********************************************************************************
// I2C Handler function
// ********************************************************************************

void SSD1306_WRITECOMMAND(uint8_t command)
{
	Wire.beginTransmission(SSD1306_I2C_ADDR);
	Wire.write(SSD1306_COMMAND);
	Wire.write(command);
	Wire.endTransmission();
}

void SSD1306_WRITEDATA(uint8_t *data, uint16_t count)
{
	Wire.beginTransmission(SSD1306_I2C_ADDR);
	Wire.write(SSD1306_DATA);
	while (count-- > 0)
		Wire.write(*data++);
	Wire.endTransmission();
}

// ********************************************************************************
// OLED functions
// ********************************************************************************
/**
 * Initialize the OLED with I2C protocol (fast speed, 400000 Hz)
 * - Priority : High
 * - Mode : Normal
 * - Data Width : Byte
 * I2C address is 0x3C for 0.96" oled
 * Don't forget to use the right RAM size in header file
 */
bool SSD1306_Init(uint8_t pin_SDA, uint8_t pin_SCL, uint16_t I2C_Address, uint32_t I2C_Clock)
{     
	SSD1306_I2C_ADDR = I2C_Address;

#ifdef ESP8266
	Wire.begin(pin_SDA, pin_SCL);
	Wire.setClock(I2C_Clock);
	if (Wire.status() != 0)
		return false;
#endif
#ifdef ESP32
	if (!Wire.begin(pin_SDA, pin_SCL, I2C_Clock))
		return false;
#endif

	// Pointer sur le buffer du SSD1306
	SSD1306_Buffer_ptr = (uint8_t *)&SSD1306_Buffer;

	delay(200);

	/* Init LCD */
	SSD1306_WRITECOMMAND(0xAE); //display off
	SSD1306_WRITECOMMAND(0x20); //Set Memory Addressing Mode
	SSD1306_WRITECOMMAND(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	SSD1306_WRITECOMMAND(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
	SSD1306_WRITECOMMAND(0xC8); //Set COM Output Scan Direction
	SSD1306_WRITECOMMAND(0x00); //---set low column address
	SSD1306_WRITECOMMAND(0x10); //---set high column address
	SSD1306_WRITECOMMAND(0x40); //--set start line address
	SSD1306_WRITECOMMAND(0x81); //--set contrast control register
	SSD1306_WRITECOMMAND(0xFF);
	SSD1306_WRITECOMMAND(0xA1); //--set segment re-map 0 to 127
	SSD1306_WRITECOMMAND(0xA6); //--set normal display
	SSD1306_WRITECOMMAND(0xA8); //--set multiplex ratio(1 to 64)
	SSD1306_WRITECOMMAND(0x3F); //
	SSD1306_WRITECOMMAND(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	SSD1306_WRITECOMMAND(0xD3); //-set display offset
	SSD1306_WRITECOMMAND(0x00); //-not offset
	SSD1306_WRITECOMMAND(0xD5); //--set display clock divide ratio/oscillator frequency
	SSD1306_WRITECOMMAND(0xF0); //--set divide ratio
	SSD1306_WRITECOMMAND(0xD9); //--set pre-charge period
	SSD1306_WRITECOMMAND(0x22); //
	SSD1306_WRITECOMMAND(0xDA); //--set com pins hardware configuration
	SSD1306_WRITECOMMAND(0x12);
	SSD1306_WRITECOMMAND(0xDB); //--set vcomh
	SSD1306_WRITECOMMAND(0x20); //0x20,0.77xVcc
	SSD1306_WRITECOMMAND(0x8D); //--set DC-DC enable
	SSD1306_WRITECOMMAND(0x14); //
	SSD1306_WRITECOMMAND(0xAF); //--turn on SSD1306 panel

	/* Clear screen */
	SSD1306_Clear_Screen();

	/* Set default values */
	SSD1306.CurrentX = 0;
	SSD1306.CurrentY = 0;

	/* Inverted No */
	SSD1306.Inverted = 0;

	/* Initialized OK */
	SSD1306.Initialized = 1;
	return true;
}

/**
 * Update the lcd screen
 * Must be call after all operation
 */
void SSD1306_UpdateScreen(void)
{
	uint8_t i;

	if (DisplayIsOn)
		for (i = 0; i < 8; i++)
		{
			SSD1306_WRITECOMMAND(0xB0 + i);
			SSD1306_WRITECOMMAND(0x00);
			SSD1306_WRITECOMMAND(0x10);

			SSD1306_WRITEDATA(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_RAM_WIDTH);
		}
}

void SSD1306_ToggleInvert(void)
{
	uint16_t i;

	/* Toggle invert */
	SSD1306.Inverted = !SSD1306.Inverted;

	/* Do memory toggle */
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		SSD1306_Buffer_ptr[i] = ~SSD1306_Buffer_ptr[i];
	}
}

/**
 * Fill the buffer memory with the color but not update the screen
 */
void SSD1306_Fill(SSD1306_Color color)
{
	/* Set memory */
	memset(SSD1306_Buffer_ptr, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, BUFFER_SIZE);
}

/**
 * Clear the buffer memory and the screen
 */
void SSD1306_Clear_Screen(void)
{
	SSD1306_Fill(SSD1306_COLOR_BLACK);
	SSD1306_UpdateScreen();
}

void SSD1306_Test_Screen(void)
{
	uint16_t i;
	uint8_t q;
	uint8_t p = 0xF0;

	// Dessine un damier
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		SSD1306_Buffer_ptr[i] = p;
		if ((i+1) % 4 == 0) {
			q = p;
			p = p << 4;
			p |= q >> 4;
		}
	}

	SSD1306_GotoXY(40,5);
	SSD1306_PutString((char *)"TEST LCD", Font_7x10, SSD1306_COLOR_WHITE);
	SSD1306_GotoXY(20,30);
	SSD1306_PutString((char *)"TEST LCD", Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();
	delay(1000);

	// Flip the screen for fun !
	for (i = 1; i < 10; i++)
	{
		SSD1306_ToggleInvert();
		SSD1306_UpdateScreen();
		delay(500);
	}
	SSD1306_ToggleInvert();
	SSD1306_Fill(SSD1306_COLOR_BLACK);
	SSD1306_UpdateScreen();
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_Color color)
{
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
	{
		/* Error */
		return;
	}

#ifdef SSD1306_RAM_132
	x += SSD1306_OFFSET_WIDTH;
#endif

	/* Check if pixels are inverted */
	if (SSD1306.Inverted)
	{
		color = (SSD1306_Color)!color;
	}

	/* Set color */
	if (color == SSD1306_COLOR_WHITE)
	{
		SSD1306_Buffer_ptr[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} else {
		SSD1306_Buffer_ptr[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}

/**
 * Les coordonnées x, y à l'écran. Repère le haut du caractère.
 * 0,0 en haut à gauche = premier caractère sur l'écran sur la première ligne
 * La seconde ligne sera y = hauteur du caractère de la première ligne + espace
 * x < SSD1306_WIDTH; y < SSD1306_HEIGHT
 */
void SSD1306_GotoXY(uint16_t x, uint16_t y)
{
	/* Set write pointers */
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}

/**
 * Write char ch at current position.
 * Font is : Font_7x10, Font_11x18, Font_16x26
 * If not enough space, return 0
 */
char SSD1306_PutChar(char ch, Font_TypeDef font, SSD1306_Color color)
{
	SSD1306_Color notcolor = SSD1306_COLOR_BLACK;
	uint8_t i,j;
	uint16_t b, charPos;

	if (ch < 32) return 0;
	charPos = (ch - 32) * font.FontHeight;

	if (color == SSD1306_COLOR_BLACK)
		notcolor = SSD1306_COLOR_WHITE;

	// Check remaining space on current line
	if (SSD1306_WIDTH <= (SSD1306.CurrentX + font.FontWidth) ||
			SSD1306_HEIGHT <= (SSD1306.CurrentY + font.FontHeight))
	{
		// Not enough space on current line
		return 0;
	}

	// Use the font to write
	for (i = 0; i < font.FontHeight; i++)
	{
#ifdef USE_PROGMEM
		b = pgm_read_word_near(&font.data[charPos + i]);
#else
		b = font.data[charPos + i];
#endif
		for (j = 0; j < font.FontWidth; j++)
		{
			if ((b << j) & 0x8000)
			{
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), color);
			}
			else
			{
				SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), notcolor);
			}
		}
	}

	// The current space is now taken
	SSD1306.CurrentX += font.FontWidth;

	/* Return character written */
	return ch;
}

/**
 * Write string str at current position.
 * Font is : Font_7x10, Font_11x18, Font_16x26
 * If string is too long, return the last character printed else return 0
 */
char SSD1306_PutString(const char* str, Font_TypeDef font, SSD1306_Color color)
{
	/* Write characters */
	while (*str)
	{
		/* Write character by character */
		if (SSD1306_PutChar(*str, font, color) == 0)
		{
			/* Return error */
			return *str;
		}

		/* Increase string pointer */
		str++;
	}

	/* Everything OK, zero should be returned */
	return *str;
}

/**
 * Write string str at current position.
 * Font is : Font_7x10, Font_11x18, Font_16x26
 * If string is too long, string is truncated
 */
char SSD1306_PutString(String str, Font_TypeDef font, SSD1306_Color color)
{
	/* Write characters */
	for (unsigned int i=0; i<str.length(); i++)
	{
		/* Write character by character */
		if (SSD1306_PutChar(str.charAt(i), font, color) == 0)
		{
			/* Return error */
			return str.charAt(i);
		}
	}
	return 0;
}

inline void swap(uint16_t *x, uint16_t *y)
{
	uint16_t tmp = *y;
	*y = *x;
	*x = tmp;
}

void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_Color color)
{
	int16_t dx, dy, sx, sy, err, e2, i;

	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) x0 = SSD1306_WIDTH - 1;
	if (x1 >= SSD1306_WIDTH) x1 = SSD1306_WIDTH - 1;
	if (y0 >= SSD1306_HEIGHT) y0 = SSD1306_HEIGHT - 1;
	if (y1 >= SSD1306_HEIGHT) y1 = SSD1306_HEIGHT - 1;

	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	if (dx == 0)
	{
		if (y1 < y0) swap(&y0,&y1);

		if (x1 < x0) swap(&x0,&x1);

		/* Vertical line */
		for (i = y0; i <= y1; i++)
		{
			SSD1306_DrawPixel(x0, i, color);
		}

		/* Return from function */
		return;
	}

	if (dy == 0)
	{
		if (y1 < y0) swap(&y0,&y1);

		if (x1 < x0) swap(&x0,&x1);

		/* Horizontal line */
		for (i = x0; i <= x1; i++)
		{
			SSD1306_DrawPixel(i, y0, color);
		}

		/* Return from function */
		return;
	}

	while (1)
	{
		SSD1306_DrawPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
		{
			break;
		}
		e2 = err;
		if (e2 > -dx)
		{
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			y0 += sy;
		}
	}
}

void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_Color color)
{
	/* Check input parameters */
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
	{
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH)
	{
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT)
	{
		h = SSD1306_HEIGHT - y;
	}

	/* Draw 4 lines */
	SSD1306_DrawLine(x, y, x + w, y, color);         /* Top line */
	SSD1306_DrawLine(x, y + h, x + w, y + h, color); /* Bottom line */
	SSD1306_DrawLine(x, y, x, y + h, color);         /* Left line */
	SSD1306_DrawLine(x + w, y, x + w, y + h, color); /* Right line */
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_Color color)
{
	uint8_t i;

	/* Check input parameters */
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
	{
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH)
	{
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT)
	{
		h = SSD1306_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++)
	{
		/* Draw lines */
		SSD1306_DrawLine(x, y + i, x + w, y + i, color);
	}
}

void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_Color color)
{
	/* Draw lines */
	SSD1306_DrawLine(x1, y1, x2, y2, color);
	SSD1306_DrawLine(x2, y2, x3, y3, color);
	SSD1306_DrawLine(x3, y3, x1, y1, color);
}


void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_Color color)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1)
	{
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)
	{
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay)
	{
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		SSD1306_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den)
		{
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_Color color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, color);
	SSD1306_DrawPixel(x0, y0 - r, color);
	SSD1306_DrawPixel(x0 + r, y0, color);
	SSD1306_DrawPixel(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		SSD1306_DrawPixel(x0 + x, y0 + y, color);
		SSD1306_DrawPixel(x0 - x, y0 + y, color);
		SSD1306_DrawPixel(x0 + x, y0 - y, color);
		SSD1306_DrawPixel(x0 - x, y0 - y, color);

		SSD1306_DrawPixel(x0 + y, y0 + x, color);
		SSD1306_DrawPixel(x0 - y, y0 + x, color);
		SSD1306_DrawPixel(x0 + y, y0 - x, color);
		SSD1306_DrawPixel(x0 - y, y0 - x, color);
	}
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_Color color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	SSD1306_DrawPixel(x0, y0 + r, color);
	SSD1306_DrawPixel(x0, y0 - r, color);
	SSD1306_DrawPixel(x0 + r, y0, color);
	SSD1306_DrawPixel(x0 - r, y0, color);
	SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
}

void SSD1306_DrawImage(uint8_t *img, uint8_t frame, uint8_t x, uint8_t y)
{
	uint32_t i, b, j;

	b = 0;
	if(frame >= img[2])
		return;
	uint32_t start = (frame * (img[3] + (img[4] << 8)));

	/* Go through font */
	for (i = 0; i < img[1]; i++)
	{
		for (j = 0; j < img[0]; j++)
		{
			if (((img[b/8 + 5 + start] >> (b%8)) & 1) == SSD1306_COLOR_WHITE)
				SSD1306_DrawPixel(x + j, (y + i), SSD1306_COLOR_WHITE);
			else
				SSD1306_DrawPixel(x + j, (y + i), SSD1306_COLOR_BLACK);
			b++;
		}
	}
}

/**
 * Put Display ON
 */
void SSD1306_ON(void)
{
	SSD1306_WRITECOMMAND(0x8D);
	SSD1306_WRITECOMMAND(0x14);
	SSD1306_WRITECOMMAND(0xAF);
	DisplayIsOn = true;
	// Refresh the screen
	SSD1306_UpdateScreen();
}

/**
 * Put Display OFF
 */
void SSD1306_OFF(void)
{
	SSD1306_WRITECOMMAND(0x8D);
	SSD1306_WRITECOMMAND(0x10);
	SSD1306_WRITECOMMAND(0xAE);
	DisplayIsOn = false;
}

/**
 * Toggle display
 */
void SSD1306_ToggleOnOff(void)
{
	if (DisplayIsOn)
	{
		SSD1306_OFF();
	}
	else
	{
		SSD1306_ON();
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************
