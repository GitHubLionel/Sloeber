#ifndef __SH1107__
#define __SH1107__

#include <Arduino.h>

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

// Define to rotate de screen 90°
//#define	OLED_TOP_DOWN
//#define	OLED_LEFT_RIGHT
//#define	OLED_DOWN_TOP
//#define	OLED_RIGHT_LEFT

#if ((!defined(OLED_TOP_DOWN)) && (!defined(OLED_LEFT_RIGHT)) && \
		 (!defined(OLED_DOWN_TOP)) && (!defined(OLED_RIGHT_LEFT)))
#warning "Display orientation not defined, use default OLED_TOP_DOWN"
#define	OLED_TOP_DOWN
#endif

#ifdef I2C_FREQUENCY
#define SH1107_CLOCK	I2C_FREQUENCY
#else
	#ifdef ESP32
	#define SH1107_CLOCK							800000	// Clock speed
	#else
	#define SH1107_CLOCK							400000	// Clock speed
	#endif
#endif

typedef struct
{
	uint8_t oled_addr; // requested address
	uint8_t oled_wrap;
	uint8_t *ucScreen;
	uint8_t iCursorX, iCursorY;
	uint8_t oled_x, oled_y;
	int iScreenOffset;
} OLED_1107;

// 4 possible font sizes: 8x8, 16x32, 6x8, 16x16 (stretched from 8x8)
enum
{
	FONT_6x8 = 0,
	FONT_8x8,
	FONT_12x16,
	FONT_16x16,
	FONT_16x32
};

// 4 possible rotation angles for oledScaledString()
enum
{
	ROT_0 = 0,
	ROT_90,
	ROT_180,
	ROT_270
};

#define FONT_NORMAL FONT_8x8
#define FONT_SMALL FONT_6x8
#define FONT_LARGE FONT_16x32
#define FONT_STRETCHED FONT_16x16

// Rotation and flip angles to draw tiles
enum
{
	ANGLE_0 = 0,
	ANGLE_90,
	ANGLE_180,
	ANGLE_270,
	ANGLE_FLIPX,
	ANGLE_FLIPY
};

//
// Initializes the OLED controller into "page mode" on I2C
//
bool SH1107_Init(uint8_t pin_SDA, uint8_t pin_SCL, uint16_t I2C_Address, uint32_t I2C_Clock = SH1107_CLOCK);

//
// Provide or revoke a back buffer for your OLED graphics
// This allows you to manage the RAM used by SH1107 on tiny
// embedded platforms like the ATmega series
// Pass NULL to revoke the buffer. Make sure you provide a buffer
// large enough for your display (e.g. 128x64 needs 1K - 1024 bytes)
//
void SH1107_SetBackBuffer(uint8_t *pBuffer);

//
// Just a simple test
//
void SH1107_Test_Screen(void);

//
// Power up/down the display
// useful for low power situations
//
void SH1107_Power(uint8_t bOn);

//
// Shortcut power functions
//
void SH1107_ON(void);
void SH1107_OFF(void);
bool SH1107_ToggleOnOff(void);

//
// Sets the brightness (0=off, 255=brightest)
//
void SH1107_SetContrast(unsigned char ucContrast);

//
// Invert the display
//
void SH1107_SetInvertDisplay(uint8_t bNormal);

//
// Load a 128x64 1-bpp Windows bitmap
// Pass the pointer to the beginning of the BMP file
// First pass version assumes a full screen bitmap
//
int SH1107_LoadBMP(uint8_t *pBMP, int bInvert, int bRender = 0);

//
// Set the current cursor position
// The column represents the pixel column (0-127)
// The row represents the text row (0-7)
//
void SH1107_SetCursor(int x, int y);

//
// Turn text wrap on or off for the oldWriteString() function
//
void SH1107_SetTextWrap(int bWrap);

//
// Draw a string of normal (8x8), small (6x8) or large (16x32) characters
// At the given col+row with the given scroll offset. The scroll offset allows you to
// horizontally scroll text which does not fit on the width of the display. The offset
// represents the pixels to skip when drawing the text. An offset of 0 starts at the beginning
// of the text.
// The system remembers where the last text was written (the cursor position)
// To continue writing from the last position, set the x,y values to -1
// The text can optionally wrap around to the next line by calling SH1107_SetTextWrap(true);
// otherwise text which would go off the right edge will not be drawn and the cursor will
// be left "off screen" until set to a new position explicitly

//
//  Returns 0 for success, -1 for invalid parameter
//
int SH1107_WriteString(int iScrollX, int x, int y, char *szMsg, int iSize, int bInvert,
		int bRender = 0);

//
// Draw a string with a fractional scale in both dimensions
// the scale is a 16-bit integer with and 8-bit fraction and 8-bit mantissa
// To draw at 1x scale, set the scale factor to 256. To draw at 2x, use 512
// The output must be drawn into a memory buffer, not directly to the display
//
int SH1107_ScaledString(int x, int y, char *szMsg, int iSize, int bInvert, int iXScale, int iYScale,
		int iRotation);

//
// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
//
void SH1107_Fill(unsigned char ucData, int bRender = 0);

//
// Set (or clear) an individual pixel
// The local copy of the frame buffer is used to avoid
// reading data from the display controller
// (which isn't possible in most configurations)
// This function needs the USE_BACKBUFFER macro to be defined
// otherwise, new pixels will erase old pixels within the same byte
//
int SH1107_SetPixel(int x, int y, unsigned char ucColor, int bRender = 0);

//
// Dump an entire custom buffer to the display
// useful for custom animation effects
// By default, use the internal buffer
//
void SH1107_DumpBuffer(uint8_t *pBuffer = NULL);

//
// Render a window of pixels from a provided buffer or the library's internal buffer
// to the display. The row values refer to byte rows, not pixel rows due to the memory
// layout of OLEDs. Pass a src pointer of NULL to use the internal backing buffer
// returns 0 for success, -1 for invalid parameter
//
int SH1107_DrawGFX(uint8_t *pSrc, int iSrcCol, int iSrcRow, int iDestCol, int iDestRow, int iWidth,
		int iHeight, int iSrcPitch);

//
// Draw a line between 2 points
//
void SH1107_DrawLine(int x1, int y1, int x2, int y2, int bRender = 0);

//
// Play a frame of animation data
// The animation data is assumed to be encoded for a full frame of the display
// Given the pointer to the start of the compressed data,
// it returns the pointer to the start of the next frame
// Frame rate control is up to the calling program to manage
// When it finishes the last frame, it will start again from the beginning
//
uint8_t* SH1107_PlayAnimFrame(uint8_t *pAnimation, uint8_t *pCurrent, int iLen);

//
// Scroll the internal buffer by 1 scanline (up/down)
// width is in pixels, lines is group of 8 rows
// Returns 0 for success, -1 for invalid parameter
//
int SH1107_ScrollBuffer(int iStartCol, int iEndCol, int iStartRow, int iEndRow, int bUp);

//
// Draw a sprite of any size in any position
// If it goes beyond the left/right or top/bottom edges
// it's trimmed to show the valid parts
// This function requires a back buffer to be defined
// The priority color (0 or 1) determines which color is painted 
// when a 1 is encountered in the source image.
// e.g. when 0, the input bitmap acts like a mask to clear
// the destination where bits are set.
//
void SH1107_DrawSprite(uint8_t *pSprite, int cx, int cy, int iPitch, int x, int y,
		uint8_t iPriority);

//
// Draw a 16x16 tile in any of 4 rotated positions
// Assumes input image is laid out like "normal" graphics with
// the MSB on the left and 2 bytes per line
// On AVR, the source image is assumed to be in FLASH memory
// The function can draw the tile on byte boundaries, so the x value
// can be from 0 to 112 and y can be from 0 to 6
//
void SH1107_DrawTile(const uint8_t *pTile, int x, int y, int iRotation, int bInvert, int bRender = 0);

//
// Draw an outline or filled ellipse
//
void SH1107_Ellipse(int iCenterX, int iCenterY, int32_t iRadiusX, int32_t iRadiusY, uint8_t ucColor,
		uint8_t bFilled);

//
// Draw an outline or filled rectangle
//
void SH1107_Rectangle(int x1, int y1, int x2, int y2, uint8_t ucColor, uint8_t bFilled);

#endif // __SH1107__
