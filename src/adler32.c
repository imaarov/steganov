#include "../include/adler32.h"
#include <stdint.h>

uint32_t adler32(const byte *message, uint32_t message_size) {
    uint32_t sumA = INIT_SUM_A;
    uint32_t sumB = INIT_SUM_B;

    for (uint32_t i = 0; i < message_size; i++) {
        sumA += message[i];
        sumB += sumA;

        sumA %= BASE_MODULO;
        sumB %= BASE_MODULO;
    }

    return (sumB << 16) + sumA;
}
