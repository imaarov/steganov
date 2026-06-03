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
        encodeBMP(file);
    }
}
