#include "SSD1327_Test.h"

#include "ninja_flower.h" // Uniquement pour la démo

// Function for debug message, may be redefined elsewhere
void __attribute__((weak)) print_debug(String mess, bool ln = true)
{
	// Just to avoid compile warning
	(void) mess;
	(void) ln;
}

extern SSD1327_DIS sSSD1327_DIS;

/******************************************************************************
 function:	SSD1327_Show
 note:	OLED Clear,
 Draw Line,
 Draw Rectangle,
 Draw Rings,
 Draw Olympic Rings,
 Display String,
 Show Pic
 ******************************************************************************/
void SSD1327_Show(void)
{
	print_debug(F("Clear..."));
	SSD1327_Clear(SSD1327_BACKGROUND);
	SSD1327_Display();

	print_debug(F("Draw Line"));
	SSD1327_DrawLine({0, 1}, TPoint(sSSD1327_DIS.Column - 1, 1), SSD1327_WHITE, LINE_SOLID, DOT_PIXEL_2X2);
	SSD1327_DrawLine({0, 4}, TPoint(sSSD1327_DIS.Column - 1, 4), SSD1327_WHITE, LINE_DOTTED, DOT_PIXEL_1X1);
	SSD1327_DrawLine(TPoint(0, sSSD1327_DIS.Row - 5), TPoint(sSSD1327_DIS.Column - 1, sSSD1327_DIS.Row - 5),
			SSD1327_WHITE, LINE_DOTTED, DOT_PIXEL_1X1);
	SSD1327_DrawLine(TPoint(0, sSSD1327_DIS.Row - 1), TPoint(sSSD1327_DIS.Column - 1, sSSD1327_DIS.Row - 1),
			SSD1327_WHITE, LINE_SOLID, DOT_PIXEL_2X2);

	print_debug(F("Draw Rectangle"));
	SSD1327_DrawRectangle({5, 7}, TPoint(sSSD1327_DIS.Column - 5, sSSD1327_DIS.Row - 7), SSD1327_WHITE,
			DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawRectangle({10, 10}, TPoint(sSSD1327_DIS.Column - 10, 20), SSD1327_WHITE, DRAW_FULL,
			DOT_PIXEL_1X1);

	print_debug(F("Draw Rings"));
	SSD1327_DrawCircle({10, 30}, 3, SSD1327_WHITE, DRAW_FULL, DOT_PIXEL_1X1);
	SSD1327_DrawCircle({10, 40}, 3, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle({10, 50}, 3, SSD1327_WHITE, DRAW_FULL, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(sSSD1327_DIS.Column - 10, 30), 3, SSD1327_WHITE, DRAW_FULL, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(sSSD1327_DIS.Column - 10, 40), 3, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(sSSD1327_DIS.Column - 10, 50), 3, SSD1327_WHITE, DRAW_FULL, DOT_PIXEL_1X1);

	print_debug(F("Draw Olympic Rings"));
	uint16_t Cx1 = 35, Cy1 = 85, Cr = 12;
	uint16_t Cx2 = Cx1 + (2.5 * Cr), Cy2 = Cy1;
	uint16_t Cx3 = Cx1 + (5 * Cr), Cy3 = Cy1;
	uint16_t Cx4 = (Cx1 + Cx2) / 2, Cy4 = Cy1 + Cr;
	uint16_t Cx5 = (Cx2 + Cx3) / 2, Cy5 = Cy1 + Cr;

	SSD1327_DrawCircle(TPoint(Cx1, Cy1), Cr, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(Cx2, Cy2), Cr, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(Cx3, Cy3), Cr, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(Cx4, Cy4), Cr, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
	SSD1327_DrawCircle(TPoint(Cx5, Cy5), Cr, SSD1327_WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);

	print_debug(F("Display String"));
	SSD1327_String({30, 25}, "WaveShare", &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_String({28, 35}, "Electronic", &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_String({18, 45}, "1.5inch OLED", &Font12, FONT_BACKGROUND, SSD1327_WHITE);

	print_debug(F("Showing..."));
	SSD1327_Display();
	SSD_1327_DELAY(4000);
}

/******************************************************************************
 function:	SSD1327_Test_Screen
 note:	Some examples
 ******************************************************************************/
void SSD1327_Test_Screen()
{
	print_debug(F("OLED Show()...\r\n"));
	SSD1327_Show();

	SSD1327_Clear(SSD1327_BACKGROUND);
	SSD1327_Display();

	print_debug(F("Show Pic\r\n"));
	SSD1327_Bitmap({0, 2}, Signal816, 16, 8);
	SSD1327_Bitmap({24, 2}, Bluetooth88, 8, 8);
	SSD1327_Bitmap({40, 2}, Msg816, 16, 8);
	SSD1327_Bitmap({64, 2}, GPRS88, 8, 8);
	SSD1327_Bitmap({90, 2}, Alarm88, 8, 8);
	SSD1327_Bitmap({112, 2}, Bat816, 16, 8);

	print_debug(F("Show 16 Gray Map\r\n"));
	SSD1327_GrayMap({0, 55}, gImage_flower, true);  // 73 gImage_flower  gImage_Dalhia

	SSD1327_String({0, 52}, "MUSIC", &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_String({48, 52}, "MENU", &Font12, FONT_BACKGROUND, SSD1327_WHITE);
	SSD1327_String({90, 52}, "PHONE", &Font12, FONT_BACKGROUND, SSD1327_WHITE);

	SSD1327_Display();
	SSD_1327_DELAY(3000);
}

// ********************************************************************************
// End of file
// ********************************************************************************
