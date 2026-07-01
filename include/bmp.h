#ifndef BMP_H
#define BMP_H

#include <stdbool.h>
#include <stdio.h>

#define BMP_HEADER_SIZE                   2
#define BMP_HEADER_SIGNATURE              0x424d
#define BMP_PIXEL_DATA_OFFSET_ADDRESS     10
#define BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE 4
#define BMP_STEG_HEADER_SIZE              32
#define BMP_FILE_SIZE_FIELD_SIZE          4

bool encode_bmp(FILE *file, const char *secret_message);
char *decode_bmp(FILE *file);

#endif
