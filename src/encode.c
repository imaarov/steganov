#include "../include/encode.h"
#include "../include/bmp.h"
#include <stdio.h>
#include <string.h>

void encode(char *fileName, char *secretMessage) {
    FILE *file = fopen(fileName, "r");
    printf("TRY OPENING: [%s]\n", fileName);
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
        return;
    }

    char *fileExtension = strchr(fileName, '.');
    char *supportedExtension = ".bmp";
    if (!strcmp(fileExtension, supportedExtension)) {
        encodeBMP(file, secretMessage);
    }
}

unsigned int hexaToDecimal(Byte *hexa, unsigned short size) {
    unsigned int result = 0;
    for (int i = 0; i < size; i++) {
        result |= (hexa[i] << (i * 8));
    }
    return result;
}

int ableToEncode(unsigned short fileDataSize, unsigned short secretMessageSize) {
    if (fileDataSize != secretMessageSize) {
        return 0;
    }
    return 1;
}
