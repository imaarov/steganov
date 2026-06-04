#include "../include/encode.h"
#include "../include/bmp.h"
#include <stdio.h>
#include <string.h>

bool encode(char *fileName, char *secretMessage) {
    if (strcmp(".bmp", getFileExtension(fileName))) {
        printf("INFO: File type is not supported\n");
        return false;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
        return false;
    }
    if (!encodeBMP(file, secretMessage)) {
        printf("INFO: Failed to encode\n");
        return false;
    }
    printf("INFO: Successful encoding operation\n");

    fclose(file);
    return true;
}

