#ifndef BMP_H
#define BMP_H

#include "../include/utils.h"
#include <stdbool.h>
#include <stdio.h>

#define BMP_HEADER_SIZE                   2
#define BMP_HEADER_SIGNATURE              0x424d // signature of BMP header (BM)
#define BMP_PIXEL_DATA_ADDRESS_OFFSET     10
#define BMP_PIXEL_DATA_ADDRESS_FIELD_SIZE 4
#define BMP_STEG_HEADER_SIZE              32
#define BMP_FILE_SIZE_FIELD_SIZE          4

bool encodeBMP(FILE *file, const char *secretMessage);
char *decodeBMP(FILE *file);

#endif
