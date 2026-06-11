#include "../include/crc32.h"
#include <stdbool.h>

static u_int32 table[256];
u_int32 polynomial = 0xEDB88320;

bool refIn         = true;
bool refOut        = true;
bool xorOut        = true;

static void init_crc_table();

u_int32 crc32_calculate(Byte *data, u_int32 size) {
    u_int32 crc        = 0xFFFFFFFF; // init crc
    init_crc_table();
    for (int i = 0; i < size; i++) {
        Byte index = (data[i] ^ crc) & 0xFF;
        crc = (crc >> 8) ^ table[index];
    }

    return crc ^ 0xFFFFFFFF;
}

static void init_crc_table() {
    for (u_int32 i = 0; i < 256; i++) {
        u_int32 current_byte = i;
        for (u_int32 b = 0; b < 8; b++) {
            if (0 != (current_byte & 0x00000001)) {
                current_byte  = (current_byte >> 1) ^ polynomial;
            }else {
                current_byte >>= 1;
            }
        }
        table[i] = current_byte;
    }
}
