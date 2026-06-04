#include "../include/encode.h"
#include "../include/bmp.h"
#include <stdio.h>
#include <string.h>

char *getFileExtension(char *fileName);

bool encode(char *fileName, char *secretMessage) {
    FILE *file = fopen(fileName, "rb+");
    printf("TRY OPENING: [%s]\n", fileName);
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
        return false;
    }
    if (strcmp(".bmp", getFileExtension(fileName))) {
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



