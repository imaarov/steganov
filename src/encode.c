#include "../include/encode.h"
#include "../include/bmp.h"
#include "../include/png.h"
#include "../include/wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool encode(char *fileName, char *secretMessage) {
    fileName      = getFileName();
    secretMessage = getSecret();
    if (fileName == NULL || secretMessage == NULL) {
        free(fileName);
        free(secretMessage);
        printf("WARNING: file name or secret message should not be empty\n");
        return false;
    }
    const char *fileExt = getFileExtension(fileName);
    if (fileExt == NULL) {
        FATAL("there is no file extension /:\n");
        return false;
    }
    SupportedExtension ext = getSupportedExtEnum(fileExt);
    DEBUG("supported is : %d\n", ext);
    if (ext == UNDEFINED) {
        FATAL("File type is not supported\n");
        return false;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        FATAL("failed to open the file named: %s\n", fileName);
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
    case PNG:
        status = encodePNG(file, secretMessage);
        break;
    case UNDEFINED: // unnecessary
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
