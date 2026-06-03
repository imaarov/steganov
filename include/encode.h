#ifndef ENCODE_H
#define ENCODE_H

typedef unsigned char Byte;

void encode(char *fileName, char *secretMessage);
unsigned int hexaToDecimal(Byte *hexadecimalArray, unsigned short size);
int ableToEncode(unsigned short fileDataSize, unsigned short secretMessageSize);

#endif
