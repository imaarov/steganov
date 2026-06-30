#include "../include/decode.h"
#include "../include/bmp.h"
#include "../include/png.h"
#include "../include/utils.h"
#include "../include/wav.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *decode(char *fileName) {
    fileName = getFileName();
    if (fileName == NULL) {
        FATAL("file name should not be empty");
        return NULL;
    }
    const char *fileExt = getFileExtension(fileName);
    if (fileExt == NULL) {
        FATAL("there is no file extension /:\n");
        return NULL;
    }
    SupportedExtension ext = getSupportedExtEnum(fileExt);
    if (ext == UNDEFINED) {
        FATAL("file extension is not supported\n");
        return NULL;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        FATAL("file can't open the file\n");
        return NULL;
    }
    char *message = NULL;
    switch (ext) {
    case BMP:
        message = decodeBMP(file);
        break;
    case WAV:
        message = decodeWAV(file);
        break;
    case PNG:
        message = decodePNG(file);
        break;
    case UNDEFINED:
        fclose(file);
        return NULL;
    }
    if (message == NULL) {
        DEBUG("INFO: decoding operation failed\n");
        fclose(file);
        return NULL;
    }
    DEBUG("INFO: Successful decoding operation\n");
    fclose(file);
    return message;
}

Byte extract_secret_byte_from_file(FILE *file) {
    Byte secret_byte = 0;
    for (int bit = BITS_PER_BYTE - 1; bit >= 0; bit--) {
        Byte c = fgetc(file);
        c      = (c & 1) << bit;
        secret_byte |= c;
    }
    return secret_byte;
}

void extract_secret_message_from_file(FILE *file, uint32_t secret_byte_size, char *secret) {
    for (uint32_t i = 0; i < secret_byte_size; i++) {
        secret[i] = (char)extract_secret_byte_from_file(file);
    }
}

Byte extract_secret_message_length_from_file(FILE *file) {
    uint32_t secret_byte_size = 0;
    for (int16_t i = BMP_STEG_HEADER_SIZE - 1; i >= 0; i--) {
        int ch = fgetc(file);
        if (EOF == ch) {
            FATAL("reached to the End Of File");
            return 0;
        }
        Byte c = (Byte)ch;
        c      = (c & 1) << i;
        secret_byte_size |= c;
    }
    return secret_byte_size;
}
