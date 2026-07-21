#include "../include/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_NAME_LEN      255
#define MAX_SECRET_MESSAGE_LEN 255
#define MAX_OUTPUT_FILE_NAME   255

char *get_file_extension(const char *fileName) { return strchr(fileName, '.'); }

uint32_t hexa_to_decimal(const Endian endian, const byte *hexa, const unsigned short size) {
    uint32_t result = 0;
    if (endian == LITTLE_ENDIAN) {
        for (int i = 0; i < size; i++) {
            result |= (hexa[i] << (i * 8));
        }
    } else if (endian == BIG_ENDIAN) {
        for (int i = size - 1, b = 0; i >= 0; i--, b++) {
            result |= (hexa[b] << (i * 8));
        }
    }
    return result;
}

bool able_to_encode(const uint32_t file_data_size, const uint32_t secret_message_size) {
    return file_data_size >= secret_message_size;
}

SupportedExtension get_supported_ext(const char *fileExt) {
    if (!strcmp(fileExt, ".bmp")) {
        return BMP;
    }
    if (!strcmp(fileExt, ".wav")) {
        return WAV;
    }
    if (!strcmp(fileExt, ".png")) {
        return PNG;
    }
    return UNDEFINED;
}

char *get_secret() {
    char *secret_message = malloc(sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    memset(secret_message, 0, sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    printf("enter the secret message you want to be encoded in data: \n");
    void *buff = fgets(secret_message, MAX_SECRET_MESSAGE_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    secret_message[strcspn(secret_message, "\r\n")] = 0;
    return secret_message;
}

char *get_file_name() {
    char *file_name = malloc(sizeof(char) * MAX_FILE_NAME_LEN);
    memset(file_name, 0, sizeof(char) * MAX_FILE_NAME_LEN);
    printf("enter the file name where data should encode/decode there: \n");
    void *buff = fgets(file_name, MAX_FILE_NAME_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    file_name[strcspn(file_name, "\r\n")] = 0;
    return file_name;
}

char *get_output_file_name() {
    char *out_file_name = malloc(sizeof(char) * MAX_OUTPUT_FILE_NAME);
    memset(out_file_name, 0, sizeof(char) * MAX_OUTPUT_FILE_NAME);
    printf("enter the output file name: \n");
    void *buff = fgets(out_file_name, MAX_OUTPUT_FILE_NAME, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user output file name\n");
        return NULL;
    }
    out_file_name[strcspn(out_file_name, "\r\n")] = 0;
    return out_file_name;
}
