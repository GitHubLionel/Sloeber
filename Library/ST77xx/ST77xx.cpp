/**
 * Library for ST7735 and ST7789 TFT oled display
 * Compilation of a lot of files from Internet :
 * https://github.com/afiskon/stm32-st7735
 * https://github.com/cbm80amiga/Arduino_ST7789_STM
 * https://github.com/Floyd-Fish/ST7789-STM32
 */
#include "ST77xx.h"
#include "saber.h"  // Uniquement pour la démo
#include "Dalhia_32bpp.h"

/* Private variable */
SPIClass *ST77xx_Handle = NULL;
// ST77xx Reset pin
uint8_t ST77xx_RESET_Pin;

// ST77xx Select pin
uint8_t ST77xx_CS_Pin;

// ST77xx DC pin
uint8_t ST77xx_DC_Pin;

// ST77xx BLK pin
uint8_t ST77xx_BLK_Pin;

SPISettings spisettings = SPISettings(24000000, MSBFIRST, SPI_MODE3);  //SPI_MODE2

// ********************************************************************************
// Initialization sequence
// ********************************************************************************
/**
 * The sequence is defined as follow :
 * - Number of command in the list
 * - Command, Number of data
 * - ... data ...
 *  If we need delay after send data, add DELAY to "Number of data"
 *  and add the duration delay after the data
 */
#define DELAY 0x80

// Defaults parameters for compiler. Should be override
#ifdef ST77xx_DEFAULT_SIZE
static const uint8_t init_cmds1[];
static const uint8_t init_cmds2[];
static const uint8_t init_cmds3[];
#endif

#if ((defined(ST77xx_IS_128X128)) || (defined(ST77xx_IS_160X80)) || (defined(ST77xx_IS_160X128)))
// based on Adafruit ST7735 library for Arduino
static const uint8_t
  init_cmds1[] = {      	// Init for 7735R, part 1 (red or green tab)
  15,             		// 15 commands in list:
  ST77xx_SWRESET, DELAY,  	//  1: Software reset, 0 args, w/delay
    150,          		//   150 ms delay
  ST77xx_SLPOUT, DELAY,  	//  2: Out of sleep mode, 0 args, w/delay
    255,          		//   500 ms delay
  ST77xx_FRMCTR1, 3,  		//  3: Frame rate ctrl - normal mode, 3 args:
    0x01, 0x2C, 0x2D,     	//   Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST77xx_FRMCTR2, 3,  		//  4: Frame rate control - idle mode, 3 args:
    0x01, 0x2C, 0x2D,     	//   Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST77xx_FRMCTR3, 6,  		//  5: Frame rate ctrl - partial mode, 6 args:
    0x01, 0x2C, 0x2D,     	//   Dot inversion mode
    0x01, 0x2C, 0x2D,     	//   Line inversion mode
  ST77xx_INVCTR, 1,  		//  6: Display inversion ctrl, 1 arg, no delay:
    0x07,           		//   No inversion
  ST77xx_PWCTR1, 3,  		//  7: Power control, 3 args, no delay:
    0xA2,
    0x02,           		//   -4.6V
    0x84,           		//   AUTO mode
  ST77xx_PWCTR2, 1,  		//  8: Power control, 1 arg, no delay:
    0xC5,           		//   VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
  ST77xx_PWCTR3, 2,  		//  9: Power control, 2 args, no delay:
    0x0A,           		//   Opamp current small
    0x00,           		//   Boost frequency
  ST77xx_PWCTR4, 2,  		// 10: Power control, 2 args, no delay:
    0x8A,           		//   BCLK/2, Opamp current small & Medium low
    0x2A,
  ST77xx_PWCTR5, 2,  		// 11: Power control, 2 args, no delay:
    0x8A, 0xEE,
  ST77xx_VMCTR1, 1,  		// 12: Power control, 1 arg, no delay:
    0x0E,
  ST77xx_INVOFF, 0,  		// 13: Don't invert display, no args, no delay
  ST77xx_MADCTL, 1,  		// 14: Memory access control (directions), 1 arg:
    ST77xx_ROTATION,    	//   row addr/col addr, bottom to top refresh
  ST77xx_COLMOD, 1,  		// 15: set color mode, 1 arg, no delay:
    0x05 };         		//   16-bit color

#if (defined(ST77xx_IS_128X128) || defined(ST77xx_IS_160X128))
static const uint8_t
  init_cmds2[] = {      	// Init for 7735R, part 2 (1.44" display)
  2,            		//  2 commands in list:
  ST77xx_CASET, 4,  		//  1: Column addr set, 4 args, no delay:
    0x00, 0x00,       		//   XSTART = 0
    0x00, 0x7F,       		//   XEND = 127
  ST77xx_RASET, 4,  		//  2: Row addr set, 4 args, no delay:
    0x00, 0x00,       		//   XSTART = 0
    0x00, 0x7F };       	//   XEND = 127
#endif // ST77xx_IS_128X128

#ifdef ST77xx_IS_160X80
static const uint8_t
  init_cmds2[] = {      	// Init for 7735S, part 2 (160x80 display)
  3,            		//  3 commands in list:
  ST77xx_CASET, 4,  		//  1: Column addr set, 4 args, no delay:
    0x00, 0x00,       		//   XSTART = 0
    0x00, 0x4F,       		//   XEND = 79
  ST77xx_RASET, 4,  		//  2: Row addr set, 4 args, no delay:
    0x00, 0x00,       		//   XSTART = 0
    0x00, 0x9F,      		//   XEND = 159
  ST77xx_INVON, 0 };    	//  3: Invert colors
#endif

static const uint8_t
  init_cmds3[] = {      	// Init for 7735R, part 3 (red or green tab)
  4,            		//  4 commands in list:
  ST77xx_GMCTRP1, 16, 		//  1: Magical unicorn dust, 16 args, no delay:
    0x02, 0x1c, 0x07, 0x12,
    0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
  ST77xx_GMCTRN1, 16, 		//  2: Sparkles and rainbows, 16 args, no delay:
    0x03, 0x1d, 0x07, 0x06,
    0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
  ST77xx_NORON, DELAY, 		//  3: Normal display on, no args, w/delay
    10,           		//   10 ms delay
  ST77xx_DISPON, DELAY, 	//  4: Main screen turn on, no args w/delay
    100 };          		//   100 ms delay

#endif

#if ((defined(ST77xx_IS_135X240)) || (defined(ST77xx_IS_240X240)))
//
static const uint8_t init_cmds1[] = {      	// Init for ST7789, part 1
		3,            		//  3 commands in list:
		ST77xx_COLMOD, 1 + DELAY,  	//  1: Set color mode
		ST77xx_COLOR_MODE_16bit, 10,
		ST77xx_PORCTRL, 5,  		//  2: Porch control
		0x0C, 0x0C, 0x00, 0x33, 0x33,
		ST77xx_MADCTL, 1,  		//  3: MADCTL (Display Rotation)
		ST77xx_ROTATION};

/* Internal LCD Voltage generator settings */
static const uint8_t init_cmds2[] = {      	// Init for ST7789, part 2
		8,            		//  8 commands in list:
		ST77xx_GCTRL, 1,  		//  1: Gate Control
		0x35,  			// Default value
		ST77xx_VCOMS, 1,  		//  2: VCOM setting
		0x19,  			// 0.725v (default 0.75v for 0x20)
		ST77xx_LCMCTRL, 1,  		//  3: LCMCTRL
		0x2C,  			// Default value
		ST77xx_VDVVRHEN, 1,  		//  4: VDV and VRH command Enable
		0x01,  			// Default value
		ST77xx_VRHS, 1,  		//  5: VRH set
		0x12,  			// +-4.45v (defalut +-4.1v for 0x0B)
		ST77xx_VDVSET, 1,  		//  6: VDV set
		0x20,  			// Default value
		ST77xx_FRCTR2, 1,  		//  7: Frame rate control in normal mode
		0x0F,  			// Default value (60HZ)
		ST77xx_PWCTRL1, 2,  		//  8: Power control
		0xA4, 0xA1}; 		// Default value

/* Division line */
static const uint8_t init_cmds3[] = {      	// Init for ST7789, part 3
		6,            		//  6 commands in list:
		ST77xx_PVGAMCTRL, 14,  	//  1: Positive Voltage Gamma Control
		0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23,
		ST77xx_NVGAMCTRL, 14,  	//  2: Negative voltage Gamma Control
		0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23,
		ST77xx_INVON, DELAY,  	//  3: Inversion ON
		10,
		ST77xx_SLPOUT, 0,  		//  4: Out of sleep mode
		ST77xx_NORON, DELAY,  	//  5: Normal Display on
		10,
		ST77xx_DISPON, DELAY,		//  6: Main screen turned on
		10};

#endif

// ********************************************************************************
// Handler functions
// ********************************************************************************

#define GPIO_PIN_RESET	0
#define GPIO_PIN_SET	1

#define SPI_BEGIN_TRANSACTION()	ST77xx_Handle->beginTransaction(spisettings)
#define SPI_END_TRANSACTION()	ST77xx_Handle->endTransaction()

// RST : Reset function
#define ST77xx_RST_Clr() digitalWrite(ST77xx_RESET_Pin, GPIO_PIN_RESET)
#define ST77xx_RST_Set() digitalWrite(ST77xx_RESET_Pin, GPIO_PIN_SET)

// DT : Data/Command selection function
#define ST77xx_DC_Clr() digitalWrite(ST77xx_DC_Pin, GPIO_PIN_RESET)
#define ST77xx_DC_Set() digitalWrite(ST77xx_DC_Pin, GPIO_PIN_SET)

// CS : Chip Select (if exist)
#ifdef ST77xx_USE_CS
#define ST77xx_Select() digitalWrite(ST77xx_CS_Pin, GPIO_PIN_RESET)
#define ST77xx_UnSelect() digitalWrite(ST77xx_CS_Pin, GPIO_PIN_SET)
#else
#define ST77xx_Select()
#define ST77xx_UnSelect()
#endif

// BLK : Backlight selection function
#define ST77xx_BLK_Clr() digitalWrite(ST77xx_BLK_Pin, GPIO_PIN_RESET)
#define ST77xx_BLK_Set() digitalWrite(ST77xx_BLK_Pin, GPIO_PIN_SET)

// SPI transmit function
inline void SPI_Transmit(uint8_t *buffer, uint16_t size)
{
	while (size--)
	{
		ST77xx_Handle->transfer(*buffer++);
	}
}

// ********************************************************************************
// Hardware functions
// ********************************************************************************

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define SWAP(x, y) {uint16_t z; z = (y), y = (x), x = z;}

/**
 * RESET execution
 */
void ST77xx_Reset()
{
	delay(25);
	ST77xx_RST_Clr();
	delay(25);
	ST77xx_RST_Set();
	delay(50);
}

/**
 * @brief Write command to ST77xx controller
 * @param cmd -> command to write
 * @return none
 */
static void ST77xx_WriteCommand(uint8_t cmd)
{
	ST77xx_Select();
	ST77xx_DC_Clr();
	SPI_BEGIN_TRANSACTION();
	SPI_Transmit(&cmd, 1);
	ST77xx_UnSelect();
	SPI_END_TRANSACTION();
}

/**
 * @brief Write data to ST77xx controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */

static inline void ST77xx_BeginWriteData(void)
{
	ST77xx_Select();
	ST77xx_DC_Set();
	SPI_BEGIN_TRANSACTION();
}

static inline void ST77xx_EndWriteData(void)
{
	ST77xx_UnSelect();
	SPI_END_TRANSACTION();
}

static void ST77xx_WriteData(uint8_t *buff, size_t buff_size)
{
	ST77xx_BeginWriteData();

	if (buff_size < 65535)
	{
		SPI_Transmit(buff, buff_size);
	}
	else
	{
		// split data in small chunks because HAL can't send more than 64K at once
		while (buff_size > 0)
		{
			uint16_t chunk_size = buff_size > 65535 ? 65535 : (uint16_t) buff_size;
			SPI_Transmit(buff, chunk_size);
			buff += chunk_size;
			buff_size -= chunk_size;
		}
	}
	ST77xx_EndWriteData();
}

/**
 * @brief Write data to ST77xx controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST77xx_WriteOneData(uint8_t data)
{
	ST77xx_BeginWriteData();
	SPI_Transmit(&data, 1);
	ST77xx_EndWriteData();
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST77xx_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	uint16_t x_start = x0 + ST77xx_XSHIFT, x_end = x1 + ST77xx_XSHIFT;
	uint16_t y_start = y0 + ST77xx_YSHIFT, y_end = y1 + ST77xx_YSHIFT;

	/* Column Address set */
	ST77xx_WriteCommand(ST77xx_CASET);
	{
		uint8_t data[4] = {(uint8_t) (x_start >> 8), (uint8_t) (x_start & 0xFF), (uint8_t) (x_end >> 8),
				(uint8_t) (x_end & 0xFF)};
		ST77xx_WriteData(data, 4);
	}

	/* Row Address set */
	ST77xx_WriteCommand(ST77xx_RASET);
	{
		uint8_t data[4] = {(uint8_t) (y_start >> 8), (uint8_t) (y_start & 0xFF), (uint8_t) (y_end >> 8),
				(uint8_t) (y_end & 0xFF)};
		ST77xx_WriteData(data, 4);
	}
	/* Write to RAM */
	ST77xx_WriteCommand(ST77xx_RAMWR);
}

/**
 * @brief Execute a formated list of commands for initialization
 * @param list command
 * @return none
 */
static void ST77xx_ExecuteCommandList(const uint8_t *addr)
{
	uint8_t numCommands, numArgs;
	uint16_t ms;

	numCommands = *addr++;
	while (numCommands--)
	{
		uint8_t cmd = *addr++;
		ST77xx_WriteCommand(cmd);

		numArgs = *addr++;
		// If high bit set, delay follows args
		ms = numArgs & DELAY;
		numArgs &= ~DELAY;
		if (numArgs)
		{
			if (numArgs == 1)
				ST77xx_WriteOneData(*addr);
			else
				ST77xx_WriteData((uint8_t*) addr, numArgs);
			addr += numArgs;
		}

		if (ms)
		{
			ms = *addr++;
			if (ms == 255)
				ms = 500;
			delay(ms);
		}
	}
}
// ********************************************************************************
// ST77xx functions
// ********************************************************************************
/**
 * @brief Initialize ST77xx controller
 * @param SPI Handler, RESET, CS (for ST7735 otherwise set NULL) and DC port
 * SPI configuration : Transmit Only Master, 8 bits, MSB First
 * Clock Polarity : High, Clock Phase : 1 Edge
 * Baud Rate : 42 MBits/s  (24 MHz Max)
 * If CS is used, define ST77xx_USE_CS
 * ATTENTION: Backlight must be set !
 * @return none
 */
uint8_t ST77xx_Init(void *pspi, uint8_t RESET_Pin, uint8_t CS_Pin, uint8_t DC_Pin, uint8_t BLK_Pin)
{
	// Hardware parameters
	ST77xx_Handle = (SPIClass*) pspi;
	ST77xx_RESET_Pin = RESET_Pin;
	pinMode(ST77xx_RESET_Pin, OUTPUT);
	ST77xx_CS_Pin = CS_Pin;
#ifdef ST77xx_USE_CS
  pinMode(ST77xx_CS_Pin, OUTPUT);
#endif
	ST77xx_DC_Pin = DC_Pin;
	pinMode(ST77xx_DC_Pin, OUTPUT);
	ST77xx_BLK_Pin = BLK_Pin;
	if (ST77xx_BLK_Pin != 0)
	{
		pinMode(ST77xx_BLK_Pin, OUTPUT);
		ST77xx_BLK_Set();  // Backlight on
	}

//  ST77xx_Handle->pins(D5, D6, D7, CS_Pin);
	ST77xx_Handle->begin();

	// Initialization
	ST77xx_Reset();
	ST77xx_ExecuteCommandList(init_cmds1);
	ST77xx_ExecuteCommandList(init_cmds2);
	ST77xx_ExecuteCommandList(init_cmds3);

	delay(50);
	ST77xx_Fill_Color(BLACK);        //  Fill with Black.
	return 1;
}

/**
 * @brief Backlight control
 * @return none
 */
void ST77xx_BLK_OnOff(ST77xx_BLK_State state)
{
	if (ST77xx_BLK_Pin != 0)
	{
		if (state == ST77xx_BLK_ON)
			ST77xx_BLK_Set();
		else
			ST77xx_BLK_Clr();
	}
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST77xx_Fill_Color(uint16_t color)
{
	uint16_t i, j;
	uint8_t data[2] = {(uint8_t) (color >> 8), (uint8_t) (color & 0xFF)};
	ST77xx_SetAddressWindow(0, 0, ST77xx_WIDTH - 1, ST77xx_HEIGHT - 1);

	ST77xx_BeginWriteData();
	for (i = 0; i < ST77xx_WIDTH; i++)
		for (j = 0; j < ST77xx_HEIGHT; j++)
		{
			SPI_Transmit(data, 2);
		}
	ST77xx_EndWriteData();
	delay(1);
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST77xx_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= ST77xx_WIDTH) || (y >= ST77xx_HEIGHT))
		return;

	ST77xx_SetAddressWindow(x, y, x, y);
	uint8_t data[2] = {(uint8_t) (color >> 8), (uint8_t) (color & 0xFF)};

	ST77xx_WriteData(data, 2);
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST77xx_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	if ((xEnd >= ST77xx_WIDTH) || (yEnd >= ST77xx_HEIGHT))
		return;

	uint16_t i, j;
	uint8_t data[2] = {(uint8_t) (color >> 8), (uint8_t) (color & 0xFF)};
	ST77xx_SetAddressWindow(xSta, ySta, xEnd, yEnd);

	ST77xx_BeginWriteData();
	for (i = ySta; i <= yEnd; i++)
		for (j = xSta; j <= xEnd; j++)
		{
			SPI_Transmit(data, 2);
		}
	ST77xx_EndWriteData();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST77xx_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > ST77xx_WIDTH) || (y <= 0) || (y > ST77xx_HEIGHT))
		return;

	ST77xx_Fill((uint16_t) (x - 1), (uint16_t) (y - 1), (uint16_t) (x + 1), (uint16_t) (y + 1),
			color);
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST77xx_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
	if (steep)
	{
		SWAP(x0, y0);
		SWAP(x1, y1);
	}

	if (x0 > x1)
	{
		SWAP(x0, x1);
		SWAP(y0, y1);
	}

	int16_t dx = (int16_t) (x1 - x0);
	int16_t dy = (int16_t) (ABS(y1 - y0));

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			ST77xx_DrawPixel(y0, x0, color);
		}
		else
		{
			ST77xx_DrawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST77xx_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	ST77xx_DrawLine(x1, y1, x2, y1, color);
	ST77xx_DrawLine(x1, y1, x1, y2, color);
	ST77xx_DrawLine(x1, y2, x2, y2, color);
	ST77xx_DrawLine(x2, y1, x2, y2, color);
}

/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST77xx_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3,
		uint16_t y3, uint16_t color)
{
	/* Draw lines */
	ST77xx_DrawLine(x1, y1, x2, y2, color);
	ST77xx_DrawLine(x2, y2, x3, y3, color);
	ST77xx_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST77xx_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = (int16_t) (1 - r);
	int16_t ddF_x = 1;
	int16_t ddF_y = (int16_t) (-2 * r);
	int16_t x = 0;
	int16_t y = r;

	ST77xx_DrawPixel(x0, (uint16_t) (y0 + r), color);
	ST77xx_DrawPixel(x0, (uint16_t) (y0 - r), color);
	ST77xx_DrawPixel((uint16_t) (x0 + r), y0, color);
	ST77xx_DrawPixel((uint16_t) (x0 - r), y0, color);

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

		ST77xx_DrawPixel((uint16_t) (x0 + x), (uint16_t) (y0 + y), color);
		ST77xx_DrawPixel((uint16_t) (x0 - x), (uint16_t) (y0 + y), color);
		ST77xx_DrawPixel((uint16_t) (x0 + x), (uint16_t) (y0 - y), color);
		ST77xx_DrawPixel((uint16_t) (x0 - x), (uint16_t) (y0 - y), color);

		ST77xx_DrawPixel((uint16_t) (x0 + y), (uint16_t) (y0 + x), color);
		ST77xx_DrawPixel((uint16_t) (x0 - y), (uint16_t) (y0 + x), color);
		ST77xx_DrawPixel((uint16_t) (x0 + y), (uint16_t) (y0 - x), color);
		ST77xx_DrawPixel((uint16_t) (x0 - y), (uint16_t) (y0 - x), color);
	}
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST77xx_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST77xx_WIDTH) || (y >= ST77xx_HEIGHT))
		return;
	if ((x + w - 1) >= ST77xx_WIDTH)
		return;
	if ((y + h - 1) >= ST77xx_HEIGHT)
		return;

	ST77xx_SetAddressWindow(x, y, (uint16_t) (x + w - 1), (uint16_t) (y + h - 1));
	ST77xx_WriteData((uint8_t*) data, sizeof(uint16_t) * w * h);
}

void ST77xx_DrawImage_P(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data,
		bool invert)
{
	if ((x >= ST77xx_WIDTH) || (y >= ST77xx_HEIGHT))
		return;
	if ((x + w - 1) >= ST77xx_WIDTH)
		return;
	if ((y + h - 1) >= ST77xx_HEIGHT)
		return;

	ST77xx_SetAddressWindow(x, y, (uint16_t) (x + w - 1), (uint16_t) (y + h - 1));

	ST77xx_BeginWriteData();
	uint32_t size = w * h;
	for (uint32_t i = 0; i < size; i++)
	{
		uint16_t byteval = pgm_read_word_near(data + i);
		uint8_t data[2] = {(uint8_t) (byteval & 0xFF), (uint8_t) (byteval >> 8)};
		if (invert)
			SWAP(data[0], data[1]);
		SPI_Transmit(data, 2);
	}

	ST77xx_EndWriteData();
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST77xx_WriteChar(uint16_t x, uint16_t y, char ch, Font_TypeDef font, uint16_t color,
		uint16_t bgcolor)
{
	uint8_t i, j;
	uint16_t b;
	uint8_t data[2] = {(uint8_t) (color >> 8), (uint8_t) (color & 0xFF)};
	uint8_t databg[2] = {(uint8_t) (bgcolor >> 8), (uint8_t) (bgcolor & 0xFF)};

	ST77xx_SetAddressWindow(x, y, (uint16_t) (x + font.FontWidth - 1),
			(uint16_t) (y + font.FontHeight - 1));

	ST77xx_BeginWriteData();
	for (i = 0; i < font.FontHeight; i++)
	{
//    b = font.data[(ch - 32) * font.FontHeight + i];
		b = pgm_read_word_near(&font.data[(ch - 32) * font.FontHeight + i]);
		for (j = 0; j < font.FontWidth; j++)
		{
			if ((b << j) & 0x8000)
			{
				SPI_Transmit(data, 2);
			}
			else
			{
				SPI_Transmit(databg, 2);
			}
		}
	}
	ST77xx_EndWriteData();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST77xx_WriteString(uint16_t x, uint16_t y, const char *str, Font_TypeDef font, uint16_t color,
		uint16_t bgcolor)
{
	while (*str)
	{
		if (x + font.FontWidth >= ST77xx_WIDTH)
		{
			x = 0;
			y += font.FontHeight;
			if (y + font.FontHeight >= ST77xx_HEIGHT)
			{
				break;
			}

			if (*str == ' ')
			{
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST77xx_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.FontWidth;
		str++;
	}
}

/** 
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST77xx_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint8_t i;

	/* Check input parameters */
	if (x >= ST77xx_WIDTH || y >= ST77xx_HEIGHT)
	{
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= ST77xx_WIDTH)
	{
		w = (uint16_t) (ST77xx_WIDTH - x);
	}
	if ((y + h) >= ST77xx_HEIGHT)
	{
		h = (uint16_t) (ST77xx_HEIGHT - y);
	}

	/* Draw lines */
	for (i = 0; i <= h; i++)
	{
		/* Draw lines */
		ST77xx_DrawLine(x, (uint16_t) (y + i), (uint16_t) (x + w), (uint16_t) (y + i), color);
	}
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST77xx_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3,
		uint16_t y3, uint16_t color)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, yinc1 = 0, yinc2 = 0, den = 0,
			num = 0, numadd = 0, numpixels = 0, curpixel = 0;

	deltax = (int16_t) (ABS(x2 - x1));
	deltay = (int16_t) (ABS(y2 - y1));
	x = (int16_t) x1;
	y = (int16_t) y1;

	if (x2 >= x1)
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1)
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else
	{
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
	}
	else
	{
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		ST77xx_DrawLine((uint16_t) x, (uint16_t) y, x3, y3, color);

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

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST77xx_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = (int16_t) (1 - r);
	int16_t ddF_x = 1;
	int16_t ddF_y = (int16_t) (-2 * r);
	int16_t x = 0;
	int16_t y = r;

	ST77xx_DrawPixel((uint16_t) x0, (uint16_t) (y0 + r), color);
	ST77xx_DrawPixel((uint16_t) x0, (uint16_t) (y0 - r), color);
	ST77xx_DrawPixel((uint16_t) (x0 + r), (uint16_t) y0, color);
	ST77xx_DrawPixel((uint16_t) (x0 - r), (uint16_t) y0, color);
	ST77xx_DrawLine((uint16_t) (x0 - r), (uint16_t) y0, (uint16_t) (x0 + r), (uint16_t) y0, color);

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

		ST77xx_DrawLine((uint16_t) (x0 - x), (uint16_t) (y0 + y), (uint16_t) (x0 + x),
				(uint16_t) (y0 + y), color);
		ST77xx_DrawLine((uint16_t) (x0 + x), (uint16_t) (y0 - y), (uint16_t) (x0 - x),
				(uint16_t) (y0 - y), color);

		ST77xx_DrawLine((uint16_t) (x0 + y), (uint16_t) (y0 + x), (uint16_t) (x0 - y),
				(uint16_t) (y0 + x), color);
		ST77xx_DrawLine((uint16_t) (x0 + y), (uint16_t) (y0 - x), (uint16_t) (x0 - y),
				(uint16_t) (y0 - x), color);
	}
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST77xx_InvertColors(uint8_t invert)
{
	ST77xx_WriteCommand(invert ? ST77xx_INVON : ST77xx_INVOFF);
}

/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST77xx_TearEffect(uint8_t tear)
{
	ST77xx_WriteCommand(tear ? ST77xx_TEON : ST77xx_TEOFF);
}

/**
 * @brief On/off display
 * @param display -> Whether to display on
 * @return none
 */
void ST77xx_DisplayState(uint8_t display)
{
	ST77xx_WriteCommand(display ? ST77xx_DISPON : ST77xx_DISPOFF);
}

/**
 * @brief On/off idle display
 * @param idle -> Whether to idle
 * @return none
 */
void ST77xx_IdleMode(uint8_t idle)
{
	ST77xx_WriteCommand(idle ? ST77xx_IDMON : ST77xx_IDMOFF);
}

/** 
 * @brief A Simple test function for ST77xx
 * @param  none
 * @return  none
 */
void ST77xx_Test_Screen(void)
{
	ST77xx_Fill_Color(WHITE);
	delay(1000);
	ST77xx_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	delay(1000);
	ST77xx_Fill_Color(CYAN);
	ST77xx_Fill_Color(RED);
	ST77xx_Fill_Color(BLUE);
	ST77xx_Fill_Color(GREEN);
	ST77xx_Fill_Color(YELLOW);
	ST77xx_Fill_Color(BROWN);
	ST77xx_Fill_Color(DARKBLUE);
	ST77xx_Fill_Color(MAGENTA);
	ST77xx_Fill_Color(LIGHTGREEN);
	ST77xx_Fill_Color(LGRAY);
	ST77xx_Fill_Color(LBBLUE);
	ST77xx_Fill_Color(WHITE);

	ST77xx_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST77xx_WriteString(10, 50, "Hello Steve!", Font_7x10, RED, WHITE);
	ST77xx_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, WHITE);
	ST77xx_WriteString(10, 100, "Hello Steve!", Font_16x26, MAGENTA, WHITE);
	delay(2000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, RED);
	ST77xx_DrawRectangle(30, 30, 100, 100, WHITE);
	delay(1000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, RED);
	ST77xx_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	delay(1000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, RED);
	ST77xx_DrawCircle(60, 60, 25, WHITE);
	delay(1000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, RED);
	ST77xx_DrawFilledCircle(60, 60, 25, WHITE);
	delay(1000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, RED);
	ST77xx_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	delay(1000);

	ST77xx_Fill_Color(RED);
	ST77xx_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, RED);
	ST77xx_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	delay(1000);

	//  If FLASH cannot storage anymore datas, please delete codes below.
	ST77xx_Fill_Color(WHITE);
	ST77xx_DrawImage_P(0, 40, 128, 72, (uint16_t*) Dalhia_32bpp, true);
	ST77xx_DrawImage_P(0, 112, 128, 128, (uint16_t*) saber, false);
	delay(3000);
}

// ********************************************************************************
// End of file
// ********************************************************************************
