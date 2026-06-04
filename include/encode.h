#ifndef ENCODE_H
#define ENCODE_H

#include <stdbool.h>

bool encode(char *fileName, char *secretMessage);
bool ableToEncode(unsigned int fileDataSize, unsigned int secretMessageSize);

#endif
