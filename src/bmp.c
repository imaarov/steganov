#include "../include/bmp.h"
#include <stdio.h>
#include <stdlib.h>

int headerSignatureIsNotStandard(Byte header[2]);
Byte* getHeader(FILE *file);
Byte *getSize(FILE *file);
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);

void encodeBMP(FILE *file, char *secretMessage) {
    BMPHeaderFile *bmpHeader = malloc(sizeof(BMPHeaderFile));
    Byte *header = NULL;
    Byte *size   = NULL;
    header = getHeader(file);
    if (headerSignatureIsNotStandard(header)) {
        printf("ERROR: the file signature is not the same as standard BMP signature\n");
        return;
    }
    size = getSize(file);
    unsigned int decimalSize = hexaToDecimal(size, 4);
    
    printf("size of the file: %d KB\n", decimalSize);
}

Byte* getHeader(FILE *file) {
    Byte *header = malloc(sizeof(Byte) * 2);
    for (int i = 0; i < BMP_HEADER_SIZE; i++) {
        Byte c = fgetc(file);
        header[i] = c;
    }
    return header;
}

Byte *getSize(FILE *file) {
    Byte *size = malloc(sizeof(Byte) * 4);
    for (int i = 0; i < 4; i++) {
        Byte c = fgetc(file);
        size[i] = (Byte)c;
    }
    return size;
}

int headerSignatureIsNotStandard(Byte header[2]) {
    printf("CHECKING: check the file header signature\n");
    int signature = header[0] << 8 | header[1];
    if (signature != BMP_HEADER_SIGNATURE) {
        printf("%02x\n", signature);
        return 1;
    }
    printf("CHECKED: File Signature is OK\n");
    return 0;
}

