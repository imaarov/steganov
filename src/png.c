#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
    PNGIHDRHeader ihdrHeader;
    if (fread(&ihdrHeader, sizeof(PNGIHDRHeader) - sizeof(ihdrHeader.gap_pad),
              1, file) != 1) {
        FATAL("failed to read the IHDR header");
        return false;
    }
    uint32_t ihdrSize =
        hexaToDecimalBigEndian(ihdrHeader.size, PNG_LENGTH_FIELD_SIZE);
    uint32_t width =
        hexaToDecimalBigEndian(ihdrHeader.width, PNG_IHDR_WIDTH_FIELD_SIZE);
    uint32_t height =
        hexaToDecimalBigEndian(ihdrHeader.height, PNG_IHDR_HEIGHT_FIELD_SIZE);
    DEBUG("IHDR chunk size: %d", ihdrSize);
    DEBUG("IHDR ID: %.4s", (char *)ihdrHeader.ID);
    DEBUG("IHDR width: %d", width);
    DEBUG("IHDR height: %d", height);
    DEBUG("IHDR color type : %d", ihdrHeader.colorType);
    DEBUG("IHDR compression type: %d", ihdrHeader.compressionType);
    DEBUG("IHDR filter type: %d", ihdrHeader.filterType);
    DEBUG("IHDR interlace method: %d", ihdrHeader.interlaceMethod);
    // CRC include both chuck data section + chunk type section
    uint32_t crcDataSize = ihdrSize + PNG_CHUNK_ID_SIZE;
    Byte *data = (Byte *)&ihdrHeader;
    data = data +
           PNG_LENGTH_FIELD_SIZE; // crc exclude the size field, so i skip that
    uint32_t crc = crc32_calculate(data, crcDataSize);
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
