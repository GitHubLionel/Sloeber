/******************************************************************************
 ****************************Upper application layer****************************
 * | file      	:	SSD1327.c
 * |	version		:	V1.0
 * | date		:	2017-11-09
 * | function	:
 Achieve drawing: draw points, lines, boxes, circles and their size,
 solid dotted line, solid rectangle hollow rectangle,
 solid circle hollow circle.
 Achieve display characters: Display a single character, string, number
 Achieve time display: adaptive size display time minutes and seconds
 ******************************************************************************/
#include "SSD1327.h"
#include <stdio.h>
#include <math.h>

//#define DEBUG_SSD1327

extern SSD1327_DIS sSSD1327_DIS;

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

void print_TPoint(String mess, TPoint p)
{
	print_debug(mess + "(" + String(p.X) + ", " + String(p.Y) + ")");
}

// Current cursor point for line
TPoint Cursor;

// Windows of the modified buffer
TPoint Buffer_Start;
TPoint Buffer_End;

/******************************************************************************
 function:	set the cursor to the Point(point.X, Ypoint)
 parameter:	point	:   The coordinate of the point
 ******************************************************************************/
void SSD1327_Point(TPoint point)
{
	// Set cursor to the point
	Cursor = point;
}

/******************************************************************************
 function:	Draw Point(point.X, point.Y) Fill the color
 parameter:	point	:   The coordinate of the point
 Color	:   Set color
 Dot_Pixel:	point size
 ******************************************************************************/
void SSD1327_DrawPoint(TPoint point, COLOR Color, DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
	if (point.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("SSD1327_DrawPoint Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	uint16_t XDir_Num, YDir_Num;
	if (Dot_Style == DOT_FILL_AROUND)
	{
		for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++)
		{
			for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++)
			{
				SSD1327_SetColor(TPoint(point.X + XDir_Num - Dot_Pixel, point.Y + YDir_Num - Dot_Pixel),
						Color);
			}
		}
	}
	else
	{
		for (XDir_Num = 0; XDir_Num < Dot_Pixel; XDir_Num++)
		{
			for (YDir_Num = 0; YDir_Num < Dot_Pixel; YDir_Num++)
			{
				SSD1327_SetColor(TPoint(point.X + XDir_Num, point.Y + YDir_Num), Color);
			}
		}
	}

	// Set cursor to the point
	Cursor = point;
	Buffer_Start = point;
	Buffer_End = point;
}

/******************************************************************************
 function:	Draw a line of arbitrary slope
 parameter:	p_start : Starting point coordinates
 p_end   : End point coordinate
 Color  : The color of the line segment
 ******************************************************************************/
void SSD1327_DrawLine(TPoint p_start, TPoint p_end, COLOR Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
{
	// Check for overflow
	p_start.Limit(sSSD1327_DIS.Size);
	p_end.Limit(sSSD1327_DIS.Size);

	int8_t Line_Style_Temp = 0;

	// Vertical line
	if (p_start.X == p_end.X)
	{
		if (p_end.Y - p_start.Y < 0)
			SSD1327_Swap(p_start.Y, p_end.Y);

		for (uint8_t i = p_start.Y; i <= p_end.Y; i++)
		{
			Line_Style_Temp++;
			//Painted dotted line, 2 point is really virtual
			if (Line_Style == LINE_DOTTED && Line_Style_Temp % 3 == 0)
			{
				//print_debug(F("LINE_DOTTED\r\n");
				SSD1327_DrawPoint(TPoint(p_start.X, i), SSD1327_BACKGROUND, Dot_Pixel, DOT_FILL_RIGHTUP);
				Line_Style_Temp = 0;
			}
			else
			{
				SSD1327_DrawPoint(TPoint(p_start.X, i), Color, Dot_Pixel, DOT_FILL_RIGHTUP);
			}
		}

		// Set cursor to the point
		Cursor = p_end;
		Buffer_Start = p_start;
		Buffer_End = p_end;
		return;
	}

	// Horizontal line
	if (p_start.Y == p_end.Y)
	{
		if (p_end.X - p_start.X < 0)
			SSD1327_Swap(p_start.X, p_end.X);

		for (uint8_t i = p_start.X; i <= p_end.X; i++)
		{
			Line_Style_Temp++;
			//Painted dotted line, 2 point is really virtual
			if (Line_Style == LINE_DOTTED && Line_Style_Temp % 3 == 0)
			{
				//print_debug(F("LINE_DOTTED\r\n");
				SSD1327_DrawPoint(TPoint(i, p_start.Y), SSD1327_BACKGROUND, Dot_Pixel, DOT_FILL_RIGHTUP);
				Line_Style_Temp = 0;
			}
			else
			{
				SSD1327_DrawPoint(TPoint(i, p_start.Y), Color, Dot_Pixel, DOT_FILL_RIGHTUP);
			}
		}

		// Set cursor to the point
		Cursor = p_end;
		Buffer_Start = p_start;
		Buffer_End = p_end;
		return;
	}

	int16_t dx = abs(p_end.X - p_start.X);
	int16_t dy = abs(p_end.Y - p_start.Y);
	int16_t sx = (p_start.X < p_end.X) ? 1 : -1;
	int16_t sy = (p_start.Y < p_end.Y) ? 1 : -1;
	int16_t err = ((dx > dy) ? dx : -dy) / 2;
	int16_t e2;

	while (1)
	{
		Line_Style_Temp++;
		//Painted dotted line, 2 point is really virtual
		if (Line_Style == LINE_DOTTED && Line_Style_Temp % 3 == 0)
		{
			//print_debug(F("LINE_DOTTED\r\n");
			SSD1327_DrawPoint(p_start, SSD1327_BACKGROUND, Dot_Pixel, DOT_FILL_RIGHTUP);
			Line_Style_Temp = 0;
		}
		else
		{
			SSD1327_DrawPoint(p_start, Color, Dot_Pixel, DOT_FILL_RIGHTUP);
		}

		if ((p_start.X == p_end.X) && (p_start.Y == p_end.Y))
		{
			// Set cursor to the point
			Cursor = p_end;
			Buffer_Start = p_start;
			Buffer_End = p_end;
			break;
		}
		e2 = err;
		if (e2 > -dx)
		{
			err -= dy;
			p_start.X += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			p_start.Y += sy;
		}
	}
}

/******************************************************************************
 function:	Draw a line of arbitrary slope from the current cursor position
 parameter:	point : End point coordinates
 Color  : The color of the line segment
 ******************************************************************************/
void SSD1327_LineTo(TPoint point, COLOR Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
{
	SSD1327_DrawLine(Cursor, point, Color, Line_Style, Dot_Pixel);
}

/******************************************************************************
 function:	Draw a rectangle
 parameter:	p_start : Rectangular Starting point coordinates
 p_end   : Rectangular End point coordinate
 Color  : The color of the Rectangular segment
 Filled : Whether it is filled--- 1 solid 0: empty
 ******************************************************************************/
void SSD1327_DrawRectangle(TPoint p_start, TPoint p_end, COLOR Color, DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
{
	if (p_start.CheckLimit(sSSD1327_DIS.Size) || p_end.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	if (p_start.X > p_end.X)
		SSD1327_Swap(p_start.X, p_end.X);
	if (p_start.Y > p_end.Y)
		SSD1327_Swap(p_start.Y, p_end.Y);

	POINT Ypoint;
	if (Filled)
	{
		for (Ypoint = p_start.Y; Ypoint < p_end.Y; Ypoint++)
		{
			SSD1327_DrawLine(TPoint(p_start.X, Ypoint), TPoint(p_end.X, Ypoint), Color, LINE_SOLID,
					Dot_Pixel);
		}
	}
	else
	{
		SSD1327_DrawLine(p_start, TPoint(p_end.X, p_start.Y), Color, LINE_SOLID, Dot_Pixel);
		SSD1327_DrawLine(p_start, TPoint(p_start.X, p_end.Y), Color, LINE_SOLID, Dot_Pixel);
		SSD1327_DrawLine(p_end, TPoint(p_end.X, p_start.Y), Color, LINE_SOLID, Dot_Pixel);
		SSD1327_DrawLine(p_end, TPoint(p_start.X, p_end.Y), Color, LINE_SOLID, Dot_Pixel);
	}
	Cursor = p_end;
	Buffer_Start = p_start;
	Buffer_End = p_end;
}

/******************************************************************************
 function:	Use the 8-point method to draw a circle of the
 specified size at the specified position.
 parameter:	center  : Center coordinate
 Radius    : circle Radius
 Color     : The color of the : circle segment
 Filled    : Whether it is filled: 1 filling 0: Do not
 ******************************************************************************/
void SSD1327_DrawCircle(TPoint center, LENGTH Radius, COLOR Color, DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel)
{
	if (center.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("SSD1327_DrawCircle Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	//Draw a circle from(0, R) as a starting point
	int16_t XCurrent, YCurrent;
	XCurrent = 0;
	YCurrent = Radius;

	//Cumulative error,judge the next point of the logo
	int16_t Esp = 3 - (Radius << 1);

	int16_t sCountY;
	if (Draw_Fill == DRAW_FULL)
	{
		while (XCurrent <= YCurrent)
		{ //Realistic circles
			for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++)
			{
				SSD1327_DrawPoint(TPoint(center.X + XCurrent, center.Y + sCountY), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //1
				SSD1327_DrawPoint(TPoint(center.X - XCurrent, center.Y + sCountY), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //2
				SSD1327_DrawPoint(TPoint(center.X - sCountY, center.Y + XCurrent), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //3
				SSD1327_DrawPoint(TPoint(center.X - sCountY, center.Y - XCurrent), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //4
				SSD1327_DrawPoint(TPoint(center.X - XCurrent, center.Y - sCountY), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //5
				SSD1327_DrawPoint(TPoint(center.X + XCurrent, center.Y - sCountY), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //6
				SSD1327_DrawPoint(TPoint(center.X + sCountY, center.Y - XCurrent), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);             //7
				SSD1327_DrawPoint(TPoint(center.X + sCountY, center.Y + XCurrent), Color, DOT_PIXEL_1X1,
						DOT_FILL_AROUND);
			}
			if (Esp < 0)
				Esp += 4 * XCurrent + 6;
			else
			{
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	}
	else
	{ //Draw a hollow circle
		while (XCurrent <= YCurrent)
		{
			SSD1327_DrawPoint(TPoint(center.X + XCurrent, center.Y + YCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //1
			SSD1327_DrawPoint(TPoint(center.X - XCurrent, center.Y + YCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //2
			SSD1327_DrawPoint(TPoint(center.X - YCurrent, center.Y + XCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //3
			SSD1327_DrawPoint(TPoint(center.X - YCurrent, center.Y - XCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //4
			SSD1327_DrawPoint(TPoint(center.X - XCurrent, center.Y - YCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //5
			SSD1327_DrawPoint(TPoint(center.X + XCurrent, center.Y - YCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //6
			SSD1327_DrawPoint(TPoint(center.X + YCurrent, center.Y - XCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //7
			SSD1327_DrawPoint(TPoint(center.X + YCurrent, center.Y + XCurrent), Color, Dot_Pixel,
					DOT_FILL_AROUND);             //0

			if (Esp < 0)
				Esp += 4 * XCurrent + 6;
			else
			{
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	}

	Buffer_Start = TPoint(center.X - Radius, center.Y - Radius);
	Buffer_End = TPoint(center.X + Radius, center.Y + Radius);
}

/******************************************************************************
 function:	Show English characters
 parameter:	point : coordinate
 Acsii_Char       : To display the English characters
 Font             : A structure pointer that displays a character size
 Color_Background : Select the background color of the English character
 Color_Foreground : Select the foreground color of the English character
 ******************************************************************************/
void SSD1327_Char(TPoint point, const char Acsii_Char, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground)
{
	POINT Page, Column;
	COLOR pixel;

	if (point.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("SSD1327_DisChar Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height
			* (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
	const unsigned char *ptr = &Font->table[Char_Offset];

	for (Page = 0; Page < Font->Height; Page++)
	{
		for (Column = 0; Column < Font->Width; Column++)
		{
			(*ptr & (0x80 >> (Column % 8))) ? pixel = Color_Foreground : pixel = Color_Background;
			SSD1327_SetColor(TPoint(point.X + Column, point.Y + Page), pixel);

			// One pixel is 8 bits
			if (Column % 8 == 7)
				ptr++;
		} // Write a line
		if (Font->Width % 8 != 0)
			ptr++;
	}

	Buffer_Start = point;
	Buffer_End = TPoint(point.X + Font->Width, point.Y + Font->Height);
}

/******************************************************************************
 function:	Display the string
 parameter:	p_start           : coordinate
 pString          : The first address of the English string to be displayed
 Font             : A structure pointer that displays a character size
 Color_Background : Select the background color of the English character
 Color_Foreground : Select the foreground color of the English character
 ******************************************************************************/
void SSD1327_String(TPoint p_start, const char *pString, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground)
{
	if (p_start.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("SSD1327_DisString Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	TPoint p = p_start;

	Buffer_End = p_start;
	while (*pString != '\0')
	{
		//if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the height of the character
		if ((p.X + Font->Width) >= sSSD1327_DIS.Column)
		{
			p.X = p_start.X;
			p.Y += Font->Height;
		}

		// If the Y direction is full, reposition to(Xstart, Ystart)
		if ((p.Y + Font->Height) >= sSSD1327_DIS.Row)
		{
			p = p_start;
		}
		SSD1327_Char(p, *pString, Font, Color_Background, Color_Foreground);

		//The next character of the address
		pString++;

		//The next word of the abscissa increases the font of the broadband
		p.X += Font->Width;
	}
	Buffer_Start = p_start;
}

/******************************************************************************
 function:	Display the string
 parameter:	point : coordinate
 Nummber          : The number displayed
 Font             : A structure pointer that displays a character size
 Color_Background : Select the background color of the English character
 Color_Foreground : Select the foreground color of the English character
 ******************************************************************************/
#define  ARRAY_LEN 255
void SSD1327_Numeric(TPoint point, int32_t Number, sFONT *Font, COLOR Color_Background, COLOR Color_Foreground)
{
	uint8_t Str_Array[ARRAY_LEN] = { 0 };
	uint8_t *pStr = Str_Array;
	bool signe = Number < 0;

	if (point.CheckLimit(sSSD1327_DIS.Size))
	{
#ifdef DEBUG_SSD1327
    print_debug(F("SSD1327_DisNum Input exceeds the normal display range\r\n"));
#endif
		return;
	}

	pStr += (ARRAY_LEN - 2);
	Number = abs(Number);
	while (Number)
	{
		*pStr-- = Number % 10 + '0';
		Number /= 10;
	}
	if (signe)
		*pStr = '-';
	else
		pStr++;

	SSD1327_String(point, (const char*) pStr, Font, Color_Background, Color_Foreground);
}

/******************************************************************************
 function:	Display the bit map,1 byte = 8bit = 8 points
 parameter:	point : coordinate
 pMap   : Pointing to the picture
 Width  : Bitmap Width
 Height : Bitmap Height
 note:
 This function is suitable for bitmap, because a 16-bit data accounted for 16 points
 ******************************************************************************/
void SSD1327_Bitmap(TPoint point, const unsigned char *pMap, POINT Width, POINT Height)
{
	POINT i, j, byteWidth = (Width + 7) / 8;
	for (j = 0; j < Height; j++)
	{
		for (i = 0; i < Width; i++)
		{
			if (*(pMap + j * byteWidth + i / 8) & (128 >> (i & 7)))
			{
				SSD1327_SetColor(TPoint(point.X + i, point.Y + j), SSD1327_WHITE);
			}
		}
	}

	Buffer_Start = point;
	Buffer_End = TPoint(point.X + Width, point.Y + Height);
}

/******************************************************************************
 function:	Display the Gray map,1 byte = 8bit = 2 points
 parameter:	point : coordinate
 pBmp   : Pointing to the picture
 reverse_color  : invert color black <-> white. Default false
 note:
 This function is suitable for bitmap with 4-bit depth color. One byte code 2 points
 Please use the Image2lcd generated array with params:
 - Output file type : C array (*.c)
 - Bits Pixel : 16 color
 - include head data : checked
 ******************************************************************************/
void SSD1327_GrayMap(TPoint point, const unsigned char *pBmp, bool reverse_color)
{
	// Get the Map header Gray, width, height
	POINT Height, Width, row, col, tpoint;
	char Gray = *(pBmp + 1);
	Width = (*(pBmp + 3) << 8) | (*(pBmp + 2));
	Height = (*(pBmp + 5) << 8) | (*(pBmp + 4));

	POINT i, j;
	// Sixteen gray levels
	if (Gray == 0x04)
	{
		// skip header
		pBmp = pBmp + 6;
		for (j = 0; j < Height; j++)
		{
			row = point.Y + j;
			for (i = 0; i < Width / 2; i++)
			{
				col = point.X + i * 2;
				if (reverse_color)
					tpoint = ~(*pBmp);
				else
					tpoint = *pBmp;
				SSD1327_SetColor(TPoint(col++, row), (tpoint >> 4));  //  & 0x0F
				SSD1327_SetColor(TPoint(col, row), tpoint);  //  & 0x0F
				pBmp++;
			}
		}
		Buffer_Start = point;
		Buffer_End = TPoint(point.X + Width, point.Y + Height);
	}
	else
	{
		print_debug(F("Does not support type"));
		return;
	}
}

/******************************************************************************
 function:	According to the display area adaptive display time
 parameter:	p_start : direction Start coordinates
 p_end  :   direction end coordinates
 pTime  :   Pointer to the definition of the structure
 Color  :   Set show color
 note:
 ******************************************************************************/
void SSD1327_Showtime(TPoint p_start, TPoint p_end, DEV_TIME *pTime, COLOR Color)
{
	uint8_t value[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	sFONT *Font = &Font8;
	SSD1327_SetWindow(p_start, p_end);
	SSD1327_ClearWindow(p_start, p_end, SSD1327_BLACK);

	//According to the display area adaptive font size
	POINT Dx = (p_end.X - p_start.X) / 7; //Determine the spacing between characters
	POINT Dy = p_end.Y - p_start.Y;      //determine the font size
	if (Dx > Font24.Width && Dy > Font24.Height)
	{
		Font = &Font24;
	}
	else
		if ((Dx > Font20.Width && Dx < Font24.Width) && (Dy > Font20.Height && Dy < Font24.Height))
		{
			Font = &Font20;
		}
		else
			if ((Dx > Font16.Width && Dx < Font20.Width) && (Dy > Font16.Height && Dy < Font20.Height))
			{
				Font = &Font16;
			}
			else
				if ((Dx > Font12.Width && Dx < Font16.Width) && (Dy > Font12.Height && Dy < Font16.Height))
				{
					Font = &Font12;
				}
				else
					if ((Dx > Font8.Width && Dx < Font12.Width) && (Dy > Font8.Height && Dy < Font12.Height))
					{
						Font = &Font8;
					}
					else
					{
						print_debug(F("Please change the display area size, or add a larger font to modify"));
					}

	//Write data into the cache
	SSD1327_Char(TPoint(p_start.X, p_start.Y), value[pTime->Hour / 10], Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx, p_start.Y), value[pTime->Hour % 10], Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx + Dx / 4 + Dx / 2, p_start.Y), ':', Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx * 2 + Dx / 2, p_start.Y), value[pTime->Min / 10], Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx * 3 + Dx / 2, p_start.Y), value[pTime->Min % 10], Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx * 4 + Dx / 2 - Dx / 4, p_start.Y), ':', Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx * 5, p_start.Y), value[pTime->Sec / 10], Font, FONT_BACKGROUND, Color);
	SSD1327_Char(TPoint(p_start.X + Dx * 6, p_start.Y), value[pTime->Sec % 10], Font, FONT_BACKGROUND, Color);

	SSD1327_DisplayWindow(p_start, p_end);
}

/********************************************************************************
 function:	Update part of the  memory to LCD
 ********************************************************************************/
void SSD1327_DisplayUpdated(void)
{
	SSD1327_DisplayWindow(Buffer_Start, Buffer_End);
}

/********************************************************************************
 function:	plot data in the windows [win_begin, win_end]
 ********************************************************************************/
void SSD1327_Plot(TPoint win_begin, TPoint win_end, float *data, uint8_t count)
{
	float min, max, scale;
	uint8_t y;
	uint8_t nb = win_end.X - win_begin.X;

	if (count > nb)
		count = nb;

	min = max = data[0];
	for (uint8_t i=1; i < count; i++)
	{
		if (data[i] < min) min = data[i];
		if (data[i] > max) max = data[i];
	}

	// Changement de repère :
  //	y = (x - min)*scale + win_end.Y
	// scale = (win_begin.Y - win_end.Y) / (max - min);
	scale = (win_begin.Y - win_end.Y) / (max - min);

	// min < 0, on place l'axe des abscisses à zéro
	if (min < 0)
	  y = win_end.Y + (uint8_t)lrint((-min)*scale);
	else y = win_end.Y;

	// Axe
	SSD1327_DrawLine(TPoint(win_begin.X, y), TPoint(win_end.X, y), SSD1327_WHITE, LINE_SOLID, DOT_PIXEL_1X1);
	SSD1327_DrawLine(win_begin, TPoint(win_begin.X, win_end.Y), SSD1327_WHITE, LINE_SOLID, DOT_PIXEL_1X1);
	SSD1327_Numeric(TPoint(win_begin.X + 2, win_begin.Y), lrint(max), &Font12, FONT_BACKGROUND, SSD1327_WHITE);

	for (uint8_t i=0; i < count; i++)
	{
		SSD1327_SetColor(TPoint(win_begin.X + i, (uint8_t)lrint((data[i] - min)*scale) + win_end.Y), SSD1327_WHITE);
	}
	Buffer_Start = win_begin;
	Buffer_End = win_end;
}

// ********************************************************************************
// End of file
// ********************************************************************************
