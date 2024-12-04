/******************************************************************************
 * | file    	:	SSD1327.c
 * | version	:	V1.0
 * | date	:	2017-11-09
 * | function	:	SSD1327 Drive function
 * | url		:	https://www.waveshare.com/wiki/1.5inch_OLED_Module

 note:
 Image scanning:
 Please use progressive scanning to generate images or fonts
 ******************************************************************************/

#include "SSD1327.h"
#include <Wire.h>

#define SSD1327_COMMAND   0x00
#define SSD1327_DATA      0x40

#define  SET_COLUMN_ADDRESS				0x15	//	Set Column Address
#define  SET_ROW_ADDRESS					0x75 	//	Set Row Address
#define  SET_CONTRAST_CURRENT			0x81 	//	Set Contrast Current
#define  SSD_NOP								  0x84	//	NOP 0x84 ~ 0x86, 0xB2, 0xBB
#define  SET_RE_MAP								0xA0 	//	Set Re-map
#define  SET_DISPLAY_START_LINE		0xA1 	//	Set Display Start Line
#define  SET_DISPLAY_OFFSET				0xA2	//	Set Display Offset
#define  SET_DISPLAY_MD_NORMAL		0xA4	//	Set Display Mode 0xA4 (Normal)
#define  SET_DISPLAY_MD_ON				0xA5	//	Set Display Mode 0xA5 (On)
#define  SET_DISPLAY_MD_OFF				0xA6	//	Set Display Mode 0xA6 (Off)
#define  SET_DISPLAY_MD_INVERSE		0xA7	//	Set Display Mode 0xA7 (Reverse)
#define  SET_MULTIPLEX_RATIO			0xA8	//	Set Multiplex Ratio
#define  FUNCTION_SELECTION_A			0xAB 	//	Function selection A
#define  SET_DISPLAY_OFF					0xAE 	//	Set Display ON
#define  SET_DISPLAY_ON						0xAF 	//	Set Display OFF
#define  SET_PHASE_LENGTH					0xB1	//	Set Phase Length
#define  SET_CLOCK_DIVIDER				0xB3 	//	Set Front Clock Divider / Oscillator Frequency
#define  SET_GPIO									0xB5 	//	Set GPIO
#define  SET_SECOND_PRE_CHARGE_PERIOD	0xB6	//	Set  Second Pre-charge period
#define  SET_GRAY_SCALE_TABLE			0xB8			//	Set Gray Scale Table
#define  SELECT_DEFAULT_LINEAR_GRAY_SCALE_TABLE	0xB9		//	Select Default Linear Gray Scale Table
#define  SET_PRE_CHARGE_VOLTAGE		0xBC 	//	Set Pre-charge voltage
#define  SET_VCOMH_VOLTAGE				0xBE	//	Set VCOMH Voltage
#define  FUNCTION_SELECTION_B			0xD5 	//	Function selection B
#define  SET_COMMAND_LOCK					0xFD	//	Set Command Lock: 0x16; unlock: 0x12
#define  HORIZONTAL_SCROLL_SETUP	0x26 	//	Horizontal Scroll Setup 0x26 / 0x27
#define  DEACTIVATE_SCROLL				0x2E	//	Deactivate Scroll
#define  ACTIVATE_SCROLL					0x2F	//	Activate Scroll

/**
 * Orientation
 */
#define  SET_TOP_DOWN							0x51	//	Top down orientation with set remap A[7:0]
#define  SET_LEFT_RIGHT						0x41	//	Left Right orientation with set remap A[7:0]
#define  SET_DOWN_TOP							0x42	//	Down Top orientation with set remap A[7:0]
#define  SET_RIGHT_LEFT						0x52	//	Right Left orientation with set remap A[7:0]

#ifdef OLED_TOP_DOWN
#define	 ORIENTATION	SET_TOP_DOWN
#define  SSD1327_IS_TOP
#endif
#ifdef OLED_LEFT_RIGHT
#define	 ORIENTATION	SET_LEFT_RIGHT
#endif
#ifdef OLED_DOWN_TOP
#define	 ORIENTATION	SET_DOWN_TOP
#define  SSD1327_IS_TOP
#endif
#ifdef OLED_RIGHT_LEFT
#define	 ORIENTATION	SET_RIGHT_LEFT
#endif

/* Private variable */
static uint16_t SSD1327_I2C_ADDR;

static COLOR Buffer[(SSD1327_WIDTH / 2) * SSD1327_HEIGHT] = { SSD1327_BACKGROUND };
SSD1327_DIS sSSD1327_DIS;
static bool DisplayIsOn = true;

// ********************************************************************************
// I2C Handler function
// ********************************************************************************
/*******************************************************************************
 function:	Write register address and data
 *******************************************************************************/
void SSD1327_WriteReg(uint8_t command)
{
	Wire.beginTransmission(SSD1327_I2C_ADDR);
	Wire.write(SSD1327_COMMAND);
	Wire.write(command);
	Wire.endTransmission();
}

void SSD1327_WriteData(uint8_t *data, uint16_t count)
{
	Wire.beginTransmission(SSD1327_I2C_ADDR);
	Wire.write(SSD1327_DATA);
	while (count-- > 0)
		Wire.write(*data++);
	Wire.endTransmission();
}

void SSD1327_WriteData(uint8_t data)
{
	Wire.beginTransmission(SSD1327_I2C_ADDR);
	Wire.write(SSD1327_DATA);
	Wire.write(data);
	Wire.endTransmission();
}

/*******************************************************************************
 function:	Common register initialization
 *******************************************************************************/
static void SSD1327_InitReg()
{
	SSD1327_WriteReg(SET_DISPLAY_OFF);  		//--turn off oled panel
	SSD_1327_DELAY(100);
	SSD1327_WriteReg(SET_COLUMN_ADDRESS);  	// set column address
	SSD1327_WriteReg(0x00);  								//  start column   0
	SSD1327_WriteReg(0x3F);  								//  end column   63
	SSD1327_WriteReg(SET_ROW_ADDRESS);  		// set row address
	SSD1327_WriteReg(0x00);  								//  start row   0
	SSD1327_WriteReg(0x7F);  								//  end row   127
	SSD1327_WriteReg(SET_CONTRAST_CURRENT); // set contrast control
	SSD1327_WriteReg(0x7F);									//  50%
	SSD1327_WriteReg(SET_RE_MAP);  					// segment remap
	SSD1327_WriteReg(ORIENTATION);
	SSD1327_WriteReg(SET_DISPLAY_START_LINE); // start line
	SSD1327_WriteReg(0x00);
	SSD1327_WriteReg(SET_DISPLAY_OFFSET);  		// display offset
	SSD1327_WriteReg(0x00);
	SSD1327_WriteReg(SET_DISPLAY_MD_NORMAL);  // normal display SET_DISPLAY_MD_NORMAL
	SSD1327_WriteReg(SET_MULTIPLEX_RATIO);  	// set multiplex ratio
	SSD1327_WriteReg(0x7F);
	SSD1327_WriteReg(SET_PHASE_LENGTH);  			// set phase leghth
	SSD1327_WriteReg(0xF1);
	SSD1327_WriteReg(SET_CLOCK_DIVIDER);  		// set dclk
	SSD1327_WriteReg(0x00); //80Hz: 0xC1 90Hz: 0xE1   100Hz: 0x00   110Hz: 0x30 120Hz: 0x50   130Hz: 0x70
	SSD1327_WriteReg(SELECT_DEFAULT_LINEAR_GRAY_SCALE_TABLE);  // use linear lookup table
	SSD1327_WriteReg(FUNCTION_SELECTION_A);  	// Function selection A
	SSD1327_WriteReg(0x01);  									// Internal VDD regulator is enabled
	SSD1327_WriteReg(SET_SECOND_PRE_CHARGE_PERIOD);  // set phase leghth
	SSD1327_WriteReg(0x0F);
	SSD1327_WriteReg(SET_VCOMH_VOLTAGE);			// Set VCOMH Voltage
	SSD1327_WriteReg(0x0F);
	SSD1327_WriteReg(SET_PRE_CHARGE_VOLTAGE);	// Set Pre-charge voltage
	SSD1327_WriteReg(0x08);
	SSD1327_WriteReg(FUNCTION_SELECTION_B);		// Function selection B
	SSD1327_WriteReg(0x62);
	SSD1327_WriteReg(SET_COMMAND_LOCK);				// Set Command Lock
	SSD1327_WriteReg(0x12);			// Unlock
}

/********************************************************************************
 function:	initialization
 ********************************************************************************/
/**
 * Initialize the LCD with I2C protocol (fast speed, 400000 Hz)
 * - Priority : High
 * - Mode : Normal
 * - Data Width : Byte
 * - pin_SDA : SDA pin
 * - pin_SCL : SCL pin
 * - orientation : Top-Down default
 */
bool SSD1327_Init(uint8_t pin_SDA, uint8_t pin_SCL, uint16_t I2C_Address, uint32_t I2C_Clock)
{
	SSD1327_I2C_ADDR = I2C_Address;

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

	//Set the initialization register
	SSD1327_InitReg();

	sSSD1327_DIS.Column = SSD1327_WIDTH;
	sSSD1327_DIS.Row = SSD1327_HEIGHT;
	sSSD1327_DIS.Size = TPoint(SSD1327_WIDTH, SSD1327_HEIGHT);
//	sSSD1327_DIS.X_Adjust = SSD1327_X;
//	sSSD1327_DIS.Y_Adjust = SSD1327_Y;
	sSSD1327_DIS.Column2 = sSSD1327_DIS.Column / 2;

	//Turn on the OLED display
	SSD1327_WriteReg(SET_DISPLAY_ON);
	SSD_1327_DELAY(200);
	SSD1327_Display();
	return true;
}

/********************************************************************************
 function:	Set the display point(X, Y)
 ********************************************************************************/
void SSD1327_SetCursor(TPoint point)
{
	if (point.CheckLimit(sSSD1327_DIS.Size))
		return;

	SSD1327_WriteReg(SET_COLUMN_ADDRESS);
	SSD1327_WriteReg(point.X / 2);
	SSD1327_WriteReg(point.X / 2);

	SSD1327_WriteReg(SET_ROW_ADDRESS);
	SSD1327_WriteReg(point.Y);
	SSD1327_WriteReg(point.Y);
}

/********************************************************************************
 function:	Set the display Window(p_start.X, p_start.Y, p_end.X, p_end.Y)
 parameter:	p_start.X :   X direction Start coordinates
 p_start.Y :   Y direction Start coordinates
 p_end.X   :   X direction end coordinates
 p_end.Y   :   Y direction end coordinates
 ********************************************************************************/
void SSD1327_SetWindow(TPoint p_start, TPoint p_end)
{
	p_start.Limit(sSSD1327_DIS.Size);
	p_end.Limit(sSSD1327_DIS.Size);

	SSD1327_WriteReg(SET_COLUMN_ADDRESS);
	SSD1327_WriteReg(p_start.X / 2);
	SSD1327_WriteReg(p_end.X / 2);

	SSD1327_WriteReg(SET_ROW_ADDRESS);
	SSD1327_WriteReg(p_start.Y);
	SSD1327_WriteReg(p_end.Y);
}

/********************************************************************************
 function:	Set show color in the buffer
 parameter:	Color  :   Set show color, 4-bit depth (16 grey nuance)
 ********************************************************************************/
void SSD1327_SetColor(TPoint point, COLOR Color)
{
	if (point.CheckLimit(sSSD1327_DIS.Size))
		return;

#ifdef SSD1327_IS_TOP
	uint8_t *px = &point.X;
	uint8_t *py = &point.Y;
#else
	uint8_t *px = &point.Y;
	uint8_t *py = &point.X;
#endif

	uint16_t pos = (uint16_t) (*py * sSSD1327_DIS.Column2) + *px / 2;
	uint8_t c = Buffer[pos];

	if (*px & 1) // odd = right part of the byte
		c = (c & 0xF0) | (Color & 0x0F); // keep left and add color (clean the 4 high bits) to right
	else
		c = (c & 0x0F) | (Color << 4); // keep right and add color (push 4 bits to high) to left

	Buffer[pos] = c;
}

/********************************************************************************
 function:	Clear buffer, not the display
 Default color is SSD1327_BACKGROUND
 ********************************************************************************/
void SSD1327_Clear(COLOR Color)
{
	// One byte for 2 pixels. First, clean the 4 high bits.
	memset(Buffer, ((Color & 0x0F) | (Color << 4)), sSSD1327_DIS.Row * sSSD1327_DIS.Column2);
}

/********************************************************************************
 function:	Clear Window (the buffer)
 ********************************************************************************/
void SSD1327_ClearWindow(TPoint p_start, TPoint p_end, COLOR Color)
{
	LENGTH page, Xpoint, Ypoint;

#ifdef SSD1327_IS_TOP
	uint8_t *sx = &p_start.X;
	uint8_t *sy = &p_start.Y;
	uint8_t *ex = &p_end.X;
	uint8_t *ey = &p_end.Y;
#else
	uint8_t *sx = &p_start.Y;
	uint8_t *sy = &p_start.X;
	uint8_t *ex = &p_end.Y;
	uint8_t *ey = &p_end.X;
#endif

	Xpoint = (*ex - *sx) / 2;
	Ypoint = *ey - *sy;

	if ((Xpoint == 0) || (Ypoint == 0))
		return;

	// One byte for 2 pixels. First, clean the 4 high bits.
	Color &= 0x0F;
	COLOR Color2 = (Color | (Color << 4));

	COLOR *pBuf = (COLOR*) Buffer;
	pBuf += *sy * sSSD1327_DIS.Column2 + *sx / 2;

	for (page = 0; page < Ypoint; page++)
	{
		if (*sx & 1) // odd
		{
			*pBuf = (*pBuf & 0xF0) | Color; // keep left and add color to right
			pBuf++;
			if (*ex & 1)
			{
				memset(pBuf, Color2, Xpoint);
				pBuf--;
			}
			else
			{
				memset(pBuf, Color2, Xpoint);
				pBuf += Xpoint;
				*pBuf = (*pBuf & 0x0F) | (Color << 4);  // keep right and add color to left
				pBuf -= Xpoint + 1;
			}
		}
		else
		{
			if (*ex & 1) // odd
				memset(pBuf, Color2, Xpoint + 1);
			else
			{
				memset(pBuf, Color2, Xpoint);
				pBuf += Xpoint;
				*pBuf = (*pBuf & 0x0F) | (Color << 4);
				pBuf -= Xpoint;
			}
		}

		// Next line
		pBuf += sSSD1327_DIS.Column2;
	}

// Equivalent but less speed
//	for (POINT row = p_start.Y; row < p_end.Y; row++)
//		for (POINT col = p_start.X; col < p_end.X; col++)
//			SSD1327_SetColor(col, row, Color);
}

/********************************************************************************
 function:	Clear buffer and update all memory to LCD
 Default color is SSD1327_BACKGROUND
 ********************************************************************************/
void SSD1327_ClearAndDisplay(COLOR Color)
{
	SSD1327_Clear(Color);
	SSD1327_Display();
}

/********************************************************************************
 function:	Update all memory to LCD
 ********************************************************************************/
void SSD1327_Display(void)
{
	// No need to display if screen is off
	if (!DisplayIsOn)
		return;

	LENGTH page;
	COLOR *pBuf = (COLOR*) Buffer;

	SSD1327_SetWindow(TPoint(0, 0), TPoint(sSSD1327_DIS.Column - 1, sSSD1327_DIS.Row - 1));

	//write data
	for (page = 0; page < sSSD1327_DIS.Row; page++)
	{
		SSD1327_WriteData(pBuf, sSSD1327_DIS.Column2);
		pBuf += sSSD1327_DIS.Column2;
	}
}

/********************************************************************************
 function:	Update part of the  memory to LCD
 ********************************************************************************/
void SSD1327_DisplayWindow(TPoint p_start, TPoint p_end)
{
	// No need to display if screen is off
	if (!DisplayIsOn)
		return;

	if (p_start == p_end)
		return;

	LENGTH page, Xpoint, Ypoint;
	p_start.Limit(sSSD1327_DIS.Size);
	p_end.Limit(sSSD1327_DIS.Size);

#ifdef SSD1327_IS_TOP
	uint8_t *sx = &p_start.X;
	uint8_t *sy = &p_start.Y;
	uint8_t *ex = &p_end.X;
	uint8_t *ey = &p_end.Y;
#else
	uint8_t *sx = &p_start.Y;
	uint8_t *sy = &p_start.X;
	uint8_t *ex = &p_end.Y;
	uint8_t *ey = &p_end.X;
#endif

	Xpoint = (*ex - *sx) / 2 + 1;
	if ((*sx & 1) && (!(*ex & 1)))
		Xpoint++;
	Ypoint = *ey - *sy;

	COLOR *pBuf = (COLOR*) Buffer;

	SSD1327_SetWindow(TPoint(*sx, *sy), TPoint(*ex, *ey));

	pBuf += *sy * sSSD1327_DIS.Column2 + *sx / 2;
	for (page = 0; page < Ypoint; page++)
	{
		SSD1327_WriteData(pBuf, Xpoint);
		pBuf += sSSD1327_DIS.Column2;
	}
}

/********************************************************************************
 function:	Toggle display
 ********************************************************************************/

/**
 * Put Display ON
 */
void SSD1327_ON(void)
{
	if (DisplayIsOn)
		return;
	SSD1327_WriteReg(SET_DISPLAY_ON);
	SSD_1327_DELAY(50);
	DisplayIsOn = true;
	// Refresh screen
	SSD1327_Display();
}

/**
 * Put Display OFF
 */
void SSD1327_OFF(void)
{
	if (!DisplayIsOn)
		return;
	SSD1327_WriteReg(SET_DISPLAY_OFF);
	DisplayIsOn = false;
}

/**
 * Toggle Display On <-> Off
 */
void SSD1327_ToggleOnOff(void)
{
	(DisplayIsOn) ? SSD1327_OFF() : SSD1327_ON();
}

// ********************************************************************************
// End of file
// ********************************************************************************
