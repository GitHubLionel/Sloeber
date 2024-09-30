//
// ss_oled (Small, Simple OLED library)
// Copyright (c) 2017-2019 BitBank Software, Inc.
// Written by Larry Bank (bitbank@pobox.com)
// Project started 1/15/2017
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "SH1107.h"
#include <Wire.h>

#define SH110X_COMMAND    			   0x00 ///< enter command mode
#define SH110X_DATA         			 0x40 ///< enter data mode

#define SH110X_SETLOWCOLUMN 			 0x00 ///< Set Lower Column Address: (00H - 0FH)
#define SH110X_SETHIGHCOLUMN 			 0x10 ///< Set Higher Column Address: (10H - 17H)
#define SH110X_MEMORYMODE 			   0x20 ///< Page addressing mode
#define SH110X_COLUMNADDR 				 0x21 ///< Vertical addressing mode
#define SH110X_PAGEADDR 					 0x22 ///< See datasheet
#define SH110X_SETCONTRAST 				 0x81 ///< Contrast Control Mode Set : 00H - FFH
#define SH110X_CHARGEPUMP 				 0x8D ///< See datasheet
#define SH110X_SEGREMAP 					 0xA0 ///< Set Segment Re-map (with SH110X_COMSCANINC)
#define SH110X_SEGREMAPFLIP				 0xA1 ///< Set Segment Re-map flip (with SH110X_COMSCANDEC)
#define SH110X_DISPLAYALLON_ON     0xA4 ///< Set Entire Display ON
#define SH110X_DISPLAYALLON_OFF		 0xA5 ///< Set Entire Display OFF
#define SH110X_NORMALDISPLAY 			 0xA6 ///< Set Normal Display
#define SH110X_INVERTDISPLAY 			 0xA7 ///< Set Reverse Display
#define SH110X_SETMULTIPLEX 			 0xA8 ///< Multiplex Ration Mode Set : 00H - 7FH
#define SH110X_DCDC 							 0xAD ///< DC-DC Control Mode Set : 8AH (ON) - 8BH (OFF)
#define SH110X_DISPLAYOFF 				 0xAE ///< Display OFF
#define SH110X_DISPLAYON					 0xAF ///< Display ON
#define SH110X_SETPAGEADDR 				 0xB0 ///< Set Page Address B0H to BFH
#define SH110X_COMSCANINC 				 0xC0 ///< Set Common Output Scan Direction C0H to C8H
#define SH110X_COMSCANDEC 				 0xC8 ///<
#define SH110X_SETDISPLAYOFFSET 	 0xD3 ///< Set Display Offset : 00H – 7FH
#define SH110X_SETDISPLAYCLOCKDIV  0xD5 ///< Set Display Clock Divide Ratio/Oscillator Frequency : 00H - FFH
#define SH110X_SETPRECHARGE 			 0xD9 ///< Set Dis-charge/Pre-charge Period : 00H - FFH
#define SH110X_SETCOMPINS 				 0xDA ///< See datasheet
#define SH110X_SETVCOMDETECT 			 0xDB ///< Set VCOM Deselect Level : 00H - FFH
#define SH110X_SETDISPSTARTLINE 	 0xDC ///< Set Display Start Line : 00H – 7FH
#define SH110X_READWRITE    			 0xE0 ///< Read-Modify-Write
#define SH110X_READWRITEEND  			 0xEE ///< Read-Modify-Write end sequence
#define SH110X_NOP          			 0xE3 ///< No Operation Command

#define SH110X_SETSTARTLINE 			 0x40 ///< See datasheet

// Initialization sequences
//const unsigned char oled128_initbuf[] = {0x00, 0xae,0xdc,0x00,0x81,0x40,
//      0xa1,0xc8,0xa8,0x7f,0xd5,0x50,0xd9,0x22,0xdb,0x35,0xb0,0xda,0x12,
//      0xa4,0xa6,0xaf};

#if ((defined(OLED_LEFT_RIGHT)) || (defined(OLED_RIGHT_LEFT)))
#define ROTATED_90
#endif

//const unsigned char oled128_initbuf[] PROGMEM = {
//		SH110X_COMMAND,
//		SH110X_DISPLAYOFF,
//		SH110X_SETDISPSTARTLINE, 0x00,
//		SH110X_SETCONTRAST, 0x40,
//#if ((defined(OLED_TOP_DOWN)) || (defined(OLED_LEFT_RIGHT)))
//		SH110X_SEGREMAP,
//		SH110X_COMSCANINC,
//#endif
//#if ((defined(OLED_DOWN_TOP)) || (defined(OLED_RIGHT_LEFT)))
//		SH110X_SEGREMAPFLIP,
//		SH110X_COMSCANDEC,
//#endif
//		SH110X_SETMULTIPLEX, 0x7F,
//		SH110X_SETDISPLAYCLOCKDIV, 0x50,
//		SH110X_SETPRECHARGE, 0x22,
//		SH110X_SETVCOMDETECT, 0x35,
//		SH110X_SETPAGEADDR,
//		SH110X_SETCOMPINS, 0x12,
//		SH110X_DISPLAYALLON_ON,
//		SH110X_NORMALDISPLAY};

// Adafruit
const unsigned char oled128_initbuf[] PROGMEM = {
		SH110X_COMMAND,
    SH110X_DISPLAYOFF,               // 0xAE
    SH110X_SETDISPLAYCLOCKDIV, 0x51, // 0xd5, 0x51,
    SH110X_MEMORYMODE,               // 0x20
    SH110X_SETCONTRAST, 0xDF,        // 0x81, 0x4F
    SH110X_DCDC, 0x8A,               // 0xAD, 0x8A
#if ((defined(OLED_TOP_DOWN)) || (defined(OLED_LEFT_RIGHT)))
		SH110X_SEGREMAP,
		SH110X_COMSCANINC,
#endif
#if ((defined(OLED_DOWN_TOP)) || (defined(OLED_RIGHT_LEFT)))
		SH110X_SEGREMAPFLIP,
		SH110X_COMSCANDEC,
#endif
    SH110X_SETDISPSTARTLINE, 0x00,    // 0xDC 0x00
    SH110X_SETPRECHARGE, 0x22,        // 0xd9, 0x22,
    SH110X_SETVCOMDETECT, 0x35,       // 0xdb, 0x35,
    SH110X_SETCOMPINS, 0x12,          // 0xda, 0x12,
		SH110X_SETDISPLAYOFFSET, 0x00,
		SH110X_SETMULTIPLEX, 0x7F,
		SH110X_DISPLAYALLON_ON,           // 0xa4
    SH110X_NORMALDISPLAY,             // 0xa6
};

extern uint8_t ucFont[];
extern uint8_t ucBigFont[];
extern uint8_t ucSmallFont[];

// some globals
#define OLED_WIDTH 128
#define OLED_HEIGHT 128
#define BUFFER_SIZE (128 * 128 / 8)  // One octet = 8 screen points
OLED_1107 oled_1107;
static uint8_t ucBackBuffer[BUFFER_SIZE];  // worked buffer
#ifdef ROTATED_90
static uint8_t rotBuffer[BUFFER_SIZE];  // rotated buffer
#endif
static bool DisplayIsOn = true;

static void SH1107_InvertBytes(uint8_t *pData, uint8_t bLen);

static void _I2CWrite(unsigned char *pData, int iLen)
{
	Wire.beginTransmission(oled_1107.oled_addr);
	Wire.write(pData, (uint8_t) iLen);
	Wire.endTransmission();
} /* _I2CWrite() */

static int I2CReadRegister(uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen)
{
	int i = 0;

	Wire.beginTransmission(iAddr);
	Wire.write(u8Register);
	Wire.endTransmission();
	Wire.requestFrom(iAddr, (uint8_t) iLen);
	while (i < iLen)
	{
		pData[i++] = Wire.read();
	}

	return (i > 0);
} /* I2CReadRegister() */

// Send a single byte command to the OLED controller
static void SH1107_WriteCommand(unsigned char c)
{
	unsigned char buf[2] = {SH110X_COMMAND, c};
	_I2CWrite(buf, 2);
} /* SH1107_WriteCommand() */

static void SH1107_WriteCommand2(unsigned char c, unsigned char d)
{
	unsigned char buf[3] = {SH110X_COMMAND, c, d};
	_I2CWrite(buf, 3);
} /* SH1107_WriteCommand2() */

//
// Initializes the OLED controller into "page mode"
//
bool SH1107_Init(uint8_t pin_SDA, uint8_t pin_SCL, uint16_t I2C_Address, uint32_t I2C_Clock)
{
	unsigned char uc[2] = {SH110X_COMMAND, SH110X_DISPLAYON};

	oled_1107.oled_addr = I2C_Address;
	oled_1107.ucScreen = ucBackBuffer;  // default buffer
	oled_1107.oled_wrap = 0; // default - disable text wrap
	oled_1107.oled_x = 128;
	oled_1107.oled_y = 128;

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

	// Verify the display controller (SH1107)
	uint8_t u = 0;
	I2CReadRegister(oled_1107.oled_addr, 0x00, &u, 1); // read the status register
	u &= 0x0F; // mask off power on/off bit
	if (!(u == 0x7 || u == 0xF)) // SH1107
		return false;

	// Init oled SH1107
	_I2CWrite((unsigned char*) oled128_initbuf, sizeof(oled128_initbuf));

	delay(100);
	_I2CWrite(uc, 2);

	return true;
} /* SH1107_Init() */

//
// Invert font data
//
static void SH1107_InvertBytes(uint8_t *pData, uint8_t bLen)
{
	for (uint8_t i = 0; i < bLen; i++)
	{
		*pData = ~(*pData);
		pData++;
	}
} /* SH1107_InvertBytes() */

//
// Provide or revoke a back buffer for your OLED graphics
// This allows you to manage the RAM used by SH1107 on tiny
// embedded platforms like the ATmega series
// Pass NULL to revoke the buffer. Make sure you provide a buffer
// large enough for your display (e.g. 128x64 needs 1K - 1024 bytes)
//
void SH1107_SetBackBuffer(uint8_t *pBuffer)
{
	oled_1107.ucScreen = pBuffer;
} /* SH1107_SetBackBuffer() */

//
// Just a simple test
//
void SH1107_Test_Screen(void)
{
	uint16_t i;
	uint8_t q;
	uint8_t p = 0xF0;

	SH1107_Fill(0x0, 0);
	SH1107_WriteString(0, 16, 0, (char*) "SH1107 Demo", FONT_NORMAL, 0);
	SH1107_WriteString(0, 0, 1, (char*) "---------------------", FONT_SMALL, 1);
	SH1107_WriteString(0, 0, 3, (char*) "**Demo**", FONT_LARGE, 0);
	SH1107_DumpBuffer();
	delay(2000);

	// Dessine un damier
	SH1107_Fill(0x0, 0);
	for (i = 0; i < BUFFER_SIZE; i++)
	{
		ucBackBuffer[i] = p;
		if ((i+1) % 4 == 0) {
			q = p;
			p = p << 4;
			p |= q >> 4;
		}
	}
	SH1107_WriteString(0, 5, 6, (char*) "Invert", FONT_LARGE, 0);
	SH1107_DumpBuffer();
	delay(500);

	// Flip the screen for fun !
	for (i = 2; i < 10; i++)
	{
		SH1107_SetInvertDisplay(i % 2);
		delay(500);
	}

	// Draw direct, no care of orientation
	SH1107_Fill(0, 1);
	for (uint8_t x = 0; x < OLED_WIDTH - 1; x += 2)
	{
		SH1107_DrawLine(x, 0, OLED_WIDTH - 1 - x, OLED_HEIGHT - 1, 1);
	}
	for (uint8_t y = 0; y < OLED_HEIGHT - 1; y += 2)
	{
		SH1107_DrawLine(OLED_WIDTH - 1, y, 0, OLED_HEIGHT - 1 - y, 1);
	}
	delay(2000);

	SH1107_Fill(0, 1);
	SH1107_DumpBuffer();
}

//
// Sends a command to turn on or off the OLED display
//
void SH1107_Power(uint8_t bOn)
{
	if (bOn)
	{
		SH1107_WriteCommand(SH110X_DISPLAYON); // turn on OLED
		DisplayIsOn = true;
	}
	else
	{
		SH1107_WriteCommand(SH110X_DISPLAYOFF); // turn off OLED
		DisplayIsOn = false;
	}
} /* SH1107_Power() */

//
// Sets the brightness (0=off, 255=brightest)
//
void SH1107_SetContrast(unsigned char ucContrast)
{
	SH1107_WriteCommand2(SH110X_SETCONTRAST, ucContrast);
} /* SH1107_SetContrast() */

//
// Invert the display
//
void SH1107_SetInvertDisplay(uint8_t bNormal)
{
	if (bNormal)
		SH1107_WriteCommand(SH110X_NORMALDISPLAY); // normal
	else
		SH1107_WriteCommand(SH110X_INVERTDISPLAY); // inverse
}

//
// Scroll the internal buffer by 1 scanline (up/down)
// width is in pixels, lines is group of 8 rows
//
int SH1107_ScrollBuffer(int iStartCol, int iEndCol, int iStartRow, int iEndRow, int bUp)
{
	uint8_t b, *s;
	int col, row;

	if (iStartCol < 0 || iStartCol > 127 || iEndCol < 0 || iEndCol > 127 || iStartCol > iEndCol) // invalid
		return -1;
	if (iStartRow < 0 || iStartRow > 7 || iEndRow < 0 || iEndRow > 7 || iStartRow > iEndRow)
		return -1;

	if (bUp)
	{
		for (row = iStartRow; row <= iEndRow; row++)
		{
			s = &oled_1107.ucScreen[(row * 128) + iStartCol];
			for (col = iStartCol; col <= iEndCol; col++)
			{
				b = *s;
				b >>= 1; // scroll pixels 'up'
				if (row < iEndRow)
					b |= (s[128] << 7); // capture pixel of row below, except for last row
				*s++ = b;
			} // for col
		} // for row
	} // up
	else // down
	{
		for (row = iEndRow; row >= iStartRow; row--)
		{
			s = &oled_1107.ucScreen[(row * 128) + iStartCol];
			for (col = iStartCol; col <= iEndCol; col++)
			{
				b = *s;
				b <<= 1; // scroll down
				if (row > iStartRow)
					b |= (s[-128] >> 7); // capture pixel of row above
				*s++ = b;
			} // for col
		} // for row
	}
	return 0;
} /* SH1107_ScrollBuffer() */

//
// Send commands to position the "cursor" (aka memory write address)
// to the given row and column
//
static void SH1107_SetPosition(int x, int y, int bRender)
{
	oled_1107.iScreenOffset = (y * 128) + x;
	if (!bRender)
		return; // don't send the commands to the OLED if we're not rendering the graphics now

	unsigned char buf[4];
	buf[0] = SH110X_COMMAND; // command introducer
	buf[1] = 0xb0 | y; // set page to Y
	buf[2] = x & 0xf; // lower column address
	buf[3] = 0x10 | (x >> 4); // upper column addr
	_I2CWrite(buf, 4);
} /* SH1107_SetPosition() */

//
// Write a block of pixel data to the OLED
// Length can be anything from 1 to 1024 (whole display)
//
static void SH1107_WriteDataBlock(unsigned char *ucBuf, int iLen, int bRender)
{
	unsigned char ucTemp[129];

	ucTemp[0] = SH110X_DATA; // data command
// Copying the data has the benefit in SPI mode of not letting
// the original data get overwritten by the SPI.transfer() function
	if (bRender)
	{
		memcpy(&ucTemp[1], ucBuf, iLen);
		_I2CWrite(ucTemp, iLen + 1);
	}
	// Keep a copy in local buffer
	if (oled_1107.ucScreen)
	{
		memcpy(&oled_1107.ucScreen[oled_1107.iScreenOffset], ucBuf, iLen);
		oled_1107.iScreenOffset += iLen;
		oled_1107.iScreenOffset &= 1023; // we use a fixed stride of 128 no matter what the display size
	}
}

//
// Byte operands for compressing the data
// The first 2 bits are the type, followed by the counts
#define OP_MASK 0xc0
#define OP_SKIPCOPY 0x00
#define OP_COPYSKIP 0x40
#define OP_REPEATSKIP 0x80
#define OP_REPEAT 0xc0

//
// Write a block of flash memory to the display
//
void SH1107_WriteFlashBlock(uint8_t *s, int iLen)
{
	int j;
	int iWidthMask = oled_1107.oled_x - 1;
	int iSizeMask = ((oled_1107.oled_x * oled_1107.oled_y) / 8) - 1;
	int iWidthShift = (oled_1107.oled_x == 128) ? 7 : 6; // assume 128 or 64 wide
	uint8_t ucTemp[128];

	while (((oled_1107.iScreenOffset & iWidthMask) + iLen) >= oled_1107.oled_x) // if it will hit the page end
	{
		j = oled_1107.oled_x - (oled_1107.iScreenOffset & iWidthMask); // amount we can write in one shot
		memcpy_P(ucTemp, s, j);
		SH1107_WriteDataBlock(ucTemp, j, 1);
		s += j;
		iLen -= j;
		oled_1107.iScreenOffset = (oled_1107.iScreenOffset + j) & iSizeMask;
		SH1107_SetPosition(oled_1107.iScreenOffset & iWidthMask,
				(oled_1107.iScreenOffset >> iWidthShift), 1);
	} // while it needs some help
	memcpy_P(ucTemp, s, iLen);
	SH1107_WriteDataBlock(ucTemp, iLen, 1);
	oled_1107.iScreenOffset = (oled_1107.iScreenOffset + iLen) & iSizeMask;
} /* SH1107_WriteFlashBlock() */

//
// Write a repeating byte to the display
//
void SH1107_RepeatByte(uint8_t b, int iLen)
{
	int j;
	int iWidthMask = oled_1107.oled_x - 1;
	int iWidthShift = (oled_1107.oled_x == 128) ? 7 : 6; // assume 128 or 64 pixels wide
	int iSizeMask = ((oled_1107.oled_x * oled_1107.oled_y) / 8) - 1;
	uint8_t ucTemp[128];

	memset(ucTemp, b, (iLen > 128) ? 128 : iLen);
	while (((oled_1107.iScreenOffset & iWidthMask) + iLen) >= oled_1107.oled_x) // if it will hit the page end
	{
		j = oled_1107.oled_x - (oled_1107.iScreenOffset & iWidthMask); // amount we can write in one shot
		SH1107_WriteDataBlock(ucTemp, j, 1);
		iLen -= j;
		oled_1107.iScreenOffset = (oled_1107.iScreenOffset + j) & iSizeMask;
		SH1107_SetPosition(oled_1107.iScreenOffset & iWidthMask,
				(oled_1107.iScreenOffset >> iWidthShift), 1);
	} // while it needs some help
	SH1107_WriteDataBlock(ucTemp, iLen, 1);
	oled_1107.iScreenOffset += iLen;
} /* SH1107_RepeatByte() */

//
// Play a frame of animation data
// The animation data is assumed to be encoded for a full frame of the display
// Given the pointer to the start of the compressed data,
// it returns the pointer to the start of the next frame
// Frame rate control is up to the calling program to manage
// When it finishes the last frame, it will start again from the beginning
//
uint8_t* SH1107_PlayAnimFrame(uint8_t *pAnimation, uint8_t *pCurrent, int iLen)
{
	uint8_t *s;
	int i, j;
	unsigned char b, bCode;
	int iBufferSize = (oled_1107.oled_x * oled_1107.oled_y) / 8; // size in bytes of the display devce
	int iWidthMask, iWidthShift;

	iWidthMask = oled_1107.oled_x - 1;
	iWidthShift = (oled_1107.oled_x == 128) ? 7 : 6; // 128 or 64 pixels wide
	if (pCurrent == NULL || pCurrent > pAnimation + iLen)
		return NULL; // invalid starting point

	s = (uint8_t*) pCurrent; // start of animation data
	i = 0;
	SH1107_SetPosition(0, 0, 1);
	while (i < iBufferSize) // run one frame
	{
		bCode = pgm_read_byte(s++);
		switch (bCode & OP_MASK)
		// different compression types
		{
			case OP_SKIPCOPY: // skip/copy
				if (bCode == OP_SKIPCOPY) // big skip
				{
					b = pgm_read_byte(s++);
					i += b + 1;
					SH1107_SetPosition(i & iWidthMask, (i >> iWidthShift), 1);
				}
				else // skip/copy
				{
					if (bCode & 0x38)
					{
						i += ((bCode & 0x38) >> 3); // skip amount
						SH1107_SetPosition(i & iWidthMask, (i >> iWidthShift), 1);
					}
					if (bCode & 7)
					{
						SH1107_WriteFlashBlock(s, bCode & 7);
						s += (bCode & 7);
						i += bCode & 7;
					}
				}
				break;
			case OP_COPYSKIP: // copy/skip
				if (bCode == OP_COPYSKIP) // big copy
				{
					b = pgm_read_byte(s++);
					j = b + 1;
					SH1107_WriteFlashBlock(s, j);
					s += j;
					i += j;
				}
				else
				{
					j = ((bCode & 0x38) >> 3);
					if (j)
					{
						SH1107_WriteFlashBlock(s, j);
						s += j;
						i += j;
					}
					if (bCode & 7)
					{
						i += (bCode & 7); // skip
						SH1107_SetPosition(i & iWidthMask, (i >> iWidthShift), 1);
					}
				}
				break;
			case OP_REPEATSKIP: // repeat/skip
				j = (bCode & 0x38) >> 3; // repeat count
				b = pgm_read_byte(s++);
				SH1107_RepeatByte(b, j);
				i += j;
				if (bCode & 7)
				{
					i += (bCode & 7); // skip amount
					SH1107_SetPosition(i & iWidthMask, (i >> iWidthShift), 1);
				}
				break;

			case OP_REPEAT:
				j = (bCode & 0x3f) + 1;
				b = pgm_read_byte(s++);
				SH1107_RepeatByte(b, j);
				i += j;
				break;
		} // switch on code type
	} // while rendering a frame
	if (s >= pAnimation + iLen) // we've hit the end, restart from the beginning
		s = pAnimation;
	return s; // return pointer to start of next frame
} /* SH1107_PlayAnimFrame() */

//
// Draw a sprite of any size in any position
// If it goes beyond the left/right or top/bottom edges
// it's trimmed to show the valid parts
// This function requires a back buffer to be defined
// The priority color (0 or 1) determines which color is painted 
// when a 1 is encountered in the source image. 
//
void SH1107_DrawSprite(uint8_t *pSprite, int cx, int cy, int iPitch, int x, int y,
		uint8_t iPriority)
{
	int tx, ty, dx, dy, iStartX;
	uint8_t *s, *d, uc, pix, ucSrcMask, ucDstMask;

	if (x + cx
			< 0|| y+cy < 0 || x >= oled_1107.oled_x || y >= oled_1107.oled_y || oled_1107.ucScreen == NULL)
		return; // no backbuffer or out of bounds
	dy = y; // destination y
	if (y < 0) // skip the invisible parts
	{
		cy += y;
		y = -y;
		pSprite += (y * iPitch);
		dy = 0;
	}
	if (y + cy > oled_1107.oled_y)
		cy = oled_1107.oled_y - y;
	iStartX = 0;
	dx = x;
	if (x < 0)
	{
		cx += x;
		x = -x;
		iStartX = x;
		dx = 0;
	}
	if (x + cx > oled_1107.oled_x)
		cx = oled_1107.oled_x - x;
	for (ty = 0; ty < cy; ty++)
	{
		s = &pSprite[iStartX >> 3];
		d = &oled_1107.ucScreen[(dy >> 3) * oled_1107.oled_x + dx];
		ucSrcMask = 0x80 >> (iStartX & 7);
		pix = *s++;
		ucDstMask = 1 << (dy & 7);
		if (iPriority) // priority color is 1
		{
			for (tx = 0; tx < cx; tx++)
			{
				uc = d[0];
				if (pix & ucSrcMask) // set pixel in source, set it in dest
					d[0] = (uc | ucDstMask);
				d++; // next pixel column
				ucSrcMask >>= 1;
				if (ucSrcMask == 0) // read next byte
				{
					ucSrcMask = 0x80;
					pix = *s++;
				}
			} // for tx
		} // priorty color 1
		else
		{
			for (tx = 0; tx < cx; tx++)
			{
				uc = d[0];
				if (pix & ucSrcMask) // clr pixel in source, clr it in dest
					d[0] = (uc & ~ucDstMask);
				d++; // next pixel column
				ucSrcMask >>= 1;
				if (ucSrcMask == 0) // read next byte
				{
					ucSrcMask = 0x80;
					pix = *s++;
				}
			} // for tx
		} // priority color 0
		dy++;
		pSprite += iPitch;
	} // for ty
} /* SH1107_DrawSprite() */

//
// Draw a 16x16 tile in any of 4 rotated positions
// Assumes input image is laid out like "normal" graphics with
// the MSB on the left and 2 bytes per line
// On AVR, the source image is assumed to be in FLASH memory
// The function can draw the tile on byte boundaries, so the x value
// can be from 0 to 112 and y can be from 0 to 6
//
void SH1107_DrawTile(const uint8_t *pTile, int x, int y, int iRotation, int bInvert, int bRender)
{
	uint8_t ucTemp[32]; // prepare LCD data here
	uint8_t i, j, k, iOffset, ucMask, uc, ucPixels;
	uint8_t bFlipX = 0, bFlipY = 0;

	if (x < 0 || y < 0 || y > 6 || x > 112)
		return; // out of bounds
	if (pTile == NULL)
		return; // bad pointer; really? :(
	if (iRotation == ANGLE_180 || iRotation == ANGLE_270 || iRotation == ANGLE_FLIPX)
		bFlipX = 1;
	if (iRotation == ANGLE_180 || iRotation == ANGLE_270 || iRotation == ANGLE_FLIPY)
		bFlipY = 1;

	memset(ucTemp, 0, sizeof(ucTemp)); // we only set white pixels, so start from black
	if (iRotation == ANGLE_0 || iRotation == ANGLE_180 || iRotation == ANGLE_FLIPX
			|| iRotation == ANGLE_FLIPY)
	{
		for (j = 0; j < 16; j++) // y
		{
			for (i = 0; i < 16; i += 8) // x
			{
				ucPixels = pgm_read_byte((uint8_t* )pTile++);
				ucMask = 0x80; // MSB is the first source pixel
				for (k = 0; k < 8; k++)
				{
					if (ucPixels & ucMask) // translate the pixel
					{
						if (bFlipY)
							uc = 0x80 >> (j & 7);
						else
							uc = 1 << (j & 7);
						iOffset = i + k;
						if (bFlipX)
							iOffset = 15 - iOffset;
						iOffset += (j & 8) << 1; // top/bottom half of output
						if (bFlipY)
							iOffset ^= 16;
						ucTemp[iOffset] |= uc;
					}
					ucMask >>= 1;
				} // for k
			} // for i
		} // for j
	}
	else // rotated 90/270
	{
		for (j = 0; j < 16; j++) // y
		{
			for (i = 0; i < 16; i += 8) // x
			{
				ucPixels = pgm_read_byte((uint8_t* )pTile++);
				ucMask = 0x80; // MSB is the first source pixel
				for (k = 0; k < 8; k++)
				{
					if (ucPixels & ucMask) // translate the pixel
					{
						if (bFlipY)
							uc = 0x80 >> k;
						else
							uc = 1 << k;
						iOffset = 15 - j;
						if (bFlipX)
							iOffset = 15 - iOffset;
						iOffset += i << 1; // top/bottom half of output
						if (bFlipY)
							iOffset ^= 16;
						ucTemp[iOffset] |= uc;
					}
					ucMask >>= 1;
				} // for k
			} // for i
		} // for j
	}
	if (bInvert)
		SH1107_InvertBytes(ucTemp, 32);
	// Send the data to the display
	SH1107_SetPosition(x, y, bRender);
	SH1107_WriteDataBlock(ucTemp, 16, bRender); // top half
	SH1107_SetPosition(x, y + 1, bRender);
	SH1107_WriteDataBlock(&ucTemp[16], 16, bRender); // bottom half
} /* SH1107_DrawTile() */

// Set (or clear) an individual pixel
// The local copy of the frame buffer is used to avoid
// reading data from the display controller

int I2CRead(uint8_t iAddr, uint8_t *pData, int iLen)
{
	int i = 0;
	Wire.requestFrom(iAddr, (uint8_t) iLen);
	while (i < iLen)
	{
		pData[i++] = Wire.read();
	}
	return (i > 0);
}

int SH1107_SetPixel(int x, int y, unsigned char ucColor, int bRender)
{
	int i;
	unsigned char uc, ucOld;

	i = ((y >> 3) * 128) + x;
	if (i < 0 || i > 1023) // off the screen
		return -1;
	SH1107_SetPosition(x, y >> 3, bRender);

	if (oled_1107.ucScreen)
		uc = ucOld = oled_1107.ucScreen[i];
	else // SH1107 can read data
	{
		uint8_t ucTemp[3];
		ucTemp[0] = 0x80; // one command
		ucTemp[1] = SH110X_READWRITE; // read_modify_write
		ucTemp[2] = 0xC0; // one data
		_I2CWrite(ucTemp, 3);

		// read a dummy byte followed by the data byte we want
		I2CRead(oled_1107.oled_addr, ucTemp, 2);
		uc = ucOld = ucTemp[1]; // first byte is garbage
	}

	uc &= ~(0x1 << (y & 7));
	if (ucColor)
	{
		uc |= (0x1 << (y & 7));
	}
	if (uc != ucOld) // pixel changed
	{
//    SH1107_SetPosition(x, y>>3);
		if (oled_1107.ucScreen)
		{
			SH1107_WriteDataBlock(&uc, 1, bRender);
			oled_1107.ucScreen[i] = uc;
		}
		else // end the read_modify_write operation
		{
			uint8_t ucTemp[4];
			ucTemp[0] = 0xc0; // one data
			ucTemp[1] = uc;   // actual data
			ucTemp[2] = 0x80; // one command
			ucTemp[3] = SH110X_READWRITEEND; // end read_modify_write operation
			_I2CWrite(ucTemp, 4);
		}
	}
	return 0;
} /* SH1107_SetPixel() */

//
// Load a 128x64 1-bpp Windows bitmap
// Pass the pointer to the beginning of the BMP file
// First pass version assumes a full screen bitmap
//
int SH1107_LoadBMP(uint8_t *pBMP, int bInvert, int bRender)
{
	int16_t i16;
	int iOffBits, q, y, j; // offset to bitmap data
	int iPitch;
	uint8_t x, z, b, *s;
	uint8_t dst_mask;
	uint8_t ucTemp[16]; // process 16 bytes at a time
	uint8_t bFlipped = false;

	i16 = pgm_read_word(pBMP);
	if (i16 != 0x4d42) // must start with 'BM'
		return -1; // not a BMP file
	i16 = pgm_read_word(pBMP + 18);
	if (i16 != 128) // must be 128 pixels wide
		return -1;
	i16 = pgm_read_word(pBMP + 22);
	if (i16 != 64 && i16 != -64) // must be 64 pixels tall
		return -1;
	if (i16 == 64) // BMP is flipped vertically (typical)
		bFlipped = true;
	i16 = pgm_read_word(pBMP + 28);
	if (i16 != 1) // must be 1 bit per pixel
		return -1;
	iOffBits = pgm_read_word(pBMP + 10);
	iPitch = 16;
	if (bFlipped)
	{
		iPitch = -16;
		iOffBits += (63 * 16); // start from bottom
	}

// rotate the data and send it to the display
	for (y = 0; y < 8; y++) // 8 lines of 8 pixels
	{
		SH1107_SetPosition(0, y, bRender);
		for (j = 0; j < 8; j++) // do 8 sections of 16 columns
		{
			s = &pBMP[iOffBits + (j * 2) + (y * iPitch * 8)]; // source line
			memset(ucTemp, 0, 16); // start with all black
			for (x = 0; x < 16; x += 8) // do each block of 16x8 pixels
			{
				dst_mask = 1;
				for (q = 0; q < 8; q++) // gather 8 rows
				{
					b = pgm_read_byte(s + (q * iPitch));
					for (z = 0; z < 8; z++) // gather up the 8 bits of this column
					{
						if (b & 0x80)
							ucTemp[x + z] |= dst_mask;
						b <<= 1;
					} // for z
					dst_mask <<= 1;
				} // for q
				s++; // next source byte
			} // for x
			if (bInvert)
				SH1107_InvertBytes(ucTemp, 16);
			SH1107_WriteDataBlock(ucTemp, 16, bRender);
		} // for j
	} // for y
	return 0;
} /* SH1107_LoadBMP() */

//
// Set the current cursor position
// The column represents the pixel column (0-127)
// The row represents the text row (0-7)
//
void SH1107_SetCursor(int x, int y)
{
	oled_1107.iCursorX = x;
	oled_1107.iCursorY = y;
} /* SH1107_SetCursor() */

//
// Turn text wrap on or off for the oldWriteString() function
//
void SH1107_SetTextWrap(int bWrap)
{
	oled_1107.oled_wrap = bWrap;
} /* SH1107_SetTextWrap() */

//
// Draw a string of normal (8x8), small (6x8) or large (16x32) characters
// At the given col+row. x and y is in font char unit, first line is 0.
//
int SH1107_WriteString(int iScroll, int x, int y, char *szMsg, int iSize, int bInvert, int bRender)
{
	int i, iFontOff, iLen, iFontSkip, result;
	unsigned char c, *s, ucTemp[40];

	if (x == -1 || y == -1) // use the cursor position
	{
		x = oled_1107.iCursorX;
		y = oled_1107.iCursorY;
	}
	else
	{
		oled_1107.iCursorX = x;
		oled_1107.iCursorY = y; // set the new cursor position
	}
	if (oled_1107.iCursorX >= oled_1107.oled_x || oled_1107.iCursorY >= oled_1107.oled_y / 8)
		return -1; // can't draw off the display

	SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
	result = 0;
	i = 0;
	switch (iSize)
	{
		case FONT_8x8: // 8x8 font
		{
			iFontSkip = iScroll & 7; // number of columns to initially skip
			while ((szMsg[i] != 0) && (oled_1107.iCursorX < oled_1107.oled_x)
					&& (oled_1107.iCursorY < oled_1107.oled_y / 8))
			{
				if (iScroll < 8) // only display visible characters
				{
					c = (unsigned char) szMsg[i];
					iFontOff = (int) (c - 32) * 7;
					// we can't directly use the pointer to FLASH memory, so copy to a local buffer
					ucTemp[0] = 0;
					memcpy_P(&ucTemp[1], &ucFont[iFontOff], 7);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 8);
					//         SH1107_CachedWrite(ucTemp, 8);
					iLen = 8 - iFontSkip;
					if (oled_1107.iCursorX + iLen > oled_1107.oled_x) // clip right edge
						iLen = oled_1107.oled_x - oled_1107.iCursorX;
					SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					oled_1107.iCursorX += iLen;
					if (oled_1107.iCursorX >= oled_1107.oled_x - 7 && oled_1107.oled_wrap) // word wrap enabled?
					{
						oled_1107.iCursorX = 0; // start at the beginning of the next line
						oled_1107.iCursorY++;
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					}
					iFontSkip = 0;
				}
				iScroll -= 8;
				i++;
			} // while
//     SH1107_CachedFlush(); // write any remaining data
			break;
		} // 8x8

		case FONT_16x32: // 16x32 font
		{
			iFontSkip = iScroll & 15; // number of columns to initially skip
			while ((szMsg[i] != 0) && (oled_1107.iCursorX < oled_1107.oled_x)
					&& (oled_1107.iCursorY < (oled_1107.oled_y / 8) - 3))
			{
				if (iScroll < 16) // if characters are visible
				{
					s = (unsigned char*) &ucBigFont[(unsigned char) (szMsg[i] - 32) * 64];
					iLen = 16 - iFontSkip;
					if (oled_1107.iCursorX + iLen > oled_1107.oled_x) // clip right edge
						iLen = oled_1107.oled_x - oled_1107.iCursorX;
					// we can't directly use the pointer to FLASH memory, so copy to a local buffer
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					memcpy_P(ucTemp, s, 16);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 16);
					SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY + 1, bRender);
					memcpy_P(ucTemp, s + 16, 16);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 16);
					SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					if (oled_1107.iCursorY <= 5)
					{
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY + 2, bRender);
						memcpy_P(ucTemp, s + 32, 16);
						if (bInvert)
							SH1107_InvertBytes(ucTemp, 16);
						SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					}
					if (oled_1107.iCursorY <= 4)
					{
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY + 3, bRender);
						memcpy_P(ucTemp, s + 48, 16);
						if (bInvert)
							SH1107_InvertBytes(ucTemp, 16);
						SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					}
					oled_1107.iCursorX += iLen;
					if (oled_1107.iCursorX >= oled_1107.oled_x - 15 && oled_1107.oled_wrap) // word wrap enabled?
					{
						oled_1107.iCursorX = 0; // start at the beginning of the next line
						oled_1107.iCursorY += 4;
					}
					iFontSkip = 0;
				} // if character visible from scrolling
				iScroll -= 16;
				i++;
			} // while
			break;
		} // 16x32

		case FONT_12x16: // 6x8 stretched to 12x16
		{
			iFontSkip = iScroll % 12; // number of columns to initially skip
			while ((szMsg[i] != 0) && (oled_1107.iCursorX < oled_1107.oled_x)
					&& (oled_1107.iCursorY < (oled_1107.oled_y / 8) - 1))
			{
// stretch the 'normal' font instead of using the big font
				if (iScroll < 12) // if characters are visible
				{
					int tx, ty;
					c = szMsg[i] - 32;
					unsigned char uc1, uc2, ucMask, *pDest;
					s = (unsigned char*) &ucSmallFont[(int) c * 5];
					ucTemp[0] = 0; // first column is blank
					memcpy_P(&ucTemp[1], s, 6);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 6);
					// Stretch the font to double width + double height
					memset(&ucTemp[6], 0, 24); // write 24 new bytes
					for (tx = 0; tx < 6; tx++)
					{
						ucMask = 3;
						pDest = &ucTemp[6 + tx * 2];
						uc1 = uc2 = 0;
						c = ucTemp[tx];
						for (ty = 0; ty < 4; ty++)
						{
							if (c & (1 << ty)) // a bit is set
								uc1 |= ucMask;
							if (c & (1 << (ty + 4)))
								uc2 |= ucMask;
							ucMask <<= 2;
						}
						pDest[0] = uc1;
						pDest[1] = uc1; // double width
						pDest[12] = uc2;
						pDest[13] = uc2;
					}
					// smooth the diagonal lines
					for (tx = 0; tx < 5; tx++)
					{
						uint8_t c0, c1, ucMask2;
						c0 = ucTemp[tx];
						c1 = ucTemp[tx + 1];
						pDest = &ucTemp[6 + tx * 2];
						ucMask = 1;
						ucMask2 = 2;
						for (ty = 0; ty < 7; ty++)
						{
							if (((c0 & ucMask) && !(c1 & ucMask) && !(c0 & ucMask2) && (c1 & ucMask2))
									|| (!(c0 & ucMask) && (c1 & ucMask) && (c0 & ucMask2) && !(c1 & ucMask2)))
							{
								if (ty < 3) // top half
								{
									pDest[1] |= (1 << ((ty * 2) + 1));
									pDest[2] |= (1 << ((ty * 2) + 1));
									pDest[1] |= (1 << ((ty + 1) * 2));
									pDest[2] |= (1 << ((ty + 1) * 2));
								}
								else
									if (ty == 3) // on the border
									{
										pDest[1] |= 0x80;
										pDest[2] |= 0x80;
										pDest[13] |= 1;
										pDest[14] |= 1;
									}
									else // bottom half
									{
										pDest[13] |= (1 << (2 * (ty - 4) + 1));
										pDest[14] |= (1 << (2 * (ty - 4) + 1));
										pDest[13] |= (1 << ((ty - 3) * 2));
										pDest[14] |= (1 << ((ty - 3) * 2));
									}
							}
							else
								if (!(c0 & ucMask) && (c1 & ucMask) && (c0 & ucMask2) && !(c1 & ucMask2))
								{
									if (ty < 4) // top half
									{
										pDest[1] |= (1 << ((ty * 2) + 1));
										pDest[2] |= (1 << ((ty + 1) * 2));
									}
									else
									{
										pDest[13] |= (1 << (2 * (ty - 4) + 1));
										pDest[14] |= (1 << ((ty - 3) * 2));
									}
								}
							ucMask <<= 1;
							ucMask2 <<= 1;
						}
					}
					iLen = 12 - iFontSkip;
					if (oled_1107.iCursorX + iLen > oled_1107.oled_x) // clip right edge
						iLen = oled_1107.oled_x - oled_1107.iCursorX;
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					SH1107_WriteDataBlock(&ucTemp[6 + iFontSkip], iLen, bRender);
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY + 1, bRender);
					SH1107_WriteDataBlock(&ucTemp[18 + iFontSkip], iLen, bRender);
					oled_1107.iCursorX += iLen;
					if (oled_1107.iCursorX >= oled_1107.oled_x - 11 && oled_1107.oled_wrap) // word wrap enabled?
					{
						oled_1107.iCursorX = 0; // start at the beginning of the next line
						oled_1107.iCursorY += 2;
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					}
					iFontSkip = 0;
				} // if characters are visible
				iScroll -= 12;
				i++;
			} // while
			break;
		} // 12x16

		case FONT_16x16: // 8x8 stretched to 16x16
		{
			iFontSkip = iScroll & 15; // number of columns to initially skip
			while ((szMsg[i] != 0) && (oled_1107.iCursorX < oled_1107.oled_x)
					&& (oled_1107.iCursorY < (oled_1107.oled_y / 8) - 1))
			{
// stretch the 'normal' font instead of using the big font
				if (iScroll < 16) // if characters are visible
				{
					int tx, ty;
					c = szMsg[i] - 32;
					unsigned char uc1, uc2, ucMask, *pDest;
					s = (unsigned char*) &ucFont[(int) c * 7];
					ucTemp[0] = 0;
					memcpy_P(&ucTemp[1], s, 7);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 8);
					// Stretch the font to double width + double height
					memset(&ucTemp[8], 0, 32); // write 32 new bytes
					for (tx = 0; tx < 8; tx++)
					{
						ucMask = 3;
						pDest = &ucTemp[8 + tx * 2];
						uc1 = uc2 = 0;
						c = ucTemp[tx];
						for (ty = 0; ty < 4; ty++)
						{
							if (c & (1 << ty)) // a bit is set
								uc1 |= ucMask;
							if (c & (1 << (ty + 4)))
								uc2 |= ucMask;
							ucMask <<= 2;
						}
						pDest[0] = uc1;
						pDest[1] = uc1; // double width
						pDest[16] = uc2;
						pDest[17] = uc2;
					}
					iLen = 16 - iFontSkip;
					if (oled_1107.iCursorX + iLen > oled_1107.oled_x) // clip right edge
						iLen = oled_1107.oled_x - oled_1107.iCursorX;
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					SH1107_WriteDataBlock(&ucTemp[8 + iFontSkip], iLen, bRender);
					SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY + 1, bRender);
					SH1107_WriteDataBlock(&ucTemp[24 + iFontSkip], iLen, bRender);
					oled_1107.iCursorX += iLen;
					if (oled_1107.iCursorX >= oled_1107.oled_x - 15 && oled_1107.oled_wrap) // word wrap enabled?
					{
						oled_1107.iCursorX = 0; // start at the beginning of the next line
						oled_1107.iCursorY += 2;
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					}
					iFontSkip = 0;
				} // if characters are visible
				iScroll -= 16;
				i++;
			} // while
			break;
		} // 16x16

		case FONT_6x8: // 6x8 font
		{
			iFontSkip = iScroll % 6;
			while ((szMsg[i] != 0) && (oled_1107.iCursorX < oled_1107.oled_x)
					&& (oled_1107.iCursorY < (oled_1107.oled_y / 8)))
			{
				if (iScroll < 6) // if characters are visible
				{
					c = szMsg[i] - 32;
					// we can't directly use the pointer to FLASH memory, so copy to a local buffer
					ucTemp[0] = 0;
					memcpy_P(&ucTemp[1], &ucSmallFont[(int) c * 5], 5);
					if (bInvert)
						SH1107_InvertBytes(ucTemp, 6);
					iLen = 6 - iFontSkip;
					if (oled_1107.iCursorX + iLen > oled_1107.oled_x) // clip right edge
						iLen = oled_1107.oled_x - oled_1107.iCursorX;
					SH1107_WriteDataBlock(&ucTemp[iFontSkip], iLen, bRender); // write character pattern
					//         SH1107_CachedWrite(ucTemp, 6);
					oled_1107.iCursorX += iLen;
					iFontSkip = 0;
					if (oled_1107.iCursorX >= oled_1107.oled_x - 5 && oled_1107.oled_wrap) // word wrap enabled?
					{
						oled_1107.iCursorX = 0; // start at the beginning of the next line
						oled_1107.iCursorY++;
						SH1107_SetPosition(oled_1107.iCursorX, oled_1107.iCursorY, bRender);
					}
				} // if characters are visible
				iScroll -= 6;
				i++;
			}
//    SH1107_CachedFlush(); // write any remaining data      
			break;
		} // 6x8

		default:
			result = -1;
	}

	return result;
} /* SH1107_WriteString() */

//
// Render a sprite/rectangle of pixels from a provided buffer to the display.
// The row values refer to byte rows, not pixel rows due to the memory
// layout of OLEDs.
// returns 0 for success, -1 for invalid parameter
//
int SH1107_DrawGFX(uint8_t *pBuffer, int iSrcCol, int iSrcRow, int iDestCol, int iDestRow,
		int iWidth, int iHeight, int iSrcPitch)
{
	int y;

	if (iSrcCol < 0 || iSrcCol > 127 || iSrcRow < 0 || iSrcRow > 7 || iDestCol < 0
			|| iDestCol >= oled_1107.oled_x || iDestRow < 0 || iDestRow >= (oled_1107.oled_y >> 3)
			|| iSrcPitch <= 0)
		return -1; // invalid

	for (y = iSrcRow; y < iSrcRow + iHeight; y++)
	{
		uint8_t *s = &pBuffer[(y * iSrcPitch) + iSrcCol];
		SH1107_SetPosition(iDestCol, iDestRow, 1);
		SH1107_WriteDataBlock(s, iWidth, 1);
		pBuffer += iSrcPitch;
		iDestRow++;
	} // for y
	return 0;
} /* SH1107_DrawGFX() */

#ifdef ROTATED_90
//
// Rotate the buffer
//
void SH1107_Rotate90(uint8_t *pSource)
{
	uint16_t src_addr, dst_addr;
	uint8_t src_bit, dst_bit, src_pixel;
	for (uint8_t x = 0; x < oled_1107.oled_x; x++)
	{
		for (uint8_t y = 0; y < oled_1107.oled_y; y++)
		{
			src_addr = x + (y / 8) * oled_1107.oled_x;
			src_bit = y & 0x07;
			dst_addr = oled_1107.oled_y - 1 - y + (x / 8) * oled_1107.oled_y;
			dst_bit = x & 0x07;

			src_pixel = (pSource[src_addr] >> src_bit) & 0x01;
			rotBuffer[dst_addr] &= ~(1 << dst_bit);
			rotBuffer[dst_addr] |= (src_pixel << dst_bit);
		}
	}
}
#endif

//
// Dump a screen's worth of data directly to the display
//
void SH1107_DumpBuffer(uint8_t *pBuffer)
{
	if (!DisplayIsOn)
		return;

#ifdef ROTATED_90
	static uint8_t saveBuffer[BUFFER_SIZE];  // rotated buffer
	bool needRestaure = false;
	if (pBuffer != NULL)
	  SH1107_Rotate90(pBuffer);
	else
	{
		mempcpy(saveBuffer, oled_1107.ucScreen, BUFFER_SIZE);
		needRestaure = true;
		SH1107_Rotate90(oled_1107.ucScreen);
	}
	pBuffer = rotBuffer;
#endif

	if (pBuffer == NULL) // dump the internal buffer if none is given
		pBuffer = oled_1107.ucScreen;
	if (pBuffer == NULL)
		return; // no backbuffer and no provided buffer

	int iLines = oled_1107.oled_y >> 3;
	int iCols = oled_1107.oled_x >> 4;
	for (int y = 0; y < iLines; y++)
	{
		for (int x = 0; x < iCols; x++) // wiring library has a 32-byte buffer, so send 16 bytes so that the data prefix (0x40) can fit
		{
			SH1107_SetPosition(x * 16, y, 1);
			SH1107_WriteDataBlock(pBuffer, 16, 1);

			pBuffer += 16;
		} // for x
	} // for y

#ifdef ROTATED_90
	if (needRestaure)
		mempcpy(oled_1107.ucScreen, saveBuffer, BUFFER_SIZE);
#endif
} /* SH1107_DumpBuffer() */

//
// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
//
void SH1107_Fill(unsigned char ucData, int bRender)
{
	uint8_t x, y;
	uint8_t iLines, iCols;
	unsigned char temp[16];

	iLines = oled_1107.oled_y >> 3;
	iCols = oled_1107.oled_x >> 4;
	memset(temp, ucData, 16);
	oled_1107.iCursorX = oled_1107.iCursorY = 0;

	if (bRender)
	{
		for (y = 0; y < iLines; y++)
		{
			SH1107_SetPosition(0, y, bRender); // set to (0,Y)
			for (x = 0; x < iCols; x++) // wiring library has a 32-byte buffer, so send 16 bytes so that the data prefix (0x40) can fit
			{
				SH1107_WriteDataBlock(temp, 16, bRender);
			} // for x
		} // for y
	}

	if (oled_1107.ucScreen)
	{
		memset(oled_1107.ucScreen, ucData, BUFFER_SIZE);
	}
} /* SH1107_Fill() */

void SH1107_DrawLine(int x1, int y1, int x2, int y2, int bRender)
{
	int temp;
	int dx = x2 - x1;
	int dy = y2 - y1;
	int error;
	uint8_t *p, *pStart, mask, bOld, bNew;
	int xinc, yinc;
	int y, x;

	if (oled_1107.ucScreen == NULL)
		return;

	if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 || x1 >= oled_1107.oled_x || x2 >= oled_1107.oled_x
			|| y1 >= oled_1107.oled_y || y2 >= oled_1107.oled_y)
		return;

	if (abs(dx) > abs(dy))
	{
		// X major case
		if (x2 < x1)
		{
			dx = -dx;
			temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}

		y = y1;
		dy = (y2 - y1);
		error = dx >> 1;
		yinc = 1;
		if (dy < 0)
		{
			dy = -dy;
			yinc = -1;
		}
		p = pStart = &oled_1107.ucScreen[x1 + ((y >> 3) << 7)]; // point to current spot in back buffer
		mask = 1 << (y & 7); // current bit offset
		for (x = x1; x1 <= x2; x1++)
		{
			*p++ |= mask; // set pixel and increment x pointer
			error -= dy;
			if (error < 0)
			{
				error += dx;
				if (yinc > 0)
					mask <<= 1;
				else
					mask >>= 1;
				if (mask == 0) // we've moved outside the current row, write the data we changed
				{
					SH1107_SetPosition(x, y >> 3, bRender);
					SH1107_WriteDataBlock(pStart, (int) (p - pStart), bRender); // write the row we changed
					x = x1 + 1; // we've already written the byte at x1
					y1 = y + yinc;
					p += (yinc > 0) ? 128 : -128;
					pStart = p;
					mask = 1 << (y1 & 7);
				}
				y += yinc;
			}
		} // for x1
		if (p != pStart) // some data needs to be written
		{
			SH1107_SetPosition(x, y >> 3, bRender);
			SH1107_WriteDataBlock(pStart, (int) (p - pStart), bRender);
		}
	}
	else
	{
		// Y major case
		if (y1 > y2)
		{
			dy = -dy;
			temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}

		p = &oled_1107.ucScreen[x1 + ((y1 >> 3) * 128)]; // point to current spot in back buffer
		bOld = bNew = p[0]; // current data at that address
		mask = 1 << (y1 & 7); // current bit offset
		dx = (x2 - x1);
		error = dy >> 1;
		xinc = 1;
		if (dx < 0)
		{
			dx = -dx;
			xinc = -1;
		}
		for (x = x1; y1 <= y2; y1++)
		{
			bNew |= mask; // set the pixel
			error -= dx;
			mask <<= 1; // y1++
			if (mask == 0) // we're done with this byte, write it if necessary
			{
				if (bOld != bNew)
				{
					p[0] = bNew; // save to RAM
					SH1107_SetPosition(x, y1 >> 3, bRender);
					SH1107_WriteDataBlock(&bNew, 1, bRender);
				}
				p += 128; // next line
				bOld = bNew = p[0];
				mask = 1; // start at LSB again
			}
			if (error < 0)
			{
				error += dy;
				if (bOld != bNew) // write the last byte we modified if it changed
				{
					p[0] = bNew; // save to RAM
					SH1107_SetPosition(x, y1 >> 3, bRender);
					SH1107_WriteDataBlock(&bNew, 1, bRender);
				}
				p += xinc;
				x += xinc;
				bOld = bNew = p[0];
			}
		} // for y
		if (bOld != bNew) // write the last byte we modified if it changed
		{
			p[0] = bNew; // save to RAM
			SH1107_SetPosition(x, y2 >> 3, bRender);
			SH1107_WriteDataBlock(&bNew, 1, bRender);
		}
	} // y major case
} /* SH1107_DrawLine() */

//
// Draw a string with a fractional scale in both dimensions
// the scale is a 16-bit integer with and 8-bit fraction and 8-bit mantissa
// To draw at 1x scale, set the scale factor to 256. To draw at 2x, use 512
// The output must be drawn into a memory buffer, not directly to the display
//
int SH1107_ScaledString(int x, int y, char *szMsg, int iSize, int bInvert, int iXScale, int iYScale,
		int iRotation)
{
	uint32_t row, col, dx, dy;
	uint32_t sx, sy;
	uint8_t c, uc, color, *d;
	const uint8_t *s;
	uint8_t ucTemp[16];
	int tx, ty, bit, iFontOff;
	int iPitch;
	int iFontWidth;

	if (iXScale == 0 || iYScale == 0 || szMsg == NULL || oled_1107.ucScreen == NULL || x < 0 || y < 0
			|| x >= oled_1107.oled_x - 1 || y >= oled_1107.oled_y - 1)
		return -1; // invalid display structure
	if (iSize != FONT_8x8 && iSize != FONT_6x8)
		return -1; // only on the small fonts (for now)
	iFontWidth = (iSize == FONT_6x8) ? 6 : 8;
	s = (iSize == FONT_6x8) ? ucSmallFont : ucFont;
	iPitch = oled_1107.oled_x;
	dx = (iFontWidth * iXScale) >> 8; // width of each character
	dy = (8 * iYScale) >> 8; // height of each character
	sx = 65536 / iXScale; // turn the scale into an accumulator value
	sy = 65536 / iYScale;
	while (*szMsg)
	{
		c = *szMsg++; // debug - start with normal font
		iFontOff = (int) (c - 32) * (iFontWidth - 1);
		// we can't directly use the pointer to FLASH memory, so copy to a local buffer
		ucTemp[0] = 0; // first column is blank
		memcpy_P(&ucTemp[1], &s[iFontOff], iFontWidth - 1);
		if (bInvert)
			SH1107_InvertBytes(ucTemp, iFontWidth);
		col = 0;
		for (tx = 0; tx < (int) dx; tx++)
		{
			row = 0;
			uc = ucTemp[col >> 8];
			for (ty = 0; ty < (int) dy; ty++)
			{
				int nx = 0, ny = 0;
				bit = row >> 8;
				color = (uc & (1 << bit)); // set or clear the pixel
				switch (iRotation)
				{
					case ROT_0:
						nx = x + tx;
						ny = y + ty;
						break;
					case ROT_90:
						nx = x - ty;
						ny = y + tx;
						break;
					case ROT_180:
						nx = x - tx;
						ny = y - ty;
						break;
					case ROT_270:
						nx = x + ty;
						ny = y - tx;
						break;
				} // switch on rotation direction
				// plot the pixel if it's within the image boundaries
				if (nx >= 0 && ny >= 0 && nx < oled_1107.oled_x && ny < oled_1107.oled_y)
				{
					d = &oled_1107.ucScreen[(ny >> 3) * iPitch + nx];
					if (color)
						d[0] |= (1 << (ny & 7));
					else
						d[0] &= ~(1 << (ny & 7));
				}
				row += sy; // add fractional increment to source row of character
			} // for ty
			col += sx; // add fractional increment to source column
		} // for tx
		// update the 'cursor' position
		switch (iRotation)
		{
			case ROT_0:
				x += dx;
				break;
			case ROT_90:
				y += dx;
				break;
			case ROT_180:
				x -= dx;
				break;
			case ROT_270:
				y -= dx;
				break;
		} // switch on rotation
	} // while (*szMsg)
	return 0;
} /* SH1107_ScaledString() */

//
// For drawing ellipses, a circle is drawn and the x and y pixels are scaled by a 16-bit integer fraction
// This function draws a single pixel and scales its position based on the x/y fraction of the ellipse
//
static void DrawScaledPixel(int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac,
		uint8_t ucColor)
{
	uint8_t *d, ucMask;

	if (oled_1107.ucScreen == NULL)
		return;

	if (iXFrac != 0x10000)
		x = ((x * iXFrac) >> 16);
	if (iYFrac != 0x10000)
		y = ((y * iYFrac) >> 16);
	x += iCX;
	y += iCY;
	if (x < 0 || x >= oled_1107.oled_x || y < 0 || y >= oled_1107.oled_y)
		return; // off the screen
	d = &oled_1107.ucScreen[((y >> 3) * 128) + x];
	ucMask = 1 << (y & 7);
	if (ucColor)
		*d |= ucMask;
	else
		*d &= ~ucMask;
} /* DrawScaledPixel() */

//
// For drawing filled ellipses
//
static void DrawScaledLine(int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac,
		uint8_t ucColor)
{
	int iLen, x2;

	if (oled_1107.ucScreen == NULL)
		return;

	uint8_t *d, ucMask;
	if (iXFrac != 0x10000)
		x = ((x * iXFrac) >> 16);
	if (iYFrac != 0x10000)
		y = ((y * iYFrac) >> 16);
	iLen = x * 2;
	x = iCX - x;
	y += iCY;
	x2 = x + iLen;
	if (y < 0 || y >= oled_1107.oled_y)
		return; // completely off the screen
	if (x < 0)
		x = 0;
	if (x2 >= oled_1107.oled_x)
		x2 = oled_1107.oled_x - 1;
	iLen = x2 - x + 1; // new length
	d = &oled_1107.ucScreen[((y >> 3) * 128) + x];
	ucMask = 1 << (y & 7);
	if (ucColor) // white
	{
		for (; iLen > 0; iLen--)
			*d++ |= ucMask;
	}
	else // black
	{
		for (; iLen > 0; iLen--)
			*d++ &= ~ucMask;
	}
} /* DrawScaledLine() */

//
// Draw the 8 pixels around the Bresenham circle
// (scaled to make an ellipse)
//
static void BresenhamCircle(int iCX, int iCY, int x, int y, int32_t iXFrac, int32_t iYFrac,
		uint8_t ucColor, uint8_t bFill)
{
	if (oled_1107.ucScreen == NULL)
		return;

	if (bFill) // draw a filled ellipse
	{
		// for a filled ellipse, draw 4 lines instead of 8 pixels
		DrawScaledLine(iCX, iCY, x, y, iXFrac, iYFrac, ucColor);
		DrawScaledLine(iCX, iCY, x, -y, iXFrac, iYFrac, ucColor);
		DrawScaledLine(iCX, iCY, y, x, iXFrac, iYFrac, ucColor);
		DrawScaledLine(iCX, iCY, y, -x, iXFrac, iYFrac, ucColor);
	}
	else // draw 8 pixels around the edges
	{
		DrawScaledPixel(iCX, iCY, x, y, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, -x, y, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, x, -y, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, -x, -y, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, y, x, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, -y, x, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, y, -x, iXFrac, iYFrac, ucColor);
		DrawScaledPixel(iCX, iCY, -y, -x, iXFrac, iYFrac, ucColor);
	}
} /* BresenhamCircle() */

//
// Draw an outline or filled ellipse
//
void SH1107_Ellipse(int iCenterX, int iCenterY, int32_t iRadiusX, int32_t iRadiusY, uint8_t ucColor,
		uint8_t bFilled)
{
	int32_t iXFrac, iYFrac;
	int iRadius, iDelta, x, y;

	if (oled_1107.ucScreen == NULL)
		return; // must have back buffer defined

	if (iRadiusX <= 0 || iRadiusY <= 0)
		return; // invalid radii

	if (iRadiusX > iRadiusY) // use X as the primary radius
	{
		iRadius = iRadiusX;
		iXFrac = 65536;
		iYFrac = (iRadiusY * 65536) / iRadiusX;
	}
	else
	{
		iRadius = iRadiusY;
		iXFrac = (iRadiusX * 65536) / iRadiusY;
		iYFrac = 65536;
	}
	iDelta = 3 - (2 * iRadius);
	x = 0;
	y = iRadius;
	while (x <= y)
	{
		BresenhamCircle(iCenterX, iCenterY, x, y, iXFrac, iYFrac, ucColor, bFilled);
		x++;
		if (iDelta < 0)
		{
			iDelta += (4 * x) + 6;
		}
		else
		{
			iDelta += 4 * (x - y) + 10;
			y--;
		}
	}
} /* SH1107_Ellipse() */

//
// Draw an outline or filled rectangle
//
void SH1107_Rectangle(int x1, int y1, int x2, int y2, uint8_t ucColor, uint8_t bFilled)
{
	uint8_t *d, ucMask, ucMask2;
	int tmp, iOff;

	if (oled_1107.ucScreen == NULL)
		return; // only works with a back buffer

	if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || x1 >= oled_1107.oled_x || y1 >= oled_1107.oled_y
			|| x2 >= oled_1107.oled_x || y2 >= oled_1107.oled_y)
		return; // invalid coordinates
	// Make sure that X1/Y1 is above and to the left of X2/Y2
	// swap coordinates as needed to make this true
	if (x2 < x1)
	{
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	if (y2 < y1)
	{
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	if (bFilled)
	{
		int x, y, iMiddle;
		iMiddle = (y2 >> 3) - (y1 >> 3);
		ucMask = 0xff << (y1 & 7);
		if (iMiddle == 0) // top and bottom lines are in the same row
			ucMask &= (0xff >> (7 - (y2 & 7)));
		d = &oled_1107.ucScreen[(y1 >> 3) * 128 + x1];
		// Draw top
		for (x = x1; x <= x2; x++)
		{
			if (ucColor)
				*d |= ucMask;
			else
				*d &= ~ucMask;
			d++;
		}
		if (iMiddle > 1) // need to draw middle part
		{
			ucMask = (ucColor) ? 0xff : 0x00;
			for (y = 1; y < iMiddle; y++)
			{
				d = &oled_1107.ucScreen[(y1 >> 3) * 128 + x1 + (y * 128)];
				for (x = x1; x <= x2; x++)
					*d++ = ucMask;
			}
		}
		if (iMiddle >= 1) // need to draw bottom part
		{
			ucMask = 0xff >> (7 - (y2 & 7));
			d = &oled_1107.ucScreen[(y2 >> 3) * 128 + x1];
			for (x = x1; x <= x2; x++)
			{
				if (ucColor)
					*d++ |= ucMask;
				else
					*d++ &= ~ucMask;
			}
		}
	}
	else // outline
	{
		// see if top and bottom lines are within the same byte rows
		d = &oled_1107.ucScreen[(y1 >> 3) * 128 + x1];
		if ((y1 >> 3) == (y2 >> 3))
		{
			ucMask2 = 0xff << (y1 & 7);  // L/R end masks
			ucMask = 1 << (y1 & 7);
			ucMask |= 1 << (y2 & 7);
			ucMask2 &= (0xff >> (7 - (y2 & 7)));
			if (ucColor)
			{
				*d++ |= ucMask2; // start
				x1++;
				for (; x1 < x2; x1++)
					*d++ |= ucMask;
				if (x1 <= x2)
					*d++ |= ucMask2; // right edge
			}
			else
			{
				*d++ &= ~ucMask2;
				x1++;
				for (; x1 < x2; x1++)
					*d++ &= ~ucMask;
				if (x1 <= x2)
					*d++ &= ~ucMask2; // right edge
			}
		}
		else
		{
			int y;
			// L/R sides
			iOff = (x2 - x1);
			ucMask = 1 << (y1 & 7);
			for (y = y1; y <= y2; y++)
			{
				if (ucColor)
				{
					*d |= ucMask;
					d[iOff] |= ucMask;
				}
				else
				{
					*d &= ~ucMask;
					d[iOff] &= ~ucMask;
				}
				ucMask <<= 1;
				if (ucMask == 0)
				{
					ucMask = 1;
					d += 128;
				}
			}
			// T/B sides
			ucMask = 1 << (y1 & 7);
			ucMask2 = 1 << (y2 & 7);
			x1++;
			d = &oled_1107.ucScreen[(y1 >> 3) * 128 + x1];
			iOff = (y2 >> 3) - (y1 >> 3);
			iOff *= 128;
			for (; x1 < x2; x1++)
			{
				if (ucColor)
				{
					*d |= ucMask;
					d[iOff] |= ucMask2;
				}
				else
				{
					*d &= ~ucMask;
					d[iOff] &= ~ucMask2;
				}
				d++;
			}
		}
	} // outline
} /* SH1107_Rectangle() */

/********************************************************************************
 function:	Toggle display
 ********************************************************************************/

/**
 * Put Display ON
 */
void SH1107_ON(void)
{
	SH1107_Power(true);

	// Refresh screen
	SH1107_DumpBuffer();
}

/**
 * Put Display OFF
 */
void SH1107_OFF(void)
{
	SH1107_Power(false);
}

/**
 * Toggle Display On <-> Off
 */
void SH1107_ToggleOnOff(void)
{
	if (DisplayIsOn)
	{
		SH1107_OFF();
	}
	else
	{
		SH1107_ON();
	}
}

// ********************************************************************************
// End of file
// ********************************************************************************

