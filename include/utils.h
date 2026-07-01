#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef ENABLE_DEBUG
#include <stdio.h>
#define DEBUG(msg, ...) printf("[DEBUG]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define FATAL(msg, ...) printf("[ERROR]: %s:%d:" msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(msg, ...)
#define FATAL(msg, ...)
#endif

#define MAX_FILE_NAME_LEN                255
#define MAX_SECRET_MESSAGE_LEN           255
#define BYTES_NEEDED_FOR_ONE_SECRET_BYTE 8
#define BITS_PER_BYTE                    8
#define LSB_MASK                         0xFE

typedef enum { ENCODE = 1, DECODE = 2, EXIT = 3 } Operation;

typedef enum { BIG_ENDIAN, LITTLE_ENDIAN } Endian;

typedef enum { BMP = 0, WAV = 1, PNG = 2, UNDEFINED = 3 } SupportedExtension;

typedef unsigned char byte;
char *get_file_extension(const char *file_name);
char *get_file_name();
char *get_secret();

uint32_t hexa_to_decimal(const Endian endian, const byte *hexa, const unsigned short size);

bool able_to_encode(const uint32_t fileSize, const unsigned int messaegSize);
SupportedExtension get_supported_ext(const char *fileExt);

#endif
