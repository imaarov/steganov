#ifndef ADLER32_H
#define ADLER32_H

#include "utils.h"
#include <stdint.h>

#define BASE_MODULO 65521
#define INIT_SUM_A  1
#define INIT_SUM_B  0

uint32_t adler32(const byte *message, uint32_t message_size);

#endif
