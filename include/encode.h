#ifndef ENCODE_H
#define ENCODE_H

#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

bool encode();
bool ableToEncode(unsigned int fileDataSize, unsigned int secretMessageSize);
void embed_secret_in_file(FILE *file, uint32_t message_length, const char *message);
void embed_secret_size_in_file(FILE *file, uint32_t message_length);

#endif
