#ifndef __FAST_PRINTF_H
#define __FAST_PRINTF_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  Buffer_Begin,
  Buffer_End
} Buffer_Pos_Def;

void Fast_Set_Decimal_Separator(char decimal);
char *Fast_Pos_Buffer(char *buffer, Buffer_Pos_Def pos_def, uint16_t *buffer_len);
char *Fast_Printf(char *buffer, float value, uint8_t prec, const char *firststring,
		const char *laststring, Buffer_Pos_Def pos_def, uint16_t *end_len);

#endif /* __FAST_PRINTF_H */
