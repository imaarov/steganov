#include "../include/bmp.h"
#include "../include/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool headerSignatureIsNotStandard(const Byte header[BMP_HEADER_SIZE]);
Byte *getHeader(FILE *file);
Byte *getSize(FILE *file);
Byte *getFilePixelOffset(FILE *file);
bool encodeBMPSteganoSecretSize(uint32_t messageLength, uint32_t fileSize, uint32_t pixelOffset,
                                FILE *file);

bool encodeBMP(FILE *file, const char *secretMessage) {
    if (headerSignatureIsNotStandard(getHeader(file))) {
        FATAL("the file signature is not the same as standard BMP signature");
        return false;
    }
    Byte *size               = getSize(file);
    uint32_t decimalFileSize = hexaToDecimal(size, BMP_FILE_SIZE_FIELD_SIZE);
    uint32_t messageLength   = strlen(secretMessage);
    Byte *pixelOffset        = getFilePixelOffset(file);

    uint32_t decimalPixelOffset = hexaToDecimal(pixelOffset, BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    fseek(file, decimalPixelOffset, SEEK_SET);
    if (!encodeBMPSteganoSecretSize(messageLength, decimalFileSize, decimalPixelOffset, file)) {
        FATAL("can not encode steganography header that include secret message "
              "size in to the file");
        free(pixelOffset);
        free(size);
        return false;
    }
    // The available file size is the filesize - (offset to the which raw pixel
    // data begins) - (own steganography header that include size of the secret
    // message)
    int32_t availableFileSize =
        (int32_t)decimalFileSize - (int32_t)decimalPixelOffset - (int32_t)BMP_STEG_HEADER_SIZE;
    if (availableFileSize < 0) {
        FATAL("file size corrupted");
        free(pixelOffset);
        free(size);
        return false;
    }
    if (ableToEncode(availableFileSize, messageLength * 8)) {
        for (uint32_t i = 0; i < messageLength; i++) {
            for (int b = 0; b < 8; b++) {
                Byte c          = fgetc(file);
                Byte messageLSB = (secretMessage[i] >> (7 - b)) & 1;
                c               = (c & 0b11111110) | messageLSB;

                fseek(file, -1, SEEK_CUR);
                fputc(c, file);
            }
        }
    } else {
        FATAL("its not possible to store message data in the file because of "
              "the file size");
        DEBUG("available file size: %u, message size allocation: %u", availableFileSize,
              messageLength * 8);
        free(pixelOffset);
        free(size);
        return false;
    }

    free(pixelOffset);
    free(size);
    return true;
}

char *decodeBMP(FILE *file) {
    Byte *pixelOffset           = getFilePixelOffset(file);
    uint32_t decimalPixelOffset = hexaToDecimal(pixelOffset, BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    DEBUG("file pixel offset is %u", decimalPixelOffset);
    fseek(file, decimalPixelOffset, SEEK_SET);

    uint32_t byteSize = 0;
    for (uint16_t i = 0; i < BMP_STEG_HEADER_SIZE; i++) {
        int ch = fgetc(file);
        if (EOF == ch) {
            FATAL("reached to the End Of File");
            free(pixelOffset);
            return NULL;
        }
        Byte c = (Byte)ch;
        c      = (c & 1) << (BMP_STEG_HEADER_SIZE - 1 - i);
        byteSize |= c;
    }
    DEBUG("the size of the message is : %u", byteSize);
    char *message = malloc(sizeof(char) * byteSize + 1);
    memset(message, 0, byteSize + 1);
    for (uint32_t i = 0; i < byteSize; i++) {
        Byte final = 0;
        for (int b = 0; b < 8; b++) {
            Byte c = fgetc(file);
            c      = (c & 1) << (7 - b);
            final |= c;
        }
        message[i] = final;
        final      = 0;
    }
    free(pixelOffset);
    return message;
}

Byte *getFilePixelOffset(FILE *file) {
    fseek(file, BMP_PIXEL_DATA_ADDRESS_OFFSET, SEEK_SET);
    Byte *pixelOffset = malloc(sizeof(Byte) * BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    for (int i = 0; i < BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE; i++) {
        pixelOffset[i] = fgetc(file);
    }
    return pixelOffset;
}

bool encodeBMPSteganoSecretSize(uint32_t messageLength, uint32_t fileSize, uint32_t pixelOffset,
                                FILE *file) {
    if (ableToEncode(fileSize - pixelOffset, messageLength * 8)) {
        for (uint32_t i = 0; i < BMP_STEG_HEADER_SIZE; i++) {
            Byte c = fgetc(file);
            c      = (c & 0b11111110) | ((messageLength >> (BMP_STEG_HEADER_SIZE - 1 - i)) & 1);

            fseek(file, -1, SEEK_CUR);
            fputc(c, file);
        }
    } else {
        FATAL("failed to encode file size, message size exceeds file capacity");
        return false;
    }
    return true;
}
Byte *getHeader(FILE *file) {
    Byte *header = malloc(sizeof(Byte) * BMP_HEADER_SIZE);
    if (fread(header, sizeof(Byte), BMP_HEADER_SIZE, file) != BMP_HEADER_SIZE) {
        FATAL("failed to read the header from file");
        return NULL;
    }
    return header;
}

Byte *getSize(FILE *file) {
    Byte *size = malloc(sizeof(Byte) * BMP_FILE_SIZE_FIELD_SIZE);
    if (fread(size, sizeof(Byte), BMP_FILE_SIZE_FIELD_SIZE, file) != BMP_FILE_SIZE_FIELD_SIZE) {
        FATAL("failed to read file size from file");
        return NULL;
    }
    return size;
}

bool headerSignatureIsNotStandard(const Byte header[BMP_HEADER_SIZE]) {
    DEBUG("check the file header signature");
    int signature = header[0] << 8 | header[1];
    if (signature != BMP_HEADER_SIGNATURE) {
        printf("%02x\n", signature);
        return true;
    }
    DEBUG("File Signature is OK");
    return false;
}
