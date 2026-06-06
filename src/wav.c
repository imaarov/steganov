#include "../include/wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool signatureIsNotValid(Byte *headerSignature);
bool readStandardHeaderInfo(FILE *file);
bool readFMTChunk(FILE *file);
bool skipChunk(FILE *file);
bool readLISTChunk(FILE *file);
bool readChunksUntilData(FILE **file);

bool encodeWAV(FILE *file, char *secretMessage) {
    // read standard header information, signature, size, format type
    if (!readStandardHeaderInfo(file)) {
        return false;
    }
    // read all of the chunks after the standard header until the data chunk
    if (!readChunksUntilData(&file)) {
        return false;
    }

    // read data chunk id in readChunksUntilData functoin
    // now read the data chunk size, then the actual data chunk starts
    Byte dataChunkSize[4];
    if (fread(dataChunkSize, sizeof(Byte), 4, file) != 4) {
        FATAL("can not read the data chunk's size");
        return false;
    }
    unsigned int decimalDataChunk = hexaToDecimal(dataChunkSize, 4);
    DEBUG("data chunk size: %d\n", decimalDataChunk);
    if (ableToEncode(decimalDataChunk - WAV_STEG_HEADER_SIZE, strlen(secretMessage) * 8)) {
        // embed the header of the steganography, which is the size of unsigned int, containing message size
        unsigned char secretMessageSize = strlen(secretMessage);
        for (unsigned int i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
            Byte sample[2];                                             //TODO here we ASSUME(which is bad idea to assume anything
                                                                        //in this project) that each sample is 16bit (bits per sample)
                                                                        //so i should fix it
            if (fread(sample, sizeof(Byte), 2, file) != 2) {
                FATAL("failed to read a sample");
                return false;
            }
            sample[0] = (sample[0] & 0b11111110) | (0b00000001 & (secretMessageSize >> (WAV_STEG_HEADER_SIZE - 1 - i)));
            fseek(file, -2, SEEK_CUR);
            fputc(sample[0], file);
            fseek(file, 1, SEEK_CUR);
        }

        for (int i = 0; i < secretMessageSize; i++) {
            for (int b = 0; b < 8; b++) {
                Byte sample[2];
                if (fread(sample, sizeof(Byte), 2, file) != 2) {
                    FATAL("failed to read the a sample of audio");
                    return false;
                }
                sample[0] = (sample[0] & 0b11111110) | ((secretMessage[i] >> (7 - b)) & 0b00000001);
                fseek(file, -2, SEEK_CUR);
                fputc(sample[0], file);
                fseek(file, 1, SEEK_CUR);
            }
        }

    }else {
        FATAL("cannot encode message in this file, message file exceeds file capacity");
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
    Byte dataChunkSize[4];
    if (fread(dataChunkSize, sizeof(Byte), 4, file) != 4) {
        FATAL("can not read the data chunk's size");
        return NULL;
    }

    unsigned int messageSize = 0;
    Byte mask = 0b00000001;
    for (unsigned int i = 0; i < WAV_STEG_HEADER_SIZE; i++) {
        Byte sample[2];
        if (fread(sample, sizeof(Byte), 2, file) != 2) {
            FATAL("failed to read a sample from file");
            return NULL;
        }
        messageSize |= ((sample[0] & 0b00000001) << (7 - i));           // TODO apparently i store in big endian
        mask >>= 1;
    }
    DEBUG("size of the message is : %u", messageSize);

    Byte *message = malloc(sizeof(Byte) * messageSize + 1);
    memset(message, 0, messageSize + 1); // prevent from reading out of scope of a string (in other words, add null terminator)
    for (unsigned int i = 0; i < messageSize; i++) {
        Byte c = 0;
        for (int b = 0; b < 8; b++) {
            Byte sample[2];
            if (fread(sample, sizeof(Byte), 2, file) != 2) {
                FATAL("failed to read a sample");
                return NULL;
            }
            c |= ((sample[0] & 0b00000001) << (7 - b));
        }
        message[i] = c;
        c = 0;
    }
    DEBUG("secret message is : %s", message);
    return (char *)message;
}

bool readStandardHeaderInfo(FILE *file) {
    Byte headerSignature[4];
    if (fread(headerSignature, sizeof(Byte), 4, file) != 4) {
        FATAL("Failed to read header signature");
        return false;
    }
    DEBUG("the header signature is : %.4s\n", headerSignature);
    if (signatureIsNotValid(headerSignature)) {
        return false;
    }

    Byte fileSize[4];
    if (fread(fileSize, sizeof(Byte), 4, file) != 4) {
        FATAL("Failed to read file size in header");
        return false;
    }
    unsigned int decimalFileSize = hexaToDecimal(fileSize, 4);
    DEBUG("INFO: file size is : %d kb\n", decimalFileSize + 8);

    Byte format[4];
    if (fread(format, sizeof(Byte), 4, file) != 4) {
        FATAL("Failed to read format in header");
        return false;
    }
    DEBUG("INFO: format type is : %.4s\n", format);
    return true;
}

bool readChunksUntilData(FILE **file) {
    while (1) {
        Byte chunkID[5] = {0};
        if (fread(chunkID, sizeof(Byte), 4, *file) != 4) {
            FATAL("Failed to read chunk id in header");
            return false;
        }
        DEBUG("INFO: chunk id : %.4s\n", chunkID);

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
    Byte chunkSize[4];
    if (fread(chunkSize, sizeof(Byte), 4, file) != 4) {
        FATAL("failed to read the fmt chunk size in header");
        return false;
    }
    unsigned int decimalChunkSize = hexaToDecimal(chunkSize, 4);
    Byte audioFormat[2];
    if (fread(audioFormat, sizeof(Byte), 2, file) != 2) {
        FATAL("Failed to read audio format in header");
        return false;
    }
    DEBUG("audio format is : %u (1 for PCM)\n", hexaToDecimal(audioFormat, 2));
    
    Byte numChannels[2];
    if (fread(numChannels, sizeof(Byte), 2, file) != 2) {
        FATAL("Failed to read number of channels in header");
        return false;
    }
    DEBUG("number of channels : %u, mono or stereo\n", hexaToDecimal(numChannels, 2));

    Byte sampleRate[4];
    if (fread(sampleRate, sizeof(Byte), 4, file) != 4) {
        FATAL("Failed to read sample rate in header");
        return false;
    }
    DEBUG("sample rate : %uz, %ukz \n", hexaToDecimal(sampleRate, 4), hexaToDecimal(sampleRate, 4) / 1000);

    fseek(file, 6, SEEK_CUR); // skipping byte rate + block align
    Byte bitsPerSample[2];
    if (fread(bitsPerSample, sizeof(Byte), 2, file) != 2) {
        FATAL("Failed to read the bits per sample in header of FMT");
        return false;
    }
    unsigned int decimalBitsPerSample = hexaToDecimal(bitsPerSample, 2);
    DEBUG("bits per sample is : %d", decimalBitsPerSample);
    if (decimalChunkSize > 16) {  // weird but whatever
        fseek(file, decimalChunkSize - 16, SEEK_CUR);
    }
    return true;
}

bool skipChunk(FILE *file) {
    Byte sizeToSkip[4];
    if (fread(sizeToSkip, sizeof(Byte), 4, file) != 4) {
        FATAL("failed to read the size of the chunk to skip based on that");
        return false;
    }
    unsigned int skippingSize = hexaToDecimal(sizeToSkip, 4);
    fseek(file, skippingSize, SEEK_CUR);
    return true;
}

bool readLISTChunk(FILE *file) {
    Byte chunkSize[4];
    if (fread(chunkSize, sizeof(Byte), 4, file) != 4) {
        FATAL("failed to read the chunk size in LIST");
        return false;
    }
    unsigned int decimalChunkSize = hexaToDecimal(chunkSize, 4);

    fseek(file, 4, SEEK_CUR); // skipping "INFO" - List type (metadata)
    Byte softwareId[5] = {0};
    if (fread(softwareId, sizeof(Byte), 4, file) != 4) {
        FATAL("Failed to read software identifier in header");
        return false;
    }
    DEBUG("software identifier : (created by software id): %.4s \n", softwareId);

    if (strcmp("ISFT", (const char *)softwareId) == 0) {
        DEBUG("created by software");
        Byte softwareNameLen[4];
        if (fread(softwareNameLen, sizeof(Byte), 4, file) != 4) {
            FATAL("failed to read software name length in header");
            return false;
        }
        unsigned int decimalSoftwareNameLen = hexaToDecimal(softwareNameLen, 4);

        Byte softwareName[decimalSoftwareNameLen];
        if (fread(softwareName, sizeof(Byte), decimalSoftwareNameLen, file) != decimalSoftwareNameLen) {
            FATAL("Failed to read software name in header");
            return false;
        }
        DEBUG("software name or library used to create the audio: %s\n", softwareName);
        return true;        // end of the LIST chunk
    }// else we just jump to end this chunk as well
    fseek(file, decimalChunkSize - 8, SEEK_CUR);      // where 8 come from? after skipping of INFO
                                                                           // and read the ISFT (software identifier) I
                                                                           // check it with "ISFT" then i read length of
                                                                           // software name and read the software name after
                                                                           // and reach the end of the chunk, but if the condition
                                                                           // is not true, i should skipping the is equal to
                                                                           // decimal - (8 = 4(INFO) + 4(ISFT))
    
    return true;
}

bool signatureIsNotValid(Byte *headerSignature) {
    unsigned int signature = hexaToDecimal(headerSignature, 4);
    if (signature == WAV_HEADER_SIGNATURE) {
        DEBUG("the wav header signature is verified");
        return false;
    }
    FATAL("the wav header signature is not verified");
    return true;
}
