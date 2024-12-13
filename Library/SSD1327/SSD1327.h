/******************************************************************************
 * | file      	:	SSD1327.h
 * | version	:	V1.0
 * | date	:	2017-11-09
 * | function	:	SSD1327 Drive function
 * | url		:	https://www.waveshare.com/wiki/1.5inch_OLED_Module
 
 note:
 Image scanning:
 Please use progressive scanning to generate images or fonts
 ******************************************************************************/
#pragma once

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "Arduino.h"
#include "stdint.h"
#include "fonts_8_24.h"  // For GUI part

// Define to rotate de screen 90°
//#define	OLED_TOP_DOWN
//#define	OLED_LEFT_RIGHT
//#define	OLED_DOWN_TOP
//#define	OLED_RIGHT_LEFT

#if ((!defined(OLED_TOP_DOWN)) && (!defined(OLED_LEFT_RIGHT)) && \
		 (!defined(OLED_DOWN_TOP)) && (!defined(OLED_RIGHT_LEFT)))
#warning "Display orientation not defined, use default Top_Down"
#define	OLED_TOP_DOWN
#endif

#ifdef I2C_FREQUENCY
#define SSD1327_CLOCK	I2C_FREQUENCY
#else
	#ifdef ESP32
	#define SSD1327_CLOCK							800000	// Clock speed
	#else
	#define SSD1327_CLOCK							400000	// Clock speed
	#endif
#endif

/********************************************************************************
 define:	Define the full screen height length of the display
 ********************************************************************************/
#define SSD1327_X_MAXPIXEL  128  // OLED width maximum memory
#define SSD1327_Y_MAXPIXEL  128  // OLED height maximum memory
#define SSD1327_X	 0
#define SSD1327_Y	 0

#define SSD1327_WIDTH  (SSD1327_X_MAXPIXEL - 2 * SSD1327_X)  // OLED width
#define SSD1327_HEIGHT  SSD1327_Y_MAXPIXEL // OLED height

/********************************************************************************
 define:	Defines type
 ********************************************************************************/
#define	COLOR		uint8_t		// The color (only 4 bits used = 16 grey nuance)
#define	POINT		uint8_t		// The type of coordinate (unsigned short)
#define	LENGTH	uint8_t		// The type of coordinate (unsigned short)

typedef union TPoint
{
	public:
		uint8_t tab[2];
		struct
		{
			uint8_t X;   // X -> LSB
			uint8_t Y;   // Y -> MSB
		};
		uint16_t Bit16;
	public:
		// Default constructor. Initialize to (0, 0)
		TPoint()
		{
			X = 0;
			Y = 0;
		}
		TPoint(const uint8_t x, const uint8_t y)
		{
			X = x;
			Y = y;
		}

		~TPoint()
		{
			;
		}

//		inline TPoint& operator =(const TPoint &p)
//		{
//			X = p.X;
//			Y = p.Y;
//			return *this;
//		}

		inline bool operator ==(const TPoint &p) const
		{
			return ((X == p.X) && (Y == p.Y));
		}

		// Check if we are over limit
		inline bool CheckLimit(const TPoint &limit) const
		{
			return ((X >= limit.X) || (Y >= limit.Y));
		}

		inline void Limit(const TPoint &limit)
		{
			if (X >= limit.X)
				X = limit.X - 1;
			if (Y >= limit.Y)
				Y = limit.Y - 1;
		}
} TPoint;

/********************************************************************************
 typedef:	Defines the total number of rows in the display area
 ********************************************************************************/
typedef struct
{
	LENGTH Column;	//COLUMN
	LENGTH Column2;	//COLUMN2 = COLUMN / 2
	LENGTH Row;		//PAGE
	TPoint Size;
//  POINT X_Adjust;	//OLED x actual display position calibration
//  POINT Y_Adjust;	//OLED y actual display position calibration
} SSD1327_DIS;

#define SSD1327_Swap(x, y) {POINT z; z = (y), y = (x), x = z;}
#define SSD_1327_DELAY	delay

/********************************************************************************
 function:	Defines commonly used colors for the display
 ********************************************************************************/
//Color is 8-bit depth
#define SSD1327_WHITE       0x0F
#define SSD1327_BLACK       0x00
#define SSD1327_GREY				0x08

#define SSD1327_BACKGROUND	SSD1327_BLACK   // Default background color is black
#define FONT_BACKGROUND			SSD1327_BLACK   // Default font background color is black
#define FONT_FOREGROUND	  	SSD1327_WHITE   // Default font foreground color is white

/********************************************************************************
 function:	Core functions
 ********************************************************************************/
bool SSD1327_Init(uint8_t pin_SDA, uint8_t pin_SCL, uint16_t I2C_Address, uint32_t I2C_Clock = SSD1327_CLOCK);

//OLED set cursor + windows + color
void SSD1327_SetCursor(TPoint point);
void SSD1327_SetWindow(TPoint p_start, TPoint p_end);
void SSD1327_SetColor(TPoint point, COLOR Color);
void SSD1327_Clear(COLOR Color = SSD1327_BACKGROUND);
void SSD1327_ClearWindow(TPoint p_start, TPoint p_end, COLOR Color = SSD1327_BACKGROUND);
void SSD1327_ClearAndDisplay(COLOR Color = SSD1327_BACKGROUND);
void SSD1327_Display(void);
void SSD1327_DisplayWindow(TPoint p_start, TPoint p_end);
void SSD1327_ON(void);
void SSD1327_OFF(void);
bool SSD1327_ToggleOnOff(void);

/********************************************************************************
 function:	GUI part
 ********************************************************************************/

/********************************************************************************
 typedef:	dot pixel
 ********************************************************************************/
typedef enum
{
	DOT_PIXEL_1X1 = 1,		// dot pixel 1 x 1
	DOT_PIXEL_2X2, 		// dot pixel 2 X 2
	DOT_PIXEL_3X3,		// dot pixel 3 X 3
	DOT_PIXEL_4X4,		// dot pixel 4 X 4
	DOT_PIXEL_5X5, 		// dot pixel 5 X 5
	DOT_PIXEL_6X6, 		// dot pixel 6 X 6
	DOT_PIXEL_7X7, 		// dot pixel 7 X 7
	DOT_PIXEL_8X8, 		// dot pixel 8 X 8
} DOT_PIXEL;
//#define DOT_PIXEL_DFT  DOT_PIXEL_1X1  //Default dot pilex

/********************************************************************************
 typedef:	dot Fill style
 ********************************************************************************/
typedef enum
{
	DOT_FILL_AROUND = 1,		// dot pixel 1 x 1
	DOT_FILL_RIGHTUP, 		// dot pixel 2 X 2
} DOT_STYLE;
//#define DOT_STYLE_DFT  DOT_FILL_AROUND  //Default dot pilex

/********************************************************************************
 typedef:	solid line and dotted line
 ********************************************************************************/
typedef enum
{
	LINE_SOLID = 0,
	LINE_DOTTED,
} LINE_STYLE;

/********************************************************************************
 typedef:	DRAW Internal fill
 ********************************************************************************/
typedef enum
{
	DRAW_EMPTY = 0,
	DRAW_FULL,
} DRAW_FILL;

/********************************************************************************
 typedef:	time
 ********************************************************************************/
typedef struct
{
	uint16_t Year;  //0000
	uint8_t Month; //1 - 12
	uint8_t Day;   //1 - 30
	uint8_t Hour;  //0 - 23
	uint8_t Min;   //0 - 59
	uint8_t Sec;   //0 - 59
} DEV_TIME;
extern DEV_TIME sDev_time;

/********************************************************************************
 function:	Macro definition variable name
 ********************************************************************************/
//Drawing
void SSD1327_Point(TPoint point);
void SSD1327_DrawPoint(TPoint point, COLOR Color, DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style);
void SSD1327_DrawLine(TPoint p_start, TPoint p_end, COLOR Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel);
void SSD1327_LineTo(TPoint point, COLOR Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel);
void SSD1327_DrawRectangle(TPoint p_start, TPoint p_end, COLOR Color, DRAW_FILL Filled, DOT_PIXEL Dot_Pixel);
void SSD1327_DrawCircle(TPoint center, LENGTH Radius, COLOR Color, DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel);

//pic
void SSD1327_Bitmap(TPoint point, const unsigned char *pMap, POINT Width, POINT Height);
void SSD1327_GrayMap(TPoint point, const unsigned char *pBmp, bool reverse_color = false);

//Display string
void SSD1327_Char(TPoint p_start, const char Acsii_Char, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground);
void SSD1327_String(TPoint p_start, const char *pString, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground);
void SSD1327_Numeric(TPoint point, int32_t Number, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground);
void SSD1327_Showtime(TPoint p_start, TPoint p_end, DEV_TIME *pTime, COLOR Color);

void SSD1327_DisplayUpdated(void);

void SSD1327_Plot(TPoint win_begin, TPoint win_end, float *data, uint8_t count);

static const uint8_t Signal816[16] = //mobile signal // @suppress("Static variable in header file")
		{ 0xFE, 0x02, 0x92, 0x0A, 0x54, 0x2A, 0x38, 0xAA, 0x12, 0xAA, 0x12, 0xAA, 0x12, 0xAA, 0x12, 0xAA };

static const uint8_t Msg816[16] =  //message // @suppress("Static variable in header file")
		{ 0x1F, 0xF8, 0x10, 0x08, 0x18, 0x18, 0x14, 0x28, 0x13, 0xC8, 0x10, 0x08, 0x10, 0x08, 0x1F, 0xF8 };

static const uint8_t Bat816[16] = //battery // @suppress("Static variable in header file")
		{ 0x0F, 0xFE, 0x30, 0x02, 0x26, 0xDA, 0x26, 0xDA, 0x26, 0xDA, 0x26, 0xDA, 0x30, 0x02, 0x0F, 0xFE };

static const uint8_t Bluetooth88[8] = // bluetooth // @suppress("Static variable in header file")
		{ 0x18, 0x54, 0x32, 0x1C, 0x1C, 0x32, 0x54, 0x18 };

static const uint8_t GPRS88[8] = //GPRS // @suppress("Static variable in header file")
		{ 0xC3, 0x99, 0x24, 0x20, 0x2C, 0x24, 0x99, 0xC3 };

static const uint8_t Alarm88[8] = //alarm // @suppress("Static variable in header file")
		{ 0xC3, 0xBD, 0x42, 0x52, 0x4E, 0x42, 0x3C, 0xC3 };

