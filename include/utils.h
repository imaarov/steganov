#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#ifdef DEBUG
#define DEBUG(msg, ...)                                                        \
    printf("[DEBUG]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define FATAL(msg, ...)                                                        \
    printf("[ERROR]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(msg, ...)
#define FATAL(msg, ...)
#endif
#define MAX_FILE_NAME_LEN      255
#define MAX_SECRET_MESSAGE_LEN 255

typedef enum { ENCODE = 1, DECODE = 2, EXIT = 3 } Operation;

typedef enum { BMP = 0, WAV = 1, PNG = 2, UNDEFINED = 3 } SupportedExtension;

typedef unsigned char Byte;
char *getFileExtension(char *fileName);
char *getFileName();
char *getSecret();
// little-endian
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);
// big-endian
unsigned int hexaToDecimalBigEndian(Byte *hexa, unsigned short size);
bool ableToEncode(unsigned int fileSize, unsigned int messaegSize);
SupportedExtension getSupportedExtEnum(const char *fileExt);

#endif
