#include "../include/decode.h"
#include "../include/bmp.h"
#include "../include/png.h"
#include "../include/utils.h"
#include "../include/wav.h"
#include <stdio.h>
#include <string.h>

char *decode(char *fileName) {
    fileName = getFileName();
    if (fileName == NULL) {
        FATAL("file name should not be empty");
        return NULL;
    }
    const char *fileExt = getFileExtension(fileName);
    if (fileExt == NULL) {
        FATAL("there is no file extension /:\n");
        return NULL;
    }
    SupportedExtension ext = getSupportedExtEnum(fileExt);
    if (ext == UNDEFINED) {
        FATAL("file extension is not supported\n");
        return NULL;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        FATAL("file can't open the file\n");
        return NULL;
    }
    char *message = NULL;
    switch (ext) {
    case BMP:
        message = decodeBMP(file);
        break;
    case WAV:
        message = decodeWAV(file);
        break;
    case PNG:
        message = decodePNG(file);
        break;
    case UNDEFINED:
        fclose(file);
        return NULL;
    }
    if (message == NULL) {
        DEBUG("INFO: decoding operation failed\n");
        fclose(file);
        return NULL;
    }
    DEBUG("INFO: Successful decoding operation\n");
    fclose(file);
    return message;
}
