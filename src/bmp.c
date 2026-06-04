#include "../include/bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool headerSignatureIsNotStandard(Byte header[2]);
Byte* getHeader(FILE *file);
Byte *getSize(FILE *file);
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);

bool encodeBMP(FILE *file, char *secretMessage) {
    //BMPHeaderFile *bmpHeader = malloc(sizeof(BMPHeaderFile));
    Byte *header = NULL;
    Byte *size   = NULL;
    header = getHeader(file);
    if (headerSignatureIsNotStandard(header)) {
        printf("ERROR: the file signature is not the same as standard BMP signature\n");
        free(header);
        return 0;
    }
    size = getSize(file);
    unsigned int decimalFileSize = hexaToDecimal(size, 4);
    unsigned int messageLength = strlen(secretMessage);
    // after header and size traversing, the file pointer should be in (4 + 2) = 6
    // in the address of 10th byte to 13  we have the byte, where the pixels data is reside
    Byte pixelOffset[4];
    fseek(file, 4, SEEK_CUR);
    for (int i = 0; i < 4; i++) {
        pixelOffset[i] = fgetc(file);
    }
    unsigned int decimalPixelOffset = hexaToDecimal(pixelOffset, 4);
    fseek(file, decimalPixelOffset, SEEK_SET);
    // for secret message sizes I multiplied it with 8, cause its LSB way for now, and 
    // each bit of message need the least significant bit of a BYTE to alter that
    // so in general for each byte of secret character we need 8 bytes to store them 
    // because of the nature of the LSB
    // we don't want to corrupt a file by editing the header area and read only area
    // the decimalSize - decimalPixelOffset its for that, we don't count them
    // cause those are header byte + reserved byte position
    if (ableToEncode(decimalFileSize - decimalPixelOffset, messageLength * 8)) {
        for (unsigned int i = 0; i < messageLength; i++) {
            Byte mask = 0b10000000;
            for (int b = 0; b < 8; b++) {
                Byte c = fgetc(file);
                Byte bitMessage = (secretMessage[i] >> (7 - b)) & 0b00000001;
                c = (c & 0x11111110) | bitMessage;

                fseek(file, -1, SEEK_CUR);
                fputc(c, file);
                mask >>= 1;
            }
        }
    }else {
        printf("ERROR: its not possible to store message data in the file because of the file size\n");
        printf("INFO: file size: %u, message size: %lu\n", decimalFileSize, strlen(secretMessage));
        return 0;
    }


    // clean up
    free(header);
    free(size);
    return 1;
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

bool headerSignatureIsNotStandard(Byte header[2]) {
    printf("CHECKING: check the file header signature\n");
    int signature = header[0] << 8 | header[1];
    if (signature != BMP_HEADER_SIGNATURE) {
        printf("%02x\n", signature);
        return 1;
    }
    printf("CHECKED: File Signature is OK\n");
    return 0;
}

