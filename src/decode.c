#include "../include/decode.h"
#include "../include/bmp.h"
#include "../include/utils.h"
#include <stdio.h>
#include <string.h>

char *decode(char *fileName) {
    if (strcmp(".bmp", getFileExtension(fileName))){
        printf("ERROR: file type is not supported\n");
        return NULL;
    }
    FILE *file = fopen(fileName, "rb+");
    if (file == NULL) {
        printf("ERROR: file can't open the file\n");
        return NULL;
    }
    char *message = decodeBMP(file);
    if (message == NULL) {
        printf("ERROR: failed to extract the message from the file\n");
        return NULL;
    }

    printf("INFO: Successful decoding operation\n");
    fclose(file);
    return message;
}
