#include "../include/wav.h"
#include <stdio.h>
#include <string.h>

#define CHUNK_ID_FMT  "fmt "
#define CHUNK_ID_LIST "LIST"
#define CHUNK_ID_DATA "data"

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
    printf("\n");
    if (!readChunksUntilData(&file)) {
        return false;
    }
    // data chunk id passed, now at the data chunk size
    Byte dataChunkSize[4];
    if (fread(dataChunkSize, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: can not read the data chunk's size\n");
        return false;
    }
    unsigned int decimalDataChunk = hexaToDecimal(dataChunkSize, 4);
    printf("INFO: data chunk size: %d\n", decimalDataChunk);
    unsigned int messageSizeBits = sizeof(unsigned int) * 2;            //TODO its just a standard that how to save secret message size
    if (ableToEncode(decimalDataChunk - messageSizeBits, strlen(secretMessage) * 8)) {
        // embed the header of the steganography, which is the size of unsigned int, containing message size
        unsigned char secretMessageSize = strlen(secretMessage);
        for (int i = 0; i < messageSizeBits; i++) {
            Byte sample[2];                                     //TODO here we ASSUME(which is bad idea to assume anything
                                                                //in this project) that each sample is 16bit (bits per sample)
                                                                //so i should fix it
            if (fread(sample, sizeof(Byte), 2, file) != 2) {
                printf("ERROR: failed to read a sample (2byte) of a file\n");
                return false;
            }
            sample[0] = (sample[0] & 0b11111110) | (0b00000001 & (secretMessageSize >> (messageSizeBits - 1 - i)));
            fseek(file, -2, SEEK_CUR);
            fputc(sample[0], file);
            fseek(file, 1, SEEK_CUR);
        }

    }else {
        printf("ERROR: can not embed message in this file\n");
        return false;
    }

    return true;
}

bool readStandardHeaderInfo(FILE *file) {
    Byte headerSignature[4];
    if (fread(headerSignature, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read header signature\n");
        return false;
    }
    printf("INfO: the header signature is : %.4s\n", headerSignature);
    if (signatureIsNotValid(headerSignature)) {
        return false;
    }

    Byte fileSize[4];
    if (fread(fileSize, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read file size in header\n");
        return false;
    }
    unsigned int decimalFileSize = hexaToDecimal(fileSize, 4);
    printf("INFO: file size is : %d kb\n", decimalFileSize + 8);

    Byte format[4];
    if (fread(format, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read format in header\n");
        return false;
    }
    printf("INFO: format type is : %.4s\n", format);
    return true;
}

bool readChunksUntilData(FILE **file) {
    while (1) {
        Byte chunkID[5] = {0};
        if (fread(chunkID, sizeof(Byte), 4, *file) != 4) {
            printf("ERROR: Failed to read chunk id in header\n");
            return false;
        }
        printf("INFO: chunk id : %.4s\n", chunkID);

        if (strcmp(CHUNK_ID_FMT, (const char *)chunkID) == 0) {
            if (!readFMTChunk(*file)) {
                return false;
            }
            printf("\n");
            continue;
        }
        if (strcmp(CHUNK_ID_LIST, (const char *)chunkID) == 0) {
            if (!readLISTChunk(*file)) {
                return false;
            }
            printf("\n");
            continue;
        }

        if (strcmp(CHUNK_ID_DATA, (const char *)chunkID) == 0) {
            return true;
        }
        skipChunk(*file);
    }
    return true;
}

bool readFMTChunk(FILE *file) {
    Byte chunkSize[4];
    if (fread(chunkSize, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: failed to read the fmt chunk size in header\n");
        return false;
    }
    unsigned int decimalChunkSize = hexaToDecimal(chunkSize, 4);
    Byte audioFormat[2];
    if (fread(audioFormat, sizeof(Byte), 2, file) != 2) {
        printf("ERROR: Failed to read audio format in header\n");
        return false;
    }
    unsigned int decimalAudioFormat = hexaToDecimal(audioFormat, 2);
    printf("INFO: audio format is : %u (1 for PCM)\n", decimalAudioFormat);

    
    Byte numChannels[2];
    if (fread(numChannels, sizeof(Byte), 2, file) != 2) {
        printf("ERROR: Failed to read number of channels in header\n");
        return false;
    }
    unsigned int decimalNumChannels = hexaToDecimal(numChannels, 2);
    printf("INFO: number of channels : %u, mono or stereo\n", decimalNumChannels);

    Byte sampleRate[4];
    if (fread(sampleRate, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read sample rate in header\n");
        return false;
    }
    unsigned int decimalSampleRate = hexaToDecimal(sampleRate, 4);
    printf("INFO: sample rate : %uz, %ukz \n", decimalSampleRate, decimalSampleRate / 1000);

    fseek(file, 6, SEEK_CUR); // skipping byte rate + block align
    Byte bitsPerSample[2];
    if (fread(bitsPerSample, sizeof(Byte), 2, file) != 2) {
        printf("ERROR: Failed to read the bits per sample in header of FMT\n");
        return false;
    }
    if (decimalChunkSize > 16) {  // weird but whatever
        fseek(file, decimalChunkSize - 16, SEEK_CUR);
    }
    return true;
}

bool skipChunk(FILE *file) {
    Byte sizeToSkip[4];
    if (fread(sizeToSkip, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: failed to read the size of the chunk to skip based on that\n");
        return false;
    }
    unsigned int skippingSize = hexaToDecimal(sizeToSkip, 4);
    fseek(file, skippingSize, SEEK_CUR);
    return true;
}

bool readLISTChunk(FILE *file) {
    Byte chunkSize[4];
    if (fread(chunkSize, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: failed to read the chunk size in LIST\n");
        return false;
    }
    unsigned int decimalChunkSize = hexaToDecimal(chunkSize, 4);

    fseek(file, 4, SEEK_CUR); // skipping "INFO" - List type (metadata)
    Byte softwareId[5] = {0};
    if (fread(softwareId, sizeof(Byte), 4, file) != 4) {
        printf("ERROR: Failed to read software identifier in header\n");
        return false;
    }
    printf("INFO: software identifier : (created by software id): %.4s \n", softwareId);

    if (strcmp("ISFT", (const char *)softwareId) == 0) {
        printf("INFO: created by software\n");
        Byte softwareNameLen[4];
        if (fread(softwareNameLen, sizeof(Byte), 4, file) != 4) {
            printf("ERROR: Failed to read software name length in header\n");
            return false;
        }
        unsigned int decimalSoftwareNameLen = hexaToDecimal(softwareNameLen, 4);

        Byte softwareName[decimalSoftwareNameLen];
        if (fread(softwareName, sizeof(Byte), decimalSoftwareNameLen, file) != decimalSoftwareNameLen) {
            printf("ERROR: Failed to read software name in header\n");
            return false;
        }
        printf("INFO: software name or library used to create the audio: %s\n", softwareName);
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

char *decodeWAV(FILE *file) {
    return NULL;
}

bool signatureIsNotValid(Byte *headerSignature) {
    unsigned int signature = hexaToDecimal(headerSignature, 4);
    if (signature == WAV_HEADER_SIGNATURE) {
        printf("INFO: the wav header signature is verified\n");
        return false;
    }
    printf("ERROR: the wav header signature is not verified\n");
    return true;
}
