#ifndef ENCODE_H
#define ENCODE_H

#include <stdbool.h>

typedef unsigned char Byte;

bool encode(char *fileName, char *secretMessage);
unsigned int hexaToDecimal(Byte *hexadecimalArray, unsigned short size);
bool ableToEncode(unsigned int fileDataSize, unsigned int secretMessageSize);

#endif
