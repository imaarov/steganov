#ifndef ZLIB_WRAPPER_H
#define ZLIB_WRAPPER_H

#include "utils.h"
#include <stdint.h>

#define ZLIB_HEADER_SIZE  2  // CMG + FLG
#define ZLIB_ADLER32_SIZE 4
#define ZLIB_CM_DEFLATE   8

byte *zlib_decode(const byte *data, uint32_t size, uint32_t *decoded_size);

#endif
