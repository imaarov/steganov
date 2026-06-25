#include "../include/crc32.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

static u_int32_t table[256];
static bool tableInit = false;
static void init_crc32_table();

u_int32_t crc32_calculate(const Byte *data, size_t size) {
    u_int32_t crc = CRC32_INIT;
    init_crc32_table();
    for (size_t i = 0; i < size; i++) {
        Byte index = data[i] ^ (Byte)crc;
        crc        = (crc >> 8) ^ table[index];
    }

    return crc ^ CRC32_XOROUT;
}

static void init_crc32_table() {
    if (tableInit)
        return;
    for (u_int32_t i = 0; i < 256; i++) {
        u_int32_t current_byte = i;
        for (size_t b = 0; b < 8; b++) {
            if (0 != (current_byte & 1)) {
                current_byte = (current_byte >> 1) ^ POLYNOMIAL;
            } else {
                current_byte >>= 1;
            }
        }
        table[i] = current_byte;
    }
    tableInit = true;
}
