#ifndef BMP_H
#define BMP_H

#define BMP_HEADER_SIZE 2
#define BMP_FILE_LEN_SIZE 2

typedef unsigned char Byte;

typedef struct BMPHeaderFile {
    unsigned short startHeader;
    Byte header[BMP_HEADER_SIZE];             // header file of bmp, 0x42 0x4d
    Byte size[4];
} BMPHeaderFile;

#endif
