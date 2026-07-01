#include "../include/wav.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool signature_is_not_valid(byte *headerSignature);
static bool read_standard_header_info(FILE *file);
static bool read_fmt_chunk(FILE *file);
static bool skip_chunk(FILE *file);
static bool read_list_chunk(FILE *file);
static bool read_chunks_until_data(FILE **file);
static bool encodeWAVSteganoSecretSize(uint32_t secretMessageSize, uint32_t dataChunkSize,
                                       uint8_t bytesPerSample, FILE *file);

uint16_t bits_per_sample = 0;

bool encode_wav(FILE *file, const char *secretMessage) {
    if (!read_standard_header_info(file)) {
        return false;
    }
    if (!read_chunks_until_data(&file)) {
        return false;
    }

    // read the data chunk size, then the actual data chunk starts
    byte dataChunkSize[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(dataChunkSize, sizeof(byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("can not read the data chunk's size");
        return false;
    }
    uint32_t decimalDataChunk =
        hexa_to_decimal(LITTLE_ENDIAN, dataChunkSize, WAV_CHUNK_SIZE_FIELD_SIZE);
    DEBUG("data chunk size: %d\n", decimalDataChunk);
    uint16_t bytesPerSample = bits_per_sample / 8;
    if (bytesPerSample == 0 || bytesPerSample != 2) {
        FATAL("the %u bytes per sample is not currently supported", bytesPerSample);
        return false;
    }
    DEBUG("byte per sample: %u\n", bytesPerSample);
    size_t messageBytesNeeded = strlen(secretMessage) * 8;

    uint32_t secretMessageSize = strlen(secretMessage);
    if (!encodeWAVSteganoSecretSize(secretMessageSize, decimalDataChunk, bytesPerSample, file)) {
        FATAL("can not encode steganography header that include secret message "
              "in to the file");
        return false;
    }
    if (able_to_encode(decimalDataChunk - WAV_STEG_HEADER_SIZE, messageBytesNeeded)) {
        for (size_t i = 0; i < secretMessageSize; i++) {
            for (uint8_t b = 0; b < 8; b++) {
                byte sample[bytesPerSample];
                if (fread(sample, sizeof(byte), bytesPerSample, file) != bytesPerSample) {
                    FATAL("failed to read the a sample of audio");
                    return false;
                }
                // Default 2 bytes per sample
                sample[0] = (sample[0] & 0b11111110) | ((secretMessage[i] >> (7 - b)) & 1);
                fseek(file, -bytesPerSample, SEEK_CUR);
                fputc(sample[0], file);
                fseek(file, bytesPerSample - 1, SEEK_CUR);
            }
        }

    } else {
        FATAL("cannot encode message in this file, message file exceeds file "
              "capacity");
        return false;
    }

    return true;
}

char *decode_wav(FILE *file) {
    if (!read_standard_header_info(file)) {
        return NULL;
    }
    if (!read_chunks_until_data(&file)) {
        FATAL("failed to find data chunk while reading the file");
        return NULL;
    }
    byte dataChunkSize[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(dataChunkSize, sizeof(byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("can not read the data chunk's size");
        return NULL;
    }

    uint32_t message_size     = 0;
    uint16_t bytes_per_sample = bits_per_sample / 8;
    if (bytes_per_sample == 0 || bytes_per_sample != 2) {
        FATAL("the %u bytes per sample is not currently supported", bytes_per_sample);
        return NULL;
    }
    for (uint32_t i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
        byte sample[bytes_per_sample];
        if (fread(sample, sizeof(byte), bytes_per_sample, file) != bytes_per_sample) {
            FATAL("failed to read a sample from file");
            return NULL;
        }
        message_size |= ((sample[0] & 1) << (WAV_STEG_HEADER_SIZE - 1 - i));
    }
    DEBUG("size of the message is : %u", message_size);

    byte *message = malloc(sizeof(byte) * message_size + 1);
    memset(message, 0, message_size + 1); // add null terminator
    for (uint32_t i = 0; i < message_size; i++) {
        byte c = 0;
        for (uint8_t b = 0; b < 8; b++) {
            byte sample[bytes_per_sample];
            if (fread(sample, sizeof(byte), bytes_per_sample, file) != bytes_per_sample) {
                FATAL("failed to read a sample");
                return NULL;
            }
            c |= ((sample[0] & 1) << (7 - b));
        }
        message[i] = c;
        c          = 0;
    }
    DEBUG("secret message is : %s", message);
    return (char *)message;
}

static bool read_standard_header_info(FILE *file) {
    WAV_Header header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        FATAL("failed to read the file header section");
        return false;
    }
    DEBUG("the header signature is : %.4s\n", header.signature);
    if (signature_is_not_valid(header.signature)) {
        return false;
    }
    uint32_t decimal_file_size =
        hexa_to_decimal(LITTLE_ENDIAN, header.size, WAV_FILE_SIZE_FIELD_SIZE);
    DEBUG("INFO: file size is : %d kb", decimal_file_size + 8);

    DEBUG("INFO: format type is : %.4s", header.format);
    return true;
}

static bool read_chunks_until_data(FILE **file) {
    while (1) {
        byte chunk_id[WAV_CHUNK_ID_FIELD_SIZE + 1] = {0};
        if (fread(chunk_id, sizeof(byte), WAV_CHUNK_ID_FIELD_SIZE, *file) !=
            WAV_CHUNK_ID_FIELD_SIZE) {
            FATAL("Failed to read chunk id in header");
            return false;
        }
        printf("\n");
        DEBUG("INFO: chunk id : %.4s", chunk_id);

        if (strcmp(WAV_CHUNK_ID_FMT, (const char *)chunk_id) == 0) {
            if (!read_fmt_chunk(*file)) {
                return false;
            }
            continue;
        }
        if (strcmp(WAV_CHUNK_ID_LIST, (const char *)chunk_id) == 0) {
            if (!read_list_chunk(*file)) {
                return false;
            }
            continue;
        }

        if (strcmp(WAV_CHUNK_ID_DATA, (const char *)chunk_id) == 0) {
            return true;
        }
        skip_chunk(*file);
    }
    return true;
}

static bool read_fmt_chunk(FILE *file) {
    WAV_FMT fmt;
    if (fread(&fmt, sizeof(fmt), 1, file) != 1) {
        FATAL("failed to read the fmt chunk");
        return false;
    }
    uint32_t chunk_size = hexa_to_decimal(LITTLE_ENDIAN, fmt.size, WAV_CHUNK_SIZE_FIELD_SIZE);
    DEBUG("fmt chunk size is : %u", chunk_size);
    DEBUG("audio format is : %u (1 for PCM)",
          hexa_to_decimal(LITTLE_ENDIAN, fmt.audioFormat, WAV_FMT_AUDIO_FORMAT_SIZE));
    DEBUG("number of channels : %u, mono or stereo",
          hexa_to_decimal(LITTLE_ENDIAN, fmt.numChannels, WAV_FMT_NUM_CHANNELS_SIZE));
    DEBUG("sample rate : %uz, %ukz",
          hexa_to_decimal(LITTLE_ENDIAN, fmt.sampleRate, WAV_FMT_SAMPLE_RATE_SIZE),
          hexa_to_decimal(LITTLE_ENDIAN, fmt.sampleRate, WAV_FMT_SAMPLE_RATE_SIZE) / 1000);

    uint32_t decimal_bits_per_sample =
        hexa_to_decimal(LITTLE_ENDIAN, fmt.bitsPerSample, WAV_FMT_BITS_PER_SAMPLE_SIZE);
    bits_per_sample = decimal_bits_per_sample;
    DEBUG("bits per sample is : %d", decimal_bits_per_sample);
    if (chunk_size > WAV_FMT_DEFAULT_CHUNK_SIZE) { // weird but whatever
        DEBUG("FMT chunk size is not standard !!!");
        fseek(file, chunk_size - WAV_FMT_DEFAULT_CHUNK_SIZE, SEEK_CUR);
    }
    return true;
}

static bool skip_chunk(FILE *file) {
    byte size_to_skip[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(size_to_skip, sizeof(byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("failed to read the size of the chunk to skip based on that");
        return false;
    }
    uint32_t skipping_size =
        hexa_to_decimal(LITTLE_ENDIAN, size_to_skip, WAV_CHUNK_SIZE_FIELD_SIZE);
    fseek(file, skipping_size, SEEK_CUR);
    return true;
}

static bool read_list_chunk(FILE *file) {
    byte chunk_size[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(chunk_size, sizeof(byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("failed to read the chunk size in LIST");
        return false;
    }
    uint32_t decimal_chunk_size =
        hexa_to_decimal(LITTLE_ENDIAN, chunk_size, WAV_CHUNK_SIZE_FIELD_SIZE);

    fseek(file, WAV_LIST_TYPE_SIZE, SEEK_CUR);
    byte software_id[WAV_LIST_SOFTWARE_ID_FIELD_SIZE + 1] = {0};
    if (fread(software_id, sizeof(byte), WAV_LIST_SOFTWARE_ID_FIELD_SIZE, file) !=
        WAV_LIST_SOFTWARE_ID_FIELD_SIZE) {
        FATAL("Failed to read software identifier in header");
        return false;
    }
    DEBUG("software identifier : (created by software id): %.4s", software_id);

    if (strcmp("ISFT", (const char *)software_id) == 0) {
        DEBUG("created by software");
        byte software_name_len[WAV_LIST_SOFTWARE_ID_SIZE];
        if (fread(software_name_len, sizeof(byte), WAV_LIST_SOFTWARE_ID_SIZE, file) !=
            WAV_LIST_SOFTWARE_ID_SIZE) {
            FATAL("failed to read software name length in header");
            return false;
        }
        uint32_t decimal_software_name_len =
            hexa_to_decimal(LITTLE_ENDIAN, software_name_len, WAV_LIST_SOFTWARE_ID_SIZE);

        byte softwareName[decimal_software_name_len];
        if (fread(softwareName, sizeof(byte), decimal_software_name_len, file) !=
            decimal_software_name_len) {
            FATAL("Failed to read software name in header");
            return false;
        }
        DEBUG("software name or library used to create the audio: %s", softwareName);
        return true;
    }
    fseek(file, decimal_chunk_size - (WAV_LIST_TYPE_SIZE + WAV_LIST_SOFTWARE_ID_SIZE), SEEK_CUR);

    return true;
}

static bool signature_is_not_valid(byte *header_signature) {
    uint32_t signature = hexa_to_decimal(LITTLE_ENDIAN, header_signature, WAV_HEADER_SIZE);
    if (signature == WAV_HEADER_SIGNATURE) {
        DEBUG("the wav header signature is verified");
        return false;
    }
    FATAL("the wav header signature is not verified");
    return true;
}

static bool encodeWAVSteganoSecretSize(uint32_t secretMessageSize, uint32_t dataChunkSize,
                                       uint8_t bytesPerSample, FILE *file) {
    if (!able_to_encode(dataChunkSize, WAV_STEG_HEADER_SIZE)) {
        FATAL("not enough space in file to write steganography header");
        return false;
    }
    for (uint32_t i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
        byte sample[bytesPerSample];
        if (fread(sample, sizeof(byte), bytesPerSample, file) != bytesPerSample) {
            FATAL("failed to read a sample");
            return false;
        }
        sample[0] =
            (sample[0] & 0b11111110) | (1 & (secretMessageSize >> (WAV_STEG_HEADER_SIZE - 1 - i)));
        fseek(file, -bytesPerSample, SEEK_CUR);
        fputc(sample[0], file);
        fseek(file, bytesPerSample - 1, SEEK_CUR);
    }
    return true;
}
