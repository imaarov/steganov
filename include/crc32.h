#ifndef CRC32
#define CRC32

#include "../include/utils.h"
#include <stddef.h>
#include <sys/types.h>

#define CRC32_TABLE_LEN 256
#define CRC32_INIT      0xFFFFFFFF
#define CRC32_XOROUT    0xFFFFFFFF
#define POLYNOMIAL      0xEDB88320

u_int32_t crc32_calculate(const byte *data, size_t size);

#endif
