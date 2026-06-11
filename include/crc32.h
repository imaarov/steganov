#ifndef CRC32
#define crc32

#include "../include/utils.h"


typedef unsigned int u_int32;

u_int32 crc32_calculate(Byte *data, u_int32 size);

#endif
