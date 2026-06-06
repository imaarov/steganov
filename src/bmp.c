#include "../include/bmp.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool headerSignatureIsNotStandard(Byte header[2]);
Byte* getHeader(FILE *file);
Byte *getSize(FILE *file);
Byte* getFilePixelOffset(FILE *file);
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);

bool encodeBMP(FILE *file, char *secretMessage) {
    //BMPHeaderFile *bmpHeader = malloc(sizeof(BMPHeaderFile));
    Byte *header = NULL;
    Byte *size   = NULL;
    header = getHeader(file);
    if (headerSignatureIsNotStandard(header)) {
        FATAL("the file signature is not the same as standard BMP signature");
        free(header);
        return 0;
    }
    size = getSize(file);
    unsigned int decimalFileSize = hexaToDecimal(size, 4);
    unsigned int messageLength = strlen(secretMessage);
    // after traversing the header and size, the file pointer should be at (4 + 2) = 6
    // at bytes 10 to 13, we have the value that indicates where the offset to the pixel data resides
    Byte *pixelOffset = NULL;
    fseek(file, 4, SEEK_CUR);
    pixelOffset = getFilePixelOffset(file);
    
    unsigned int decimalPixelOffset = hexaToDecimal(pixelOffset, 4);
    fseek(file, decimalPixelOffset, SEEK_SET);
    // encode the size of our message in a fixed position in the file
    // the fixed position for locating the message size is 54 to 58 (typically the size of an unsigned int is 4 bytes)
    unsigned int sizeOfByteMessage = sizeof(messageLength);
    unsigned int sizeInBit = sizeOfByteMessage * 8;
    if (ableToEncode(decimalFileSize - decimalPixelOffset, sizeOfByteMessage * 8)) {
        for (unsigned int i = 0; i < sizeInBit; i++) {
            Byte c = fgetc(file);
            c = (c & 0b11111110) | ((messageLength >> (sizeInBit - 1 - i)) & 0b00000001);

            fseek(file, -1, SEEK_CUR);
            fputc(c, file);
        }
    } else {
        FATAL("failed to encode file size, message size exceeds file capacity");
        free(header);
        free(size);
        return false;
    }

    // for secret message sizes I multiplied it by 8, because it's LSB way for now, and
    // each bit of a message needs the least significant bit of a BYTE to be altered
    // so in general, for each byte of a secret character we need 8 bytes to store it
    // because of the nature of LSB
    // we don't want to corrupt a file by editing the header area and read-only area
    // the decimalSize - decimalPixelOffset is for that, we don't count them
    // because those are header bytes + reserved byte positions
    if (ableToEncode(decimalFileSize - decimalPixelOffset - 32, messageLength * 8)) {
        for (unsigned int i = 0; i < messageLength; i++) {
            for (int b = 0; b < 8; b++) {
                Byte c = fgetc(file);
                Byte bitMessage = (secretMessage[i] >> (7 - b)) & 0b00000001;
                c = (c & 0x11111110) | bitMessage;

                fseek(file, -1, SEEK_CUR);
                fputc(c, file);
            }
        }
    }else {
        FATAL("its not possible to store message data in the file because of the file size");
        DEBUG("file size: %u, message size: %lu\n", decimalFileSize, strlen(secretMessage));
        free(header);
        free(size);
        return 0;
    }


    // clean up
    free(header);
    free(size);
    return 1;
}

char *decodeBMP(FILE *file) {
    // goto position 10 to read the pixel offset
    fseek(file, 10, SEEK_SET);
    Byte *pixelOffset = getFilePixelOffset(file);
    unsigned int decimalPixelOffset = hexaToDecimal(pixelOffset, 4);
    DEBUG("file pixel offset is %u\n", decimalPixelOffset);
    fseek(file, decimalPixelOffset, SEEK_SET);

    unsigned int sizeInBit = sizeof(unsigned int) * 8;
    Byte byteSize = 0;
    for (unsigned int i = 0; i < sizeInBit; i++) {
        Byte c = fgetc(file);
        c = (c & 0b00000001) << (sizeInBit - 1 - i);
        byteSize |= c;
    }
    DEBUG("the size of the message is : %u\n", byteSize);
    char *message = malloc(sizeof(char) * byteSize + 1);
    memset(message, 0, byteSize + 1);
    for (int i = 0; i < byteSize; i++) {
        Byte final = 0;
        for (int b = 0; b < 8; b++) {
            Byte c = fgetc(file);
            c = (c & 0b00000001) << (7 - b);
            final |= c;
        }
        message[i] = final;
        final = 0;
    }
    free(pixelOffset);
    return message;
}

Byte* getFilePixelOffset(FILE *file) {
    Byte *pixelOffset = malloc(sizeof(Byte) * 4);
    for (int i = 0; i < 4; i++) {
        pixelOffset[i] = fgetc(file);
    }
    return pixelOffset;
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
    DEBUG("check the file header signature\n");
    int signature = header[0] << 8 | header[1];
    if (signature != BMP_HEADER_SIGNATURE) {
        printf("%02x\n", signature);
        return 1;
    }
    DEBUG("File Signature is OK\n");
    return 0;
}

