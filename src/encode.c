#include "../include/encode.h"
#include "../include/bmp.h"
#include "../include/wav.h"
#include <stdio.h>
#include <string.h>

bool encode(char *fileName, char *secretMessage) {
    SupportedExtension ext = getSupportedExtEnum(getFileExtension(fileName));
    if (ext == UNDEFINED) {
        printf("INFO: File type is not supported\n");
        return false;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        printf("failed to open the file named: %s\n", fileName);
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
        case UNDEFINED: // for the sake of my stupid lsp, ik i should not do that
            printf("INFO: File type is not supported\n");
            return false;

    }
    if (!status) {
        printf("INFO: Failed to encode\n");
        return false;
    }
    printf("INFO: Successful encoding operation\n");

    fclose(file);
    return true;
}

