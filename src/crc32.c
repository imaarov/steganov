#include "../include/crc32.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

static u_int32_t table[CRC32_TABLE_LEN];
static bool table_init = false;
static void init_crc32_table();

u_int32_t crc32_calculate(const byte *data, size_t size) {
    u_int32_t crc = CRC32_INIT;
    init_crc32_table();
    for (size_t i = 0; i < size; i++) {
        byte index = data[i] ^ (byte)crc;
        crc        = (crc >> BITS_PER_BYTE) ^ table[index];
    }

    return crc ^ CRC32_XOROUT;
}

static void init_crc32_table() {
    if (table_init)
        return;
    for (u_int32_t i = 0; i < CRC32_TABLE_LEN; i++) {
        u_int32_t current_byte = i;
        for (size_t b = 0; b < BITS_PER_BYTE; b++) {
            if (0 != (current_byte & 1)) {
                current_byte = (current_byte >> 1) ^ POLYNOMIAL;
            } else {
                current_byte >>= 1;
            }
        }
        table[i] = current_byte;
    }
    table_init = true;
}
