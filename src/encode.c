#include "../include/encode.h"
#include "../include/bmp.h"
#include <stdio.h>
#include <string.h>

bool encode(char *fileName, char *secretMessage) {
    FILE *file = fopen(fileName, "rb+");
    printf("TRY OPENING: [%s]\n", fileName);
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
        return false;
    }

    char *fileExtension = strchr(fileName, '.');
    if (strcmp(".bmp", fileExtension)) {
        printf("INFO: File type is not supported\n");
        return false;
    }
    if (!encodeBMP(file, secretMessage)) {
        printf("INFO: Failed to encode\n");
        return false;
    }
    printf("INFO: Successful encoding operation\n");


    // clean up
    fclose(file);
    return true;
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
