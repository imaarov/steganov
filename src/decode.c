#include "../include/decode.h"
#include "../include/bmp.h"
#include "../include/png.h"
#include "../include/utils.h"
#include "../include/wav.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *decode() {
    char *file_name = get_file_name();
    if (file_name == NULL) {
        FATAL("file name should not be empty");
        free(file_name);
        return NULL;
    }
    const char *file_ext = get_file_extension(file_name);
    if (file_ext == NULL) {
        FATAL("there is no file extension /:\n");
        free(file_name);
        return NULL;
    }
    SupportedExtension ext = get_supported_ext(file_ext);
    if (ext == UNDEFINED) {
        FATAL("file extension is not supported\n");
        free(file_name);
        return NULL;
    }
    FILE *file = fopen(file_name, "rb+");
    if (file == NULL) {
        FATAL("file can't open the file\n");
        free(file_name);
        return NULL;
    }
    char *message = NULL;
    switch (ext) {
    case BMP:
        message = decode_bmp(file);
        break;
    case WAV:
        message = decode_wav(file);
        break;
    case PNG:
        message = decode_png(file);
        break;
    case UNDEFINED:
        fclose(file);
        free(file_name);
        return NULL;
    }
    if (message == NULL) {
        DEBUG("INFO: decoding operation failed\n");
        fclose(file);
        free(file_name);
        return NULL;
    }
    DEBUG("INFO: Successful decoding operation\n");
    fclose(file);
    free(file_name);
    return message;
}

byte extract_secret_byte_from_file(FILE *file) {
    byte secret_byte = 0;
    for (int bit = BITS_PER_BYTE - 1; bit >= 0; bit--) {
        byte c = fgetc(file);
        c      = (c & 1) << bit;
        secret_byte |= c;
    }
    return secret_byte;
}

void extract_secret_message_from_file(FILE *file, const uint32_t secret_byte_size, char *secret) {
    for (uint32_t i = 0; i < secret_byte_size; i++) {
        secret[i] = (char)extract_secret_byte_from_file(file);
    }
}

byte extract_secret_message_length_from_file(FILE *file) {
    uint32_t secret_byte_size = 0;
    for (int16_t i = BMP_STEG_HEADER_SIZE - 1; i >= 0; i--) {
        int ch = fgetc(file);
        if (EOF == ch) {
            FATAL("reached to the End Of File");
            return 0;
        }
        byte c = (byte)ch;
        c      = (c & 1) << i;
        secret_byte_size |= c;
    }
    return secret_byte_size;
}
