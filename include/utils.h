#ifndef UTILS_H
#define UTILS_H

typedef enum {
    BMP = 0,
    WAV = 1,
    UNDEFINED = 2
} SupportedExtension;

#include <stdbool.h>

typedef unsigned char Byte;
char *getFileExtension(char *fileName);
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);
bool ableToEncode(unsigned int fileSize, unsigned int messaegSize);
SupportedExtension getSupportedExtEnum(const char *fileExt);

#endif
