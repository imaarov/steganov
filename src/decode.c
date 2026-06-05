#include "../include/decode.h"
#include "../include/bmp.h"
#include "../include/wav.h"
#include "../include/utils.h"
#include <stdio.h>
#include <string.h>

char *decode(char *fileName) {
    const char *fileExt = getFileExtension(fileName);
    if (fileExt == NULL) {
        printf("ERROR: there is no file extension /:\n");
        return NULL;
    }
    SupportedExtension ext = getSupportedExtEnum(fileExt);
    if (ext == UNDEFINED) {
        printf("ERROR: file extension is not supported\n");
        return NULL;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        printf("ERROR: file can't open the file\n");
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
        case UNDEFINED:
            printf("INFO: File type is not supported\n");
            fclose(file);
            return NULL;
    }
    if (message == NULL) {
        printf("INFO: decoding operation failed\n");
        fclose(file);
        return NULL;
    }
    printf("INFO: Successful decoding operation\n");
    fclose(file);
    return message;
}
