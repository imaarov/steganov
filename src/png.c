#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool verifyPNGHeaderSignature(FILE *file);
bool readIHDRChunk(FILE *file);
uint32_t getFirstIDATChunkSize(FILE *file);

bool encodePNG(FILE *file, char *secretMessage) {
    if (!verifyPNGHeaderSignature(file)) {
        return false;
    }
    if (!readIHDRChunk(file)) {
        return false;
    }
    uint32_t idatSize = getFirstIDATChunkSize(file);
    if (0 == idatSize) {
        return false;
    }

    return false;
}

bool verifyPNGHeaderSignature(FILE *file) {
    DEBUG("Header information: ");
    Byte headerA[PNG_HEADER_SIZE / 2];
    if (fread(headerA, sizeof(Byte), PNG_HEADER_SIZE / 2, file) !=
        PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
        return false;
    }
    Byte headerB[PNG_HEADER_SIZE / 2];
    if (fread(headerB, sizeof(Byte), PNG_HEADER_SIZE / 2, file) !=
        PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
        return false;
    }
    unsigned int signA = hexaToDecimal(headerA, PNG_HEADER_SIZE / 2);
    unsigned int signB = hexaToDecimal(headerB, PNG_HEADER_SIZE / 2);
    DEBUG("signature first 4 byte : %x", signA);
    DEBUG("signature second 4 byte : %x", signB);
    if (PNG_HEADER_SIGNATURE_A == signA && PNG_HEADER_SIGNATURE_B == signB) {
        DEBUG("png file signature is standard");
        return true;
    }
    FATAL("png file signature is NOT standard");
    return false;
}

bool readIHDRChunk(FILE *file) {
    printf("\n");
    DEBUG("IHDR chunk information");
    Byte size[PNG_LENGTH_FIELD_SIZE];
    if (fread(size, sizeof(Byte), PNG_LENGTH_FIELD_SIZE, file) !=
        PNG_LENGTH_FIELD_SIZE) {
        FATAL("failed to read the IHDR length field");
        return false;
    }
    unsigned int lengthField =
        hexaToDecimalBigEndian(size, PNG_LENGTH_FIELD_SIZE);
    DEBUG("length of the field of IHDR is %d", lengthField);

    Byte chunkId[PNG_CHUNK_ID_SIZE + 1];
    memset(chunkId, 0, PNG_CHUNK_ID_SIZE + 1);
    if (fread(chunkId, sizeof(Byte), PNG_CHUNK_ID_SIZE, file) !=
        PNG_CHUNK_ID_SIZE) {
        FATAL("failed to read the IHDR identifier");
    }
    DEBUG("IHDR ID : %s", chunkId);
    if (strcmp(PNG_IHDR_CHUNK_ID, (char *)chunkId) != 0) {
        FATAL("IHDR identifier is not the same as standard IHDR ID");
        return false;
    }

    Byte width[PNG_IHDR_WIDTH_FIELD_SIZE];
    if (fread(width, sizeof(Byte), PNG_IHDR_WIDTH_FIELD_SIZE, file) !=
        PNG_IHDR_WIDTH_FIELD_SIZE) {
        FATAL("failed to read width section of IHDR");
        return false;
    }
    unsigned int decimalWidth =
        hexaToDecimalBigEndian(width, PNG_IHDR_WIDTH_FIELD_SIZE);
    DEBUG("width of the png : %d", decimalWidth);

    Byte height[PNG_IHDR_HEIGHT_FIELD_SIZE];
    if (fread(height, sizeof(Byte), PNG_IHDR_HEIGHT_FIELD_SIZE, file) !=
        PNG_IHDR_HEIGHT_FIELD_SIZE) {
        FATAL("failed to read height section of IHDR");
        return false;
    }
    unsigned int decimalHeight =
        hexaToDecimalBigEndian(height, PNG_IHDR_HEIGHT_FIELD_SIZE);
    DEBUG("height of the png : %d", decimalHeight);

    Byte bitDepth = 0;
    bitDepth = fgetc(file);
    DEBUG("bit depth (bits per sample) is : %d", bitDepth);

    Byte colorType = 0;
    colorType = fgetc(file);
    DEBUG("color types: \n0 : grey scale\n2 : RGB\n3 : indexed color "
          "palette\n4 : grey scale with alpha\n6 : RGBAlpha");
    DEBUG("color type :%d", colorType);

    Byte compressionType = 0;
    compressionType = fgetc(file);
    DEBUG("compression type is %d", compressionType);

    Byte filterType = 0;
    filterType = fgetc(file);
    DEBUG("filter type is %d", filterType);

    Byte interlaceMethod = 0;
    interlaceMethod = fgetc(file);
    DEBUG("filter type is %d", interlaceMethod);

    // TODO make it dynamic and better
    fseek(file, -17, SEEK_CUR);
    Byte *data = malloc(sizeof(Byte) * (lengthField + 4));
    if (fread(data, sizeof(Byte), lengthField + 4, file) != (lengthField + 4)) {
        FATAL("failed to read past data");
        return false;
    }
    uint32_t crc = crc32_calculate(data, lengthField + 4);
    DEBUG("the CRC of the IHDR (Calculated by built in CRC Checksum "
          "calculator)is : 0x%04x",
          crc);
    Byte IHDRCrc[PNG_CRC_FIELD_SIZE];
    if (fread(IHDRCrc, sizeof(Byte), PNG_CRC_FIELD_SIZE, file) !=
        PNG_CRC_FIELD_SIZE) {
        FATAL("failed to read the CRC Checksum of IHDR chunk");
        return false;
    }
    uint32_t decimalIHDRCRC =
        hexaToDecimalBigEndian(IHDRCrc, PNG_CRC_FIELD_SIZE);
    DEBUG("the actual file CRC Checksum of IHDR : 0x%04x", decimalIHDRCRC);
    if (decimalIHDRCRC != crc) {
        FATAL("Calculated CRC checksum is not the same as the actual CRC in "
              "the png");
        return false;
    }

    free(data);
    return true;
}

uint32_t getFirstIDATChunkSize(FILE *file) {
    printf("\n");
    while (1) {
        Byte chunkSize[PNG_LENGTH_FIELD_SIZE];
        if (fread(chunkSize, sizeof(Byte), PNG_LENGTH_FIELD_SIZE, file) !=
            PNG_LENGTH_FIELD_SIZE) {
            FATAL("can not read the chunk size");
            return 0;
        }
        Byte chunkId[PNG_CHUNK_ID_SIZE];
        if (fread(chunkId, sizeof(Byte), PNG_LENGTH_FIELD_SIZE, file) !=
            PNG_LENGTH_FIELD_SIZE) {
            FATAL("can not read the chunk id");
            return 0;
        }
        uint32_t size =
            hexaToDecimalBigEndian(chunkSize, PNG_LENGTH_FIELD_SIZE);
        DEBUG("chunk id found : %.4s, skipping : %u bytes", chunkId, size);
        if (0 == strcmp(PNG_IDAT_CHUNK_ID, (char *)chunkId)) {
            fseek(file, PNG_CHUNK_ID_SIZE + PNG_LENGTH_FIELD_SIZE, SEEK_CUR);
            DEBUG("found the IDAT chunk, exit the finding process");
            return size;
        }
        if (0 == strcmp(PNG_IEND_CHUNK_ID, (char *)chunkId)) {
            DEBUG("reached the final IEND chunk, IDAT nout found! possible "
                  "corrupted file");
            return 0;
        }
        fseek(file, size + PNG_CRC_FIELD_SIZE, SEEK_CUR);
    }
    return 0;
}
