
#pragma once

#include "Arduino.h"
#include <pgmspace.h>

#define USE_PROGMEM

/**
 * @brief  Font structure used on my LCD libraries
 */
typedef struct {
  uint8_t FontWidth;    /*!< Font width in pixels */
  uint8_t FontHeight;   /*!< Font height in pixels */
  uint8_t CharBytes;    /*!< Count of bytes for one character */
  const uint16_t *data; /*!< Pointer to data font data array */
} Font_TypeDef;

//
//	De 3 fonts
//
extern Font_TypeDef Font_7x10;
extern Font_TypeDef Font_11x18;
extern Font_TypeDef Font_16x26;

/**
 * @brief  Calculates string length and height in units of pixels depending on string and font used
 * @param  *str: String to be checked for length and height
 * @param  *Font: Pointer to @ref FontDef_t font used for calculations
 * @param  *width and *height: width and height of the string with the font used
 * @retval Pointer to string used for length and height
 */
char* Font_GetStringSize(char* str, Font_TypeDef* font, uint8_t *width, uint8_t *height);
