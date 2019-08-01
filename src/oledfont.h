#ifndef _OLEDFONT_

#define _OLEDFONT_

#include <avr/pgmspace.h>

#define FONT_PIXEL_WIDTH 5
#define FONT_PIXEL_HEIGHT 8
#define ASCII_OFFSET 32
#define MAX_CHARS 94

extern const int8_t font_bytes[];


#endif