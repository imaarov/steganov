#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool verifyPNGHeaderSignature(Byte *, Byte *);
bool readIHDRChunk(FILE *file);

bool encodePNG(FILE *file, char *secretMessage) {
    Byte headerA[PNG_HEADER_SIZE / 2];
    if (fread(headerA, sizeof(Byte), PNG_HEADER_SIZE / 2, file) !=
        PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
    }
    Byte headerB[PNG_HEADER_SIZE / 2];
    if (fread(headerB, sizeof(Byte), PNG_HEADER_SIZE / 2, file) !=
        PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
    }
    if (!verifyPNGHeaderSignature(headerA, headerB)) {
        return false;
    }
    readIHDRChunk(file);
    return false;
}

bool verifyPNGHeaderSignature(Byte *signatureA, Byte *signatureB) {
    unsigned int signA = hexaToDecimal(signatureA, PNG_HEADER_SIZE / 2);
    unsigned int signB = hexaToDecimal(signatureB, PNG_HEADER_SIZE / 2);
    DEBUG("sign A : %x", signA);
    DEBUG("sign B : %x", signB);
    if (PNG_HEADER_SIGNATURE_A == signA && PNG_HEADER_SIGNATURE_B == signB) {
        DEBUG("png file signature is standard");
        return true;
    }
    FATAL("png file signature is NOT standard");
    return false;
}

bool readIHDRChunk(FILE *file) {
    Byte size[PNG_IHDR_LENGTH_FIELD_BYTES];
    if (fread(size, sizeof(Byte), 4, file) != PNG_IHDR_LENGTH_FIELD_BYTES) {
        FATAL("failed to read the IHDR length field");
        return false;
    }
    unsigned int lengthField =
        hexaToDecimalBigEndian(size, PNG_IHDR_LENGTH_FIELD_BYTES);
    DEBUG("length of the field of IHDR is %d", lengthField);
    Byte chunkIdentifier[PNG_IHDR_CHUNK_ID_SIZE + 1];
    memset(chunkIdentifier, 0, PNG_IHDR_CHUNK_ID_SIZE + 1);
    if (fread(chunkIdentifier, sizeof(Byte), PNG_IHDR_CHUNK_ID_SIZE, file) !=
        PNG_IHDR_CHUNK_ID_SIZE) {
        FATAL("failed to read the IHDR identifier");
    }
    DEBUG("IHDR ID : %s", chunkIdentifier);
    if (strcmp(PNG_IHDR_CHUNK_ID, (char *)chunkIdentifier) != 0) {
        FATAL("IHDR identifier is not the same as standard IHDR ID");
        return false;
    }

    Byte width[4];
    if (fread(width, sizeof(Byte), 4, file) != 4) {
        FATAL("failed to read width section of IHDR");
        return false;
    }
    unsigned int decimalWidth = hexaToDecimalBigEndian(width, 4);
    DEBUG("width of the png : %d", decimalWidth);

    Byte height[4];
    if (fread(height, sizeof(Byte), 4, file) != 4) {
        FATAL("failed to read height section of IHDR");
        return false;
    }
    unsigned int decimalHeight = hexaToDecimalBigEndian(height, 4);
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
    uint32_t crc = 0;
    crc = crc32_calculate(data, lengthField + 4);
    DEBUG("the CRC of the IHDR is : 0x%04x", crc);
    return false;
}
