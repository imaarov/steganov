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

bool encode(char *file_name, char *secret_message) {
    file_name      = get_file_name();
    secret_message = get_secret();
    if (file_name == NULL || secret_message == NULL) {
        free(file_name);
        free(secret_message);
        printf("WARNING: file name or secret message should not be empty\n");
        return false;
    }
    const char *file_ext = get_file_extension(file_name);
    if (file_ext == NULL) {
        FATAL("there is no file extension /:\n");
        return false;
    }
    SupportedExtension ext = get_supported_ext(file_ext);
    DEBUG("supported is : %d\n", ext);
    if (ext == UNDEFINED) {
        FATAL("File type is not supported\n");
        return false;
    }
    FILE *file = fopen(file_name, "rb+");
    if (file == NULL) {
        FATAL("failed to open the file named: %s\n", file_name);
        return false;
    }
    bool status = false;
    switch (ext) {
    case BMP:
        status = encode_bmp(file, secret_message);
        break;
    case WAV:
        status = encode_wav(file, secret_message);
        break;
    case PNG:
        status = encode_png(file, secret_message);
        break;
    case UNDEFINED: // ik, its unnecessary
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

void embed_secret_byte_in_file(FILE *file, const byte secret_byte) {
    for (int b = BITS_PER_BYTE - 1; b >= 0; b--) {
        byte file_byte   = fgetc(file);
        byte message_lsb = (secret_byte >> b) & 1;
        file_byte        = (file_byte & LSB_MASK) | message_lsb;

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
        byte c = fgetc(file);
        c      = (c & LSB_MASK) | ((message_length >> i) & 1);

        fseek(file, -1, SEEK_CUR);
        fputc(c, file);
    }
}
