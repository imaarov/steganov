#ifndef WAV_H
#define WAV_H

#include "../include/utils.h"
#include <stdbool.h>
#include <stdio.h>

#define WAV_HEADER_SIZE                 4
#define WAV_HEADER_SIGNATURE            0x46464952    // signature of WAV RIFF header (RIFF) in little endian
#define WAV_FILE_SIZE_FIELD_SIZE        4
#define WAV_FORMAT_FIELD_SIZE           4
#define WAV_CHUNK_ID_FIELD_SIZE         4
#define WAV_CHUNK_SIZE_FIELD_SIZE       4
#define WAV_FMT_AUDIO_FORMAT_SIZE       2
#define WAV_FMT_NUM_CHANNELS_SIZE       2
#define WAV_FMT_SAMPLE_RATE_SIZE        4
#define WAV_FMT_BYTE_RATE_SIZE          4
#define WAV_FMT_BLOCK_ALIGN_SIZE        2
#define WAV_FMT_BITS_PER_SAMPLE_SIZE    2
#define WAV_LIST_TYPE_SIZE              4
#define WAV_LIST_SOFTWARE_ID_FIELD_SIZE 4
#define WAV_LIST_SOFTWARE_ID_SIZE       4
#define WAV_DEFAULT_BITS_PER_SAMPLE     16
#define WAV_STEG_HEADER_SIZE            8
#define WAV_CHUNK_ID_FMT                "fmt "
#define WAV_CHUNK_ID_LIST               "LIST"
#define WAV_CHUNK_ID_DATA               "data"

bool encodeWAV(FILE *file,const char *secretMessage);
char *decodeWAV(FILE *file);

#endif
