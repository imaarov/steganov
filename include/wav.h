#ifndef WAV_H
#define WAV_H

#include "../include/utils.h"
#include <stdbool.h>
#include <stdio.h>

#define WAV_HEADER_SIZE                 4
#define WAV_HEADER_SIGNATURE            0x46464952    // signature of WAV RIFF header (RIFF) in little endian
#define WAV_DEFAULT_BITS_PER_SAMPLE     16
#define WAV_STEG_HEADER_SIZE            8
#define WAV_CHUNK_ID_FMT                "fmt "
#define WAV_CHUNK_ID_LIST               "LIST"
#define WAV_CHUNK_ID_DATA               "data"

typedef struct WAVHeaderFile {
    Byte headerSignature[4];    // file signature, RIFF
    Byte fileSize[4];           // overall file size minus 8 bytes
    Byte fileFormat[4];         // for example: WAVE (0x45 0x56 0x41 0x57)
    Byte audioFormat[2];        // audio format like PCM (1 = PCM)
    Byte numChannels[2];        // number of channels: 1 mono 2 stero
    Byte sampleRate[4];         // typically 44100
    Byte dataSize[4];
} WAVHeaderFile;

bool encodeWAV(FILE *file, char *secretMessage);
char *decodeWAV(FILE *file);

#endif
