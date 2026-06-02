#include "../include/encode.h"
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
    char first[8];
    if (strcmp(fileExtension, supportedExtension)) {
        for (int i = 0; i < 13; i++) {
            char c = fgetc(file);
            first[i] = c;
        }
    }
    printf("the first 8 : %s\n", first);
}
