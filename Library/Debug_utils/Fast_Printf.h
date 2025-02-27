#pragma once

#ifndef __FAST_PRINTF_H
#define __FAST_PRINTF_H

/**
 * Une librairie minimaliste pour formater des réels
 */

#include <stdint.h>
#include <stdbool.h>
#include <initializer_list>

typedef enum {
  Buffer_Begin,
  Buffer_End
} Buffer_Pos_Def;

void Fast_Set_Decimal_Separator(char decimal);
char *Fast_Pos_Buffer(char *buffer, Buffer_Pos_Def pos_def, uint16_t *buffer_len);
char *Fast_Pos_Buffer(char *buffer, const char *string, Buffer_Pos_Def pos_def, uint16_t *buffer_len);
char *Fast_Printf(char *buffer, float value, uint8_t prec, const char *firststring,
		const char *laststring, Buffer_Pos_Def pos_def, uint16_t *end_len);
char *Fast_Printf(char *buffer, uint8_t prec, const char *separator, Buffer_Pos_Def pos_def,
		bool keep_last_separator, uint8_t count, ...);
char *Fast_Printf(char *buffer, uint8_t prec, const char *separator, Buffer_Pos_Def pos_def,
		bool keep_last_separator, std::initializer_list<double> values);
void Fast_Add_EndLine(char *buffer, Buffer_Pos_Def pos_def);

#endif /* __FAST_PRINTF_H */
