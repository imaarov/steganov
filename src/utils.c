#include "../include/utils.h"
#include <string.h>
#include <stdbool.h>

char *getFileExtension(char *fileName) {
    return strchr(fileName, '.');
}

unsigned int hexaToDecimal(Byte *hexa, unsigned short size) {
    unsigned int result = 0;
    for (int i = 0; i < size; i++) {
        result |= (hexa[i] << (i * 8));
    }
    return result;
}

bool ableToEncode(unsigned int fileDataSize, unsigned int secretMessageSize) {
    if (fileDataSize < secretMessageSize) {
        return 0;
    }
    return 1;
}
