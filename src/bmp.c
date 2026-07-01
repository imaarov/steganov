#include "../include/bmp.h"
#include "../include/utils.h"
#include "decode.h"
#include "encode.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool header_signature_is_invalid(const byte header[BMP_HEADER_SIZE]);
bool get_header(FILE *file, byte header[BMP_HEADER_SIZE]);
bool get_size(FILE *file, uint32_t *size);
bool get_file_pixel_offset(FILE *file, uint32_t *offset);
bool encode_bmp_stegano_secret_size(uint32_t message_length, uint32_t file_size,
                                    uint32_t pixel_offset, FILE *file);
void jump_to_pixel_data(FILE *file, uint32_t pixel_offset);
void jump_to_pixel_data_offset_address(FILE *file);

bool encode_bmp(FILE *file, const char *secret_message) {
    byte header[BMP_HEADER_SIZE];
    if (!get_header(file, header)) {
        return false;
    }
    if (header_signature_is_invalid(header)) {
        FATAL("the file signature is not the same as standard BMP signature");
        return false;
    }
    uint32_t file_size = 0;
    if (!get_size(file, &file_size)) {
        return false;
    }
    uint32_t message_length = strlen(secret_message);
    uint32_t pixel_offset   = 0;
    if (!get_file_pixel_offset(file, &pixel_offset)) {
        return false;
    }
    jump_to_pixel_data(file, pixel_offset);

    if (!encode_bmp_stegano_secret_size(message_length, file_size, pixel_offset, file)) {
        FATAL("can not encode steganography header that include secret message "
              "size in to the file");
        return false;
    }

    int32_t available_file_size =
        (int32_t)file_size - (int32_t)pixel_offset - (int32_t)BMP_STEG_HEADER_SIZE;
    if (available_file_size < 0) {
        FATAL("file size corrupted");
        return false;
    }
    if (able_to_encode(available_file_size, message_length * BYTES_NEEDED_FOR_ONE_SECRET_BYTE)) {
        embed_secret_in_file(file, message_length, secret_message);
    } else {
        FATAL("its not possible to store message data in the file because of "
              "the file size");
        DEBUG("available file size: %u, message size allocation: %u", available_file_size,
              message_length * BYTES_NEEDED_FOR_ONE_SECRET_BYTE);
        return false;
    }
    return true;
}

char *decode_bmp(FILE *file) {
    uint32_t pixel_offset = 0;
    if (!get_file_pixel_offset(file, &pixel_offset)) {
        return NULL;
    }
    DEBUG("file pixel offset is %u", pixel_offset);
    jump_to_pixel_data(file, pixel_offset);

    uint32_t secret_byte_size = extract_secret_message_length_from_file(file);
    if (secret_byte_size > MAX_SECRET_MESSAGE_LEN) {
        FATAL("the steganov size header is corrupted");
        return NULL;
    }

    DEBUG("the size of the message is : %u", secret_byte_size);
    char *message = malloc(secret_byte_size + 1);
    if (!message) {
        FATAL("failed to allocate messgae buffer");
        return NULL;
    }
    memset(message, 0, secret_byte_size + 1);
    extract_secret_message_from_file(file, secret_byte_size, message);

    return message;
}

bool get_file_pixel_offset(FILE *file, uint32_t *pixel_offset_out) {
    jump_to_pixel_data_offset_address(file);
    byte pixel_offset_bytes[BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE];
    if (fread(pixel_offset_bytes, sizeof(byte), BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE, file) !=
        BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE) {
        FATAL("failed to read the pixel offset");
        return false;
    }
    *pixel_offset_out =
        hexa_to_decimal(LITTLE_ENDIAN, pixel_offset_bytes, BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE);

    return true;
}

bool encode_bmp_stegano_secret_size(uint32_t message_length, uint32_t file_size,
                                    uint32_t pixel_offset, FILE *file) {
    if (message_length > UINT32_MAX / BYTES_NEEDED_FOR_ONE_SECRET_BYTE) {
        FATAL("message length can cause overflow");
        return false;
    }
    if (able_to_encode(file_size - pixel_offset, message_length * BITS_PER_BYTE)) {
        embed_secret_size_in_file(file, message_length);
    } else {
        FATAL("failed to encode file size, message size exceeds file capacity");
        return false;
    }
    return true;
}
bool get_header(FILE *file, byte header[BMP_HEADER_SIZE]) {
    if (fread(header, sizeof(byte), BMP_HEADER_SIZE, file) != BMP_HEADER_SIZE) {
        FATAL("failed to read the header from file");
        return false;
    }
    return true;
}

bool get_size(FILE *file, uint32_t *file_size) {
    byte size[BMP_FILE_SIZE_FIELD_SIZE];
    if (fread(size, sizeof(byte), BMP_FILE_SIZE_FIELD_SIZE, file) != BMP_FILE_SIZE_FIELD_SIZE) {
        FATAL("failed to read file size from file");
        return false;
    }
    *file_size = hexa_to_decimal(LITTLE_ENDIAN, size, BMP_FILE_SIZE_FIELD_SIZE);
    return true;
}

bool header_signature_is_invalid(const byte header[BMP_HEADER_SIZE]) {
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
