#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

typedef unsigned char Byte;
char *getFileExtension(char *fileName);
unsigned int hexaToDecimal(Byte *hexa, unsigned short size);
bool ableToEncode(unsigned int fileSize, unsigned int messaegSize);

#endif
