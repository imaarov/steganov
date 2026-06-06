#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#ifdef DEBUG
    #define DEBUG(msg, ...)      \
        printf("[DEBUG]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define FATAL(msg, ...)      \
        printf("[ERROR]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG(msg, ...)
    #define FATAL(msg, ...)
#endif

typedef enum {
    ENCODE = 1,
    DECODE = 2,
    EXIT   = 3
} Operation;

typedef enum {
    BMP = 0,
    WAV = 1,
    UNDEFINED = 2
} SupportedExtension;


typedef unsigned char Byte;
char *getFileExtension(char *fileName);
char* getFileName();
char* getSecret();
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);
bool ableToEncode(unsigned int fileSize, unsigned int messaegSize);
SupportedExtension getSupportedExtEnum(const char *fileExt);

#endif
