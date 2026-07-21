#ifndef PNG_H
#define PNG_H

#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

#define PNG_HEADER_SIZE            8
#define PNG_HEADER_SIGNATURE       0x474E50890A1A0A0D
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
    byte size[PNG_LENGTH_FIELD_SIZE];
    byte ID[PNG_CHUNK_ID_SIZE];
    byte width[PNG_IHDR_WIDTH_FIELD_SIZE];
    byte height[PNG_IHDR_HEIGHT_FIELD_SIZE];
    byte bitDepth;
    byte colorType;
    byte compressionType;
    byte filterType;
    byte interlaceMethod;
    byte gap_pad[3];
} PNGIHDRHeader;

bool encode_png(FILE *file, FILE *out_file, char *secretMessage);
char *decode_png(FILE *file);

#endif
