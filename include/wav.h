#ifndef WAV_H
#define WAV_H

#include "../include/utils.h"
#include <stdbool.h>
#include <stdio.h>

#define WAV_HEADER_SIZE                 4
#define WAV_HEADER_SIGNATURE            0x46464952 // signature of WAV (RIFF) header
#define WAV_FILE_SIZE_FIELD_SIZE        4
#define WAV_FORMAT_FIELD_SIZE           4
#define WAV_CHUNK_ID_FIELD_SIZE         4
#define WAV_CHUNK_SIZE_FIELD_SIZE       4
#define WAV_FMT_DEFAULT_CHUNK_SIZE      16
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

typedef struct {
    byte signature[WAV_HEADER_SIZE];
    byte size[WAV_FILE_SIZE_FIELD_SIZE];
    byte format[WAV_FORMAT_FIELD_SIZE];
} WAV_Header;

typedef struct {
    byte size[WAV_CHUNK_SIZE_FIELD_SIZE];
    byte audioFormat[WAV_FMT_AUDIO_FORMAT_SIZE];
    byte numChannels[WAV_FMT_NUM_CHANNELS_SIZE];
    byte sampleRate[WAV_FMT_SAMPLE_RATE_SIZE];
    byte byteRate[WAV_FMT_BYTE_RATE_SIZE];
    byte blockAlign[WAV_FMT_BLOCK_ALIGN_SIZE];
    byte bitsPerSample[WAV_FMT_BITS_PER_SAMPLE_SIZE];
} WAV_FMT;

bool encode_wav(FILE *file, const char *secretMessage);
char *decode_wav(FILE *file);

#endif
