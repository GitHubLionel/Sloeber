/**
 * Library for ST7735 and ST7789 TFT oled display
 * For STM32 with HAL
 * Compilation of a lot of file from Internet :
 * https://github.com/afiskon/stm32-st7735
 * https://github.com/cbm80amiga/Arduino_ST7789_STM
 * https://github.com/Floyd-Fish/ST7789-STM32
 */
#ifndef __ST77XX_H
#define __ST77XX_H

#include <Arduino.h>

#ifdef USE_CONFIG_LIB_FILE
#include "config_lib.h"
#endif

#include "SPI.h"
#include "fonts.h"

//#define TFT_ST77xx

// Choose a type you are using
//#define ST77xx_160X128
//#define ST77xx_160X128_WS
//#define ST77xx_128X128
//#define ST77xx_160X80
//#define ST77xx_135X240
//#define ST77xx_240X240

// Choose the Rotation
//#define ST77xx_ROTATION_UP
//#define ST77xx_ROTATION_RR
//#define ST77xx_ROTATION_RL
//#define ST77xx_ROTATION_DOWN

typedef enum {
  ST77xx_BLK_ON,
  ST77xx_BLK_OFF
} ST77xx_BLK_State;

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST77xx_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST77xx_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST77xx_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST77xx_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST77xx_MADCTL_RGB 0x00
#define ST77xx_MADCTL_BGR 0x08
#define ST77xx_MADCTL_MH  0x04

// Default size. Should be override
#define ST77xx_DEFAULT_SIZE

// AliExpress/eBay 1.8" display
#ifdef ST77xx_160X128
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_160X128 1
#define ST77xx_XSHIFT 0
#define ST77xx_YSHIFT 0
#define ST77xx_USE_CS // For ST7735

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 160
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 128
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 128
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 160
#define ST77xx_ROTATION (0)
#endif

#endif

// WaveShare ST77xxS-based 1.8" display
#ifdef ST77xx_160X128_WS
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_160X128 1
#define ST77xx_USE_CS // For ST7735

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 160
#define ST77xx_XSHIFT 2
#define ST77xx_YSHIFT 1
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 128
#define ST77xx_XSHIFT 1
#define ST77xx_YSHIFT 2
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 128
#define ST77xx_XSHIFT 1
#define ST77xx_YSHIFT 2
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 160
#define ST77xx_XSHIFT 2
#define ST77xx_YSHIFT 1
#define ST77xx_ROTATION (ST77xx_MADCTL_RGB)
#endif

#endif

// 1.44" display
#ifdef ST77xx_128X128
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_128X128 1
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 128
#define ST77xx_USE_CS // For ST7735

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_XSHIFT 2
#define ST77xx_YSHIFT 3
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_XSHIFT 3
#define ST77xx_YSHIFT 2
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_XSHIFT 1
#define ST77xx_YSHIFT 2
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_XSHIFT 2
#define ST77xx_YSHIFT 1
#define ST77xx_ROTATION (ST77xx_MADCTL_BGR)
#endif

#endif

// mini 160x80 display
#ifdef ST77xx_160X80
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_160X80 1
#define ST77xx_USE_CS // For ST7735

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_WIDTH  80
#define ST77xx_HEIGHT 160
#define ST77xx_XSHIFT 26
#define ST77xx_YSHIFT 1
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 80
#define ST77xx_XSHIFT 1
#define ST77xx_YSHIFT 26
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_WIDTH  160
#define ST77xx_HEIGHT 80
#define ST77xx_XSHIFT 1
#define ST77xx_YSHIFT 26
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV | ST77xx_MADCTL_BGR)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_WIDTH  80
#define ST77xx_HEIGHT 160
#define ST77xx_XSHIFT 26
#define ST77xx_YSHIFT 1
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_BGR)
#endif

#endif

/****************************/

// 0.96 inch 7P TFT-LCD Module 135x240
#ifdef ST77xx_135X240
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_135X240 1

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_WIDTH 135
#define ST77xx_HEIGHT 240
#define ST77xx_XSHIFT 53
#define ST77xx_YSHIFT 40
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_WIDTH 240
#define ST77xx_HEIGHT 135
#define ST77xx_XSHIFT 40
#define ST77xx_YSHIFT 52
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_WIDTH 240
#define ST77xx_HEIGHT 135
#define ST77xx_XSHIFT 40
#define ST77xx_YSHIFT 53
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_WIDTH 135
#define ST77xx_HEIGHT 240
#define ST77xx_XSHIFT 52
#define ST77xx_YSHIFT 40
#define ST77xx_ROTATION (ST77xx_MADCTL_RGB)
#endif

#endif

// 1.3 inch 8P TFT-LCD Module 240x240
#ifdef ST77xx_240X240
#undef ST77xx_DEFAULT_SIZE
#define ST77xx_IS_240X240 1
#define ST77xx_WIDTH 240
#define ST77xx_HEIGHT 240

#ifdef ST77xx_ROTATION_UP // default orientation
#define ST77xx_XSHIFT 0
#define ST77xx_YSHIFT 80
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MY | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RR // rotate right
#define ST77xx_XSHIFT 80
#define ST77xx_YSHIFT 0
#define ST77xx_ROTATION (ST77xx_MADCTL_MY | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_RL // rotate left
#define ST77xx_XSHIFT 0
#define ST77xx_YSHIFT 0
#define ST77xx_ROTATION (ST77xx_MADCTL_MX | ST77xx_MADCTL_MV | ST77xx_MADCTL_RGB)
#endif
#ifdef ST77xx_ROTATION_DOWN // upside down
#define ST77xx_XSHIFT 0
#define ST77xx_YSHIFT 0
#define ST77xx_ROTATION (ST77xx_MADCTL_RGB)
#endif

#endif

// Default size. Should be override
#ifdef ST77xx_DEFAULT_SIZE
#define ST77xx_WIDTH  128
#define ST77xx_HEIGHT 128
#define ST77xx_XSHIFT 0
#define ST77xx_YSHIFT 0
#endif

// Error message if no display selected
#if ((!defined(ST77xx_IS_128X128)) && (!defined(ST77xx_IS_160X80)) && (!defined(ST77xx_IS_160X128)) && \
     (!defined(ST77xx_IS_135X240)) && (!defined(ST77xx_IS_240X240)))
#error 	"You should at least choose one display resolution!"
#endif

/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define GRAY 0X8430
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define BROWN 0XBC40
#define BRRED 0XFC07
#define DARKBLUE 0X01CF
#define LIGHTBLUE 0X7D7C
#define GRAYBLUE 0X5458

#define LIGHTGREEN 0X841F
#define LGRAY 0XC618
#define LGRAYBLUE 0XA651
#define LBBLUE 0X2B12
#define ST77xx_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// ST7735
/* Control Registers and constant codes */
#define ST77xx_NOP     0x00
#define ST77xx_SWRESET 0x01
#define ST77xx_RDDID   0x04
#define ST77xx_RDDST   0x09

#define ST77xx_SLPIN   0x10
#define ST77xx_SLPOUT  0x11
#define ST77xx_PTLON   0x12
#define ST77xx_NORON   0x13

#define ST77xx_INVOFF  0x20
#define ST77xx_INVON   0x21
#define ST77xx_DISPOFF 0x28
#define ST77xx_DISPON  0x29
#define ST77xx_CASET   0x2A
#define ST77xx_RASET   0x2B
#define ST77xx_RAMWR   0x2C
#define ST77xx_RAMRD   0x2E

#define ST77xx_PTLAR   0x30
#define ST77xx_COLMOD  0x3A
#define ST77xx_MADCTL  0x36

#define ST77xx_FRMCTR1 0xB1
#define ST77xx_FRMCTR2 0xB2
#define ST77xx_FRMCTR3 0xB3
#define ST77xx_INVCTR  0xB4
#define ST77xx_DISSET5 0xB6

#define ST77xx_PWCTR1  0xC0
#define ST77xx_PWCTR2  0xC1
#define ST77xx_PWCTR3  0xC2
#define ST77xx_PWCTR4  0xC3
#define ST77xx_PWCTR5  0xC4
#define ST77xx_VMCTR1  0xC5

#define ST77xx_RDID1   0xDA
#define ST77xx_RDID2   0xDB
#define ST77xx_RDID3   0xDC
#define ST77xx_RDID4   0xDD

#define ST77xx_PWCTR6  0xFC

#define ST77xx_GMCTRP1 0xE0
#define ST77xx_GMCTRN1 0xE1

// ST7789
// System Function Command Table 1
#define ST77xx_NOP               0x00 // No operation
#define ST77xx_SWRESET           0x01 // Software reset
#define ST77xx_RDDID             0x04 // Read display ID
#define ST77xx_RDDST             0x09 // Read display status
#define ST77xx_RDDPM             0x0A // Read display power
#define ST77xx_RDDMADCTL         0x0B // Read display
#define ST77xx_RDDCOLMOD         0x0C // Read display pixel
#define ST77xx_RDDIM             0x0D // Read display image
#define ST77xx_RDDSM             0x0E // Read display signal
#define ST77xx_RDDSDR            0x0F // Read display self-diagnostic result
#define ST77xx_SLPIN             0x10 // Sleep in
#define ST77xx_SLPOUT            0x11 // Sleep out
#define ST77xx_PTLON             0x12 // Partial mode on
#define ST77xx_NORON             0x13 // Partial off (Normal)
#define ST77xx_INVOFF            0x20 // Display inversion off
#define ST77xx_INVON             0x21 // Display inversion on
#define ST77xx_GAMSET            0x26 // Gamma set
#define ST77xx_DISPOFF           0x28 // Display off
#define ST77xx_DISPON            0x29 // Display on
#define ST77xx_CASET             0x2A // Column address set
#define ST77xx_RASET             0x2B // Row address set
#define ST77xx_RAMWR             0x2C // Memory write
#define ST77xx_RAMRD             0x2E // Memory read
#define ST77xx_PTLAR             0x30 // Partial start/end address set
#define ST77xx_VSCRDEF           0x33 // Vertical scrolling definition
#define ST77xx_TEOFF             0x34 // Tearing line effect off
#define ST77xx_TEON              0x35 // Tearing line effect on
#define ST77xx_MADCTL            0x36 // Memory data access control
#define ST77xx_VSCRSADD          0x37 // Vertical address scrolling
#define ST77xx_IDMOFF            0x38 // Idle mode off
#define ST77xx_IDMON             0x39 // Idle mode on
#define ST77xx_COLMOD            0x3A // Interface pixel format
#define ST77xx_RAMWRC            0x3C // Memory write continue
#define ST77xx_RAMRDC            0x3E // Memory read continue
#define ST77xx_TESCAN            0x44 // Set tear scanline
#define ST77xx_RDTESCAN          0x45 // Get scanline
#define ST77xx_WRDISBV           0x51 // Write display brightness
#define ST77xx_RDDISBV           0x52 // Read display brightness value
#define ST77xx_WRCTRLD           0x53 // Write CTRL display
#define ST77xx_RDCTRLD           0x54 // Read CTRL value display
#define ST77xx_WRCACE            0x55 // Write content adaptive brightness control and Color enhancemnet
#define ST77xx_RDCABC            0x56 // Read content adaptive brightness control
#define ST77xx_WRCABCMB          0x5E // Write CABC minimum brightness
#define ST77xx_RDCABCMB          0x5F // Read CABC minimum brightness
#define ST77xx_RDABCSDR          0x68 // Read Automatic Brightness Control Self-Diagnostic Result
#define ST77xx_RDID1             0xDA // Read ID1
#define ST77xx_RDID2             0xDB // Read ID2
#define ST77xx_RDID3             0xDC // Read ID3

// System Function Command Table 2
#define ST77xx_RAMCTRL           0xB0 // RAM Control
#define ST77xx_RGBCTRL           0xB1 // RGB Control
#define ST77xx_PORCTRL           0xB2 // Porch control
#define ST77xx_FRCTRL1           0xB3 // Frame Rate Control 1
#define ST77xx_GCTRL             0xB7 // Gate control
#define ST77xx_DGMEN             0xBA // Digital Gamma Enable
#define ST77xx_VCOMS             0xBB // VCOM Setting
#define ST77xx_LCMCTRL           0xC0 // LCM Control
#define ST77xx_IDSET             0xC1 // ID Setting
#define ST77xx_VDVVRHEN          0xC2 // VDV and VRH Command enable
#define ST77xx_VRHS              0xC3 // VRH Set
#define ST77xx_VDVSET            0xC4 // VDV Setting
#define ST77xx_VCMOFSET          0xC5 // VCOM Offset Set
#define ST77xx_FRCTR2            0xC6 // FR Control 2
#define ST77xx_CABCCTRL          0xC7 // CABC Control
#define ST77xx_REGSEL1           0xC8 // Register value selection 1
#define ST77xx_REGSEL2           0xCA // Register value selection 2
#define ST77xx_PWMFRSEL          0xCC // PWM Frequency Selection
#define ST77xx_PWCTRL1           0xD0 // Power Control 1
#define ST77xx_VAPVANEN          0xD2 // Enable VAP/VAN signal output
#define ST77xx_CMD2EN            0xDF // Command 2 Enable
#define ST77xx_PVGAMCTRL         0xE0 // Positive Voltage Gamma Control
#define ST77xx_NVGAMCTRL         0xE1 // Negative voltage Gamma Control
#define ST77xx_DGMLUTR           0xE2 // Digital Gamma Look-up Table for Red
#define ST77xx_DGMLUTB           0xE3 // Digital Gamma Look-up Table for Blue
#define ST77xx_GATECTRL          0xE4 // Gate control
#define ST77xx_PWCTRL2           0xE8 // Power Control 2
#define ST77xx_EQCTRL            0xE9 // Equalize Time Control
#define ST77xx_PROMCTRL          0xEC // Program Control
#define ST77xx_PROMEN            0xFA // Program Mode Enable
#define ST77xx_NVMSET            0xFC // NVM Setting
#define ST77xx_PROMACT           0xFE // Program Action

/* Advanced options */
/**
 * Caution: Do not operate these settings
 * You know what you are doing 
 */

#define ST77xx_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)
#define ST77xx_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)

/* Basic functions. */
uint8_t ST77xx_Init(void *pspi, uint8_t RESET_Pin, uint8_t CS_Pin, uint8_t DC_Pin, uint8_t BLK_Pin);
void ST77xx_BLK_OnOff(ST77xx_BLK_State state);
void ST77xx_SetRotation(uint8_t m);
void ST77xx_Fill_Color(uint16_t color);
void ST77xx_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST77xx_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void ST77xx_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Graphical functions. */
void ST77xx_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST77xx_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST77xx_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST77xx_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ST77xx_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void ST77xx_DrawImage_P(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data, bool invert);

/* Text functions. */
void ST77xx_WriteChar(uint16_t x, uint16_t y, char ch, Font_TypeDef font, uint16_t color, uint16_t bgcolor);
void ST77xx_WriteString(uint16_t x, uint16_t y, const char *str, Font_TypeDef font, uint16_t color, uint16_t bgcolor);

/* Extented Graphical functions. */
void ST77xx_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST77xx_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST77xx_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/* Command functions */
void ST77xx_InvertColors(uint8_t invert);
void ST77xx_TearEffect(uint8_t tear);
void ST77xx_DisplayState(uint8_t display);
void ST77xx_IdleMode(uint8_t idle);

/* Simple test function. */
void ST77xx_Test_Screen(void);

#endif
