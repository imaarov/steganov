#define MAX_FILE_NAME_LEN      255
#define MAX_SECRET_MESSAGE_LEN 255
#include "../include/utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *getFileExtension(char *fileName) { return strchr(fileName, '.'); }

unsigned int hexaToDecimal(Byte *hexa, unsigned short size) {
    unsigned int result = 0;
    for (int i = 0; i < size; i++) {
        result |= (hexa[i] << (i * 8));
    }
    return result;
}

unsigned int hexaToDecimalBigEndian(Byte *hexa, unsigned short size) {
    unsigned int result = 0;
    for (int i = size - 1, b = 0; i >= 0; i--, b++) {
        result |= (hexa[b] << (i * 8));
    }
    return result;
}

bool ableToEncode(unsigned int fileDataSize, unsigned int secretMessageSize) {
    return fileDataSize >= secretMessageSize;
}

SupportedExtension getSupportedExtEnum(const char *fileExt) {
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

char *getSecret() {
    char *secretMessage = malloc(sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    memset(secretMessage, 0, sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    printf("enter the secret message you want to be encoded in data: \n");
    void *buff = fgets(secretMessage, MAX_SECRET_MESSAGE_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    secretMessage[strcspn(secretMessage, "\r\n")] = 0;
    return secretMessage;
}

char *getFileName() {
    char *fileName = malloc(sizeof(char) * MAX_FILE_NAME_LEN);
    memset(fileName, 0, sizeof(char) * MAX_FILE_NAME_LEN);
    printf("enter the file name where data should encode/decode there: \n");
    void *buff = fgets(fileName, MAX_FILE_NAME_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    fileName[strcspn(fileName, "\r\n")] = 0;
    return fileName;
}
