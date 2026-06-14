#include "../include/wav.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool signatureIsNotValid(Byte *headerSignature);
bool readStandardHeaderInfo(FILE *file);
bool readFMTChunk(FILE *file);
bool skipChunk(FILE *file);
bool readLISTChunk(FILE *file);
bool readChunksUntilData(FILE **file);
bool encodeWAVSteganoSecretSize(uint32_t secretMessageSize,
                                uint32_t dataChunkSize, uint8_t bytesPerSample,
                                FILE *file);

uint16_t bitsPerSample = 0;

bool encodeWAV(FILE *file, const char *secretMessage) {
    if (!readStandardHeaderInfo(file)) {
        return false;
    }
    if (!readChunksUntilData(&file)) {
        return false;
    }

    // read the data chunk size, then the actual data chunk starts
    Byte dataChunkSize[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(dataChunkSize, sizeof(Byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("can not read the data chunk's size");
        return false;
    }
    uint32_t decimalDataChunk =
        hexaToDecimal(dataChunkSize, WAV_CHUNK_SIZE_FIELD_SIZE);
    DEBUG("data chunk size: %d\n", decimalDataChunk);
    uint16_t bytesPerSample = bitsPerSample / 8;
    if (bytesPerSample == 0 || bytesPerSample != 2) {
        FATAL("the %u bytes per sample is not currently supported",
              bytesPerSample);
        return false;
    }
    DEBUG("byte per sample: %u\n", bytesPerSample);
    size_t messageBytesNeeded = strlen(secretMessage) * 8;

    uint32_t secretMessageSize = strlen(secretMessage);
    if (!encodeWAVSteganoSecretSize(secretMessageSize, decimalDataChunk,
                                    bytesPerSample, file)) {
        FATAL("can not encode steganography header that include secret message "
              "in to the file");
        return false;
    }
    if (ableToEncode(decimalDataChunk - WAV_STEG_HEADER_SIZE,
                     messageBytesNeeded)) {
        for (size_t i = 0; i < secretMessageSize; i++) {
            for (uint8_t b = 0; b < 8; b++) {
                Byte sample[bytesPerSample];
                if (fread(sample, sizeof(Byte), bytesPerSample, file) !=
                    bytesPerSample) {
                    FATAL("failed to read the a sample of audio");
                    return false;
                }
                // Default 2 bytes per sample
                sample[0] = (sample[0] & 0b11111110) |
                            ((secretMessage[i] >> (7 - b)) & 1);
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

char *decodeWAV(FILE *file) {
    if (!readStandardHeaderInfo(file)) {
        return NULL;
    }
    if (!readChunksUntilData(&file)) {
        FATAL("failed to find data chunk while reading the file");
        return NULL;
    }
    Byte dataChunkSize[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(dataChunkSize, sizeof(Byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("can not read the data chunk's size");
        return NULL;
    }

    uint32_t messageSize = 0;
    uint16_t bytesPerSample = bitsPerSample / 8;
    if (bytesPerSample == 0 || bytesPerSample != 2) {
        FATAL("the %u bytes per sample is not currently supported",
              bytesPerSample);
        return NULL;
    }
    for (uint32_t i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
        Byte sample[bytesPerSample];
        if (fread(sample, sizeof(Byte), bytesPerSample, file) !=
            bytesPerSample) {
            FATAL("failed to read a sample from file");
            return NULL;
        }
        messageSize |= ((sample[0] & 1) << (WAV_STEG_HEADER_SIZE - 1 - i));
    }
    DEBUG("size of the message is : %u", messageSize);

    Byte *message = malloc(sizeof(Byte) * messageSize + 1);
    memset(message, 0, messageSize + 1); // add null terminator
    for (uint32_t i = 0; i < messageSize; i++) {
        Byte c = 0;
        for (uint8_t b = 0; b < 8; b++) {
            Byte sample[bytesPerSample];
            if (fread(sample, sizeof(Byte), bytesPerSample, file) !=
                bytesPerSample) {
                FATAL("failed to read a sample");
                return NULL;
            }
            c |= ((sample[0] & 1) << (7 - b));
        }
        message[i] = c;
        c = 0;
    }
    DEBUG("secret message is : %s", message);
    return (char *)message;
}

bool readStandardHeaderInfo(FILE *file) {
    WAV_Header header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        FATAL("failed to read the file header section");
        return false;
    }
    DEBUG("the header signature is : %.4s\n", header.signature);
    if (signatureIsNotValid(header.signature)) {
        return false;
    }
    uint32_t decimalFileSize =
        hexaToDecimal(header.size, WAV_FILE_SIZE_FIELD_SIZE);
    DEBUG("INFO: file size is : %d kb", decimalFileSize + 8);

    DEBUG("INFO: format type is : %.4s", header.format);
    return true;
}

bool readChunksUntilData(FILE **file) {
    while (1) {
        Byte chunkID[WAV_CHUNK_ID_FIELD_SIZE + 1] = {0};
        if (fread(chunkID, sizeof(Byte), WAV_CHUNK_ID_FIELD_SIZE, *file) !=
            WAV_CHUNK_ID_FIELD_SIZE) {
            FATAL("Failed to read chunk id in header");
            return false;
        }
        printf("\n");
        DEBUG("INFO: chunk id : %.4s", chunkID);

        if (strcmp(WAV_CHUNK_ID_FMT, (const char *)chunkID) == 0) {
            if (!readFMTChunk(*file)) {
                return false;
            }
            continue;
        }
        if (strcmp(WAV_CHUNK_ID_LIST, (const char *)chunkID) == 0) {
            if (!readLISTChunk(*file)) {
                return false;
            }
            continue;
        }

        if (strcmp(WAV_CHUNK_ID_DATA, (const char *)chunkID) == 0) {
            return true;
        }
        skipChunk(*file);
    }
    return true;
}

bool readFMTChunk(FILE *file) {
    WAV_FMT fmt;
    if (fread(&fmt, sizeof(fmt), 1, file) != 1) {
        FATAL("failed to read the fmt chunk");
        return false;
    }
    uint32_t chunkSize = hexaToDecimal(fmt.size, WAV_CHUNK_SIZE_FIELD_SIZE);
    DEBUG("fmt chunk size is : %u", chunkSize);
    DEBUG("audio format is : %u (1 for PCM)",
          hexaToDecimal(fmt.audioFormat, WAV_FMT_AUDIO_FORMAT_SIZE));
    DEBUG("number of channels : %u, mono or stereo",
          hexaToDecimal(fmt.numChannels, WAV_FMT_NUM_CHANNELS_SIZE));
    DEBUG("sample rate : %uz, %ukz",
          hexaToDecimal(fmt.sampleRate, WAV_FMT_SAMPLE_RATE_SIZE),
          hexaToDecimal(fmt.sampleRate, WAV_FMT_SAMPLE_RATE_SIZE) / 1000);

    uint32_t decimalBitsPerSample =
        hexaToDecimal(fmt.bitsPerSample, WAV_FMT_BITS_PER_SAMPLE_SIZE);
    bitsPerSample = decimalBitsPerSample;
    DEBUG("bits per sample is : %d", decimalBitsPerSample);
    if (chunkSize > WAV_FMT_DEFAULT_CHUNK_SIZE) { // weird but whatever
        DEBUG("FMT chunk size is not standard !!!");
        fseek(file, chunkSize - WAV_FMT_DEFAULT_CHUNK_SIZE, SEEK_CUR);
    }
    return true;
}

bool skipChunk(FILE *file) {
    Byte sizeToSkip[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(sizeToSkip, sizeof(Byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("failed to read the size of the chunk to skip based on that");
        return false;
    }
    uint32_t skippingSize =
        hexaToDecimal(sizeToSkip, WAV_CHUNK_SIZE_FIELD_SIZE);
    fseek(file, skippingSize, SEEK_CUR);
    return true;
}

bool readLISTChunk(FILE *file) {
    Byte chunkSize[WAV_CHUNK_SIZE_FIELD_SIZE];
    if (fread(chunkSize, sizeof(Byte), WAV_CHUNK_SIZE_FIELD_SIZE, file) !=
        WAV_CHUNK_SIZE_FIELD_SIZE) {
        FATAL("failed to read the chunk size in LIST");
        return false;
    }
    uint32_t decimalChunkSize =
        hexaToDecimal(chunkSize, WAV_CHUNK_SIZE_FIELD_SIZE);

    fseek(file, WAV_LIST_TYPE_SIZE, SEEK_CUR);
    Byte softwareId[WAV_LIST_SOFTWARE_ID_FIELD_SIZE + 1] = {0};
    if (fread(softwareId, sizeof(Byte), WAV_LIST_SOFTWARE_ID_FIELD_SIZE,
              file) != WAV_LIST_SOFTWARE_ID_FIELD_SIZE) {
        FATAL("Failed to read software identifier in header");
        return false;
    }
    DEBUG("software identifier : (created by software id): %.4s", softwareId);

    if (strcmp("ISFT", (const char *)softwareId) == 0) {
        DEBUG("created by software");
        Byte softwareNameLen[WAV_LIST_SOFTWARE_ID_SIZE];
        if (fread(softwareNameLen, sizeof(Byte), WAV_LIST_SOFTWARE_ID_SIZE,
                  file) != WAV_LIST_SOFTWARE_ID_SIZE) {
            FATAL("failed to read software name length in header");
            return false;
        }
        uint32_t decimalSoftwareNameLen =
            hexaToDecimal(softwareNameLen, WAV_LIST_SOFTWARE_ID_SIZE);

        Byte softwareName[decimalSoftwareNameLen];
        if (fread(softwareName, sizeof(Byte), decimalSoftwareNameLen, file) !=
            decimalSoftwareNameLen) {
            FATAL("Failed to read software name in header");
            return false;
        }
        DEBUG("software name or library used to create the audio: %s",
              softwareName);
        return true;
    }
    fseek(file,
          decimalChunkSize - (WAV_LIST_TYPE_SIZE + WAV_LIST_SOFTWARE_ID_SIZE),
          SEEK_CUR);

    return true;
}

bool signatureIsNotValid(Byte *headerSignature) {
    uint32_t signature = hexaToDecimal(headerSignature, WAV_HEADER_SIZE);
    if (signature == WAV_HEADER_SIGNATURE) {
        DEBUG("the wav header signature is verified");
        return false;
    }
    FATAL("the wav header signature is not verified");
    return true;
}

bool encodeWAVSteganoSecretSize(uint32_t secretMessageSize,
                                uint32_t dataChunkSize, uint8_t bytesPerSample,
                                FILE *file) {
    if (!ableToEncode(dataChunkSize, WAV_STEG_HEADER_SIZE)) {
        FATAL("not enough space in file to write steganography header");
        return false;
    }
    for (uint32_t i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
        Byte sample[bytesPerSample];
        if (fread(sample, sizeof(Byte), bytesPerSample, file) !=
            bytesPerSample) {
            FATAL("failed to read a sample");
            return false;
        }
        sample[0] = (sample[0] & 0b11111110) |
                    (1 & (secretMessageSize >> (WAV_STEG_HEADER_SIZE - 1 - i)));
        fseek(file, -bytesPerSample, SEEK_CUR);
        fputc(sample[0], file);
        fseek(file, bytesPerSample - 1, SEEK_CUR);
    }
    return true;
}
