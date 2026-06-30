#ifndef DECODE_H
#define DECODE_H

#include "utils.h"
#include <stdint.h>
#include <stdio.h>

char *decode(char *file);
void extract_secret_message_from_file(FILE *file, uint32_t secret_byte_size, char *secret);
Byte extract_secret_message_length_from_file(FILE *file);

#endif
