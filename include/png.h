#ifndef PNG_H
#define PNG_H

#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

#define PNG_HEADER_SIZE            8
#define PNG_HEADER_SIGNATURE_A     0x474E5089
#define PNG_HEADER_SIGNATURE_B     0x0A1A0A0D
#define PNG_CHUNK_ID_SIZE          4
#define PNG_LENGTH_FIELD_SIZE      4
#define PNG_IHDR_CHUNK_ID          "IHDR"
#define PNG_IHDR_WIDTH_FIELD_SIZE  4
#define PNG_IHDR_HEIGHT_FIELD_SIZE 4
#define PNG_IDAT_CHUNK_ID          "IDAT"
#define PNG_IEND_CHUNK_ID          "IEND"
#define PNG_CRC_FIELD_SIZE         4

typedef struct {
    Byte size[PNG_LENGTH_FIELD_SIZE];
    Byte ID[PNG_CHUNK_ID_SIZE];
    Byte width[PNG_IHDR_WIDTH_FIELD_SIZE];
    Byte height[PNG_IHDR_HEIGHT_FIELD_SIZE];
    Byte bitDepth;
    Byte colorType;
    Byte compressionType;
    Byte filterType;
    Byte interlaceMethod;
    Byte gap_pad[3];
} PNGIHDRHeader;

bool encodePNG(FILE *file, char *secretMessage);

#endif
