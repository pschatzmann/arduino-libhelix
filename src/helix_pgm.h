#pragma once

#ifdef ARDUINO
#include <pgm_space.h>
#else

#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))

#endif
