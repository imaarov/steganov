#include "../include/wav.h"
#include <stdio.h>

bool signatureIsNotValid(Byte *headerSignature);

bool encodeWAV(FILE *file, char *secretMessage) {
    Byte headerSignature[4];
    if (fread(headerSignature, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read header signature\n");
        return false;
    }
    printf("INfO: the header signature is : %s\n", headerSignature);
    if (signatureIsNotValid(headerSignature)) {
        return false;
    }

    Byte fileSize[4];
    if (fread(fileSize, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read file size in header\n");
        return false;
    }
    unsigned int decimalFileSize = hexaToDecimal(fileSize, 4);
    printf("INFO: file size is : %d kb\n", decimalFileSize + 8);

    Byte format[4];
    if (fread(format, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read file size in header\n");
        return false;
    }
    printf("INFO: format is : %s\n", format);

    return false;
}

char *decodeWAV(FILE *file) {
    return NULL;
}

bool signatureIsNotValid(Byte *headerSignature) {
    unsigned int signature = hexaToDecimal(headerSignature, 4);
    if (signature == WAV_HEADER_SIGNATURE) {
        printf("INFO: the wav header signature is verified\n");
        return false;
    }
    printf("ERROR: the wav header signature is not verified\n");
    return true;
}
