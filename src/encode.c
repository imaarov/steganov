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

bool encode() {
    bool status            = false;
    char *file_name        = get_file_name();
    char *secret_message   = get_secret();
    char *output_file_name = get_output_file_name();
    FILE *file             = NULL;
    FILE *out              = NULL;

    if (file_name == NULL || secret_message == NULL) {
        printf("WARNING: file name or secret message should not be empty\n");
        goto cleanup;
    }

    if (output_file_name == NULL) {
        printf("ERROR: output file name is NULL, get tf out of here\n");
        goto cleanup;
    }

    const char *file_ext = get_file_extension(file_name);
    if (file_ext == NULL) {
        FATAL("there is no file extension /:\n");
        goto cleanup;
    }
    SupportedExtension ext = get_supported_ext(file_ext);
    DEBUG("supported is : %d\n", ext);
    if (ext == UNDEFINED) {
        FATAL("File type is not supported\n");
        goto cleanup;
    }
    file = fopen(file_name, "rb+");
    out  = fopen(output_file_name, "wb");

    if (file == NULL) {
        FATAL("failed to open the file named: %s\n", file_name);
        goto cleanup;
    }
    switch (ext) {
    case BMP:
        status = encode_bmp(file, secret_message);
        break;
    case WAV:
        status = encode_wav(file, secret_message);
        break;
    case PNG:
        status = encode_png(file, out, secret_message);
        break;
    case UNDEFINED: // ik, its unnecessary
        return false;
    }
    if (!status) {
        FATAL("failed to encode");
        goto cleanup;
    }
    status = true;
    DEBUG("Successful encoding operation\n");

cleanup:
    if (file_name) {
        free(file_name);
    }
    if (output_file_name) {
        free(output_file_name);
    }
    if (secret_message) {
        free(secret_message);
    }
    if (file) {
        fclose(file);
    }
    if (out) {
        fclose(out);
    }
    return status;
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
