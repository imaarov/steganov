#include "../include/encode.h"
#include "../include/bmp.h"
#include "../include/png.h"
#include "../include/wav.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool encode(char *fileName, char *secretMessage) {
    fileName      = getFileName();
    secretMessage = getSecret();
    if (fileName == NULL || secretMessage == NULL) {
        free(fileName);
        free(secretMessage);
        printf("WARNING: file name or secret message should not be empty\n");
        return false;
    }
    const char *fileExt = getFileExtension(fileName);
    if (fileExt == NULL) {
        FATAL("there is no file extension /:\n");
        return false;
    }
    SupportedExtension ext = getSupportedExtEnum(fileExt);
    DEBUG("supported is : %d\n", ext);
    if (ext == UNDEFINED) {
        FATAL("File type is not supported\n");
        return false;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        FATAL("failed to open the file named: %s\n", fileName);
        return false;
    }
    bool status = false;
    switch (ext) {
    case BMP:
        status = encodeBMP(file, secretMessage);
        break;
    case WAV:
        status = encodeWAV(file, secretMessage);
        break;
    case PNG:
        status = encodePNG(file, secretMessage);
        break;
    case UNDEFINED: // unnecessary
        return false;
    }
    if (!status) {
        FATAL("Failed to encode\n");
        return false;
    }
    DEBUG("Successful encoding operation\n");

    fclose(file);
    return true;
}

void embed_secret_byte_in_file(FILE *file, const Byte secret_byte) {
    for (int b = BITS_PER_BYTE - 1; b >= 0; b--) {
        Byte file_byte  = fgetc(file);
        Byte messageLSB = (secret_byte >> b) & 1;
        file_byte       = (file_byte & LSB_MASK) | messageLSB;

        fseek(file, -1, SEEK_CUR);
        fputc(file_byte, file);
    }
}

void embed_secret_in_file(FILE *file, uint32_t message_length, const char *message) {
    for (uint32_t i = 0; i < message_length; i++) {
        embed_secret_byte_in_file(file, message[i]);
    }
}

void embed_secret_size_in_file(FILE *file, uint32_t message_length) {
    for (int32_t i = BMP_STEG_HEADER_SIZE - 1; i >= 0; i--) {
        Byte c = fgetc(file);
        c      = (c & LSB_MASK) | ((message_length >> i) & 1);

        fseek(file, -1, SEEK_CUR);
        fputc(c, file);
    }
}
