#include "../include/bmp.h"
#include <stdio.h>
#include <stdlib.h>

void encodeBMP(FILE *file) {
    BMPHeaderFile *bmpHeader = malloc(sizeof(BMPHeaderFile));
    Byte header[2];
    Byte size[4];
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
    unsigned int res = 
        size[0]       |
        size[1] << 8  |
        size[2] << 16 |
        size[3] << 24;
    printf("size of the file: %dkb\n", res);
}
