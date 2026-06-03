#include "../include/encode.h"
#include "../include/bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void encode(char *fileName, char *secretMessage) {
    FILE *file = fopen(fileName, "r");
    BMPHeaderFile *bmpHeader = malloc(sizeof(BMPHeaderFile));
    printf("TRY OPENING: [%s]\n", fileName);
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
        return;
    }

    char *fileExtension = strchr(fileName, '.');
    char *supportedExtension = ".bmp";
    Byte header[2];
    Byte size[4];
    if (!strcmp(fileExtension, supportedExtension)) {
        // Traversing through the bmp header
        for (int i = 0; i < BMP_HEADER_SIZE; i++) {
            Byte c = fgetc(file);
            printf("%02x", c);
            header[i] = c;
        }
        printf("\n");
        // Getting the bmp file size
        int j = 0;
        for (int i = 2; i < 6; i++) {
            Byte c = fgetc(file);
            printf("%02x", c);
            size[j++] = (Byte)c;
        }
    }
    printf("\nthe first 8 : %s\n", header);
}
