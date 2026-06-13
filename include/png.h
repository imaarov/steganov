#ifndef PNG_H
#define PNG_H

#include <stdbool.h>
#include <stdio.h>

#define PNG_HEADER_SIZE             8
#define PNG_HEADER_SIGNATURE_A      0x474E5089
#define PNG_HEADER_SIGNATURE_B      0x0A1A0A0D
#define PNG_IHDR_LENGTH_FIELD_BYTES 4
#define PNG_IHDR_CHUNK_ID_SIZE      4
#define PNG_IHDR_CHUNK_ID           "IHDR"

bool encodePNG(FILE *file, char *secretMessage);

#endif
