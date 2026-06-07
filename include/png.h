#ifndef PNG_H
#define PNG_H

#include <stdbool.h>
#include <stdio.h>

#define PNG_HEADER_SIZE      8
#define PNG_HEADER_SIGNATURE 0x89504E470D0A1A0A // TODO change it to little endian
#define PNG_CHUNK_ID_IHDR    "IHDR"

bool encodePNG(FILE *file, char *secretMessage);


#endif
