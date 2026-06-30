#include "../include/bmp.h"
#include "../include/utils.h"
#include "decode.h"
#include "encode.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool header_signature_is_not_standard(const Byte header[BMP_HEADER_SIZE]);
Byte *get_header(FILE *file);
Byte *get_size(FILE *file);
Byte *get_file_pixel_offset(FILE *file);
bool encode_bmp_stegano_secret_size(uint32_t message_length, uint32_t file_size,
                                    uint32_t pixel_offset, FILE *file);
void jump_to_pixel_data(FILE *file, uint32_t pixel_offset);
void jump_to_pixel_data_offset_address(FILE *file);

bool encodeBMP(FILE *file, const char *secret_message) {
    if (header_signature_is_not_standard(get_header(file))) {
        FATAL("the file signature is not the same as standard BMP signature");
        return false;
    }
    Byte *size                 = get_size(file);
    uint32_t decimal_file_size = hexaToDecimal(size, BMP_FILE_SIZE_FIELD_SIZE);
    uint32_t message_length    = strlen(secret_message);
    Byte *pixel_offset         = get_file_pixel_offset(file);

    uint32_t decimal_pixel_offset = hexaToDecimal(pixel_offset, BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    jump_to_pixel_data(file, decimal_pixel_offset);

    if (!encode_bmp_stegano_secret_size(message_length, decimal_file_size, decimal_pixel_offset,
                                        file)) {
        FATAL("can not encode steganography header that include secret message "
              "size in to the file");
        goto error;
    }

    int32_t available_file_size =
        (int32_t)decimal_file_size - (int32_t)decimal_pixel_offset - (int32_t)BMP_STEG_HEADER_SIZE;
    if (available_file_size < 0) {
        FATAL("file size corrupted");
        goto error;
    }
    if (ableToEncode(available_file_size, message_length * BYTES_NEEDED_FOR_ONE_SECRET_BYTE)) {
        embed_secret_in_file(file, message_length, secret_message);
    } else {
        FATAL("its not possible to store message data in the file because of "
              "the file size");
        DEBUG("available file size: %u, message size allocation: %u", available_file_size,
              message_length * BYTES_NEEDED_FOR_ONE_SECRET_BYTE);
        goto error;
    }

    free(pixel_offset);
    free(size);
    return true;
error:
    free(pixel_offset);
    free(size);
    return false;
}

char *decodeBMP(FILE *file) {
    Byte *pixel_offset            = get_file_pixel_offset(file);
    uint32_t decimal_pixel_offset = hexaToDecimal(pixel_offset, BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    DEBUG("file pixel offset is %u", decimal_pixel_offset);
    jump_to_pixel_data(file, decimal_pixel_offset);

    uint32_t secret_byte_size = secret_byte_size = extract_secret_message_length_from_file(file);

    DEBUG("the size of the message is : %u", secret_byte_size);
    char *message = malloc(sizeof(char) * secret_byte_size + 1);
    memset(message, 0, secret_byte_size + 1);
    extract_secret_message_from_file(file, secret_byte_size, message);

    free(pixel_offset);
    return message;
}

Byte *get_file_pixel_offset(FILE *file) {
    jump_to_pixel_data_offset_address(file);
    Byte *pixel_offset = malloc(sizeof(Byte) * BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);
    for (int i = 0; i < BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE; i++) {
        pixel_offset[i] = fgetc(file);
    }
    return pixel_offset;
}

bool encode_bmp_stegano_secret_size(uint32_t message_length, uint32_t file_size,
                                    uint32_t pixel_offset, FILE *file) {
    if (ableToEncode(file_size - pixel_offset, message_length * BITS_PER_BYTE)) {
        embed_secret_size_in_file(file, message_length);
    } else {
        FATAL("failed to encode file size, message size exceeds file capacity");
        return false;
    }
    return true;
}
Byte *get_header(FILE *file) {
    Byte *header = malloc(sizeof(Byte) * BMP_HEADER_SIZE);
    if (fread(header, sizeof(Byte), BMP_HEADER_SIZE, file) != BMP_HEADER_SIZE) {
        FATAL("failed to read the header from file");
        return NULL;
    }
    return header;
}

Byte *get_size(FILE *file) {
    Byte *size = malloc(sizeof(Byte) * BMP_FILE_SIZE_FIELD_SIZE);
    if (fread(size, sizeof(Byte), BMP_FILE_SIZE_FIELD_SIZE, file) != BMP_FILE_SIZE_FIELD_SIZE) {
        FATAL("failed to read file size from file");
        return NULL;
    }
    return size;
}

bool header_signature_is_not_standard(const Byte header[BMP_HEADER_SIZE]) {
    DEBUG("check the file header signature");
    int signature = header[0] << BITS_PER_BYTE | header[1];
    if (signature != BMP_HEADER_SIGNATURE) {
        printf("%02x\n", signature);
        return true;
    }
    DEBUG("File Signature is OK");
    return false;
}

void jump_to_pixel_data(FILE *file, uint32_t pixel_offset) { fseek(file, pixel_offset, SEEK_SET); }

void jump_to_pixel_data_offset_address(FILE *file) {
    fseek(file, BMP_PIXEL_DATA_OFFSET_ADDRESS, SEEK_SET);
}
