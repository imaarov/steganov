#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool png_header_is_not_valid(FILE *file);
static bool read_ihdr_chunk(FILE *file);
static uint32_t get_first_idat_chunk_size(FILE *file);

bool encode_png(FILE *file, char *secretMessage) {
    (void)secretMessage; // TODO silence unused warning
    if (png_header_is_not_valid(file)) {
        return false;
    }
    if (!read_ihdr_chunk(file)) {
        return false;
    }
    uint32_t idat_size = get_first_idat_chunk_size(file);
    if (0 == idat_size) {
        return false;
    }

    return false;
}

char *decode_png(FILE *file) {
    (void)file;
    return NULL;
}

bool png_header_is_not_valid(FILE *file) {
    DEBUG("Header information: ");
    byte headerA[PNG_HEADER_SIZE / 2];
    if (fread(headerA, sizeof(byte), PNG_HEADER_SIZE / 2, file) != PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
        return true;
    }
    byte headerB[PNG_HEADER_SIZE / 2];
    if (fread(headerB, sizeof(byte), PNG_HEADER_SIZE / 2, file) != PNG_HEADER_SIZE / 2) {
        FATAL("failed to read the PNG header section");
        return true;
    }
    unsigned int signA = hexa_to_decimal(LITTLE_ENDIAN, headerA, PNG_HEADER_SIZE / 2);
    unsigned int signB = hexa_to_decimal(LITTLE_ENDIAN, headerB, PNG_HEADER_SIZE / 2);
    DEBUG("signature first 4 byte : %x", signA);
    DEBUG("signature second 4 byte : %x", signB);
    if (PNG_HEADER_SIGNATURE_A == signA && PNG_HEADER_SIGNATURE_B == signB) {
        DEBUG("png file signature is standard");
        return false;
    }
    FATAL("png file signature is NOT standard");
    return true;
}

bool read_ihdr_chunk(FILE *file) {
    PNGIHDRHeader ihdr_header;
    if (fread(&ihdr_header, sizeof(PNGIHDRHeader) - sizeof(ihdr_header.gap_pad), 1, file) != 1) {
        FATAL("failed to read the IHDR header");
        return false;
    }
    uint32_t ihdr_size = hexa_to_decimal(BIG_ENDIAN, ihdr_header.size, PNG_LENGTH_FIELD_SIZE);
    uint32_t width     = hexa_to_decimal(BIG_ENDIAN, ihdr_header.width, PNG_IHDR_WIDTH_FIELD_SIZE);
    uint32_t height = hexa_to_decimal(BIG_ENDIAN, ihdr_header.height, PNG_IHDR_HEIGHT_FIELD_SIZE);
    DEBUG("IHDR chunk size: %d", ihdr_size);
    DEBUG("IHDR ID: %.4s", (char *)ihdr_header.ID);
    DEBUG("IHDR width: %d", width);
    DEBUG("IHDR height: %d", height);
    DEBUG("IHDR color type : %d", ihdr_header.colorType);
    DEBUG("IHDR compression type: %d", ihdr_header.compressionType);
    DEBUG("IHDR filter type: %d", ihdr_header.filterType);
    DEBUG("IHDR interlace method: %d", ihdr_header.interlaceMethod);
    // CRC include both chuck data section + chunk type section
    uint32_t crc_data_size = ihdr_size + PNG_CHUNK_ID_SIZE;
    byte *data             = (byte *)&ihdr_header;
    data         = data + PNG_LENGTH_FIELD_SIZE; // crc exclude the size field, so i skip that
    uint32_t crc = crc32_calculate(data, crc_data_size);
    DEBUG("the CRC of the IHDR (Calculated by built in CRC Checksum "
          "calculator)is : 0x%04x",
          crc);
    byte ihdr_crc[PNG_CRC_FIELD_SIZE];
    if (fread(ihdr_crc, sizeof(byte), PNG_CRC_FIELD_SIZE, file) != PNG_CRC_FIELD_SIZE) {
        FATAL("failed to read the CRC Checksum of IHDR chunk");
        return false;
    }
    uint32_t decimal_ihdr_crc = hexa_to_decimal(BIG_ENDIAN, ihdr_crc, PNG_CRC_FIELD_SIZE);
    DEBUG("the actual file CRC Checksum of IHDR : 0x%04x", decimal_ihdr_crc);
    if (decimal_ihdr_crc != crc) {
        FATAL("Calculated CRC checksum is not the same as the actual CRC in "
              "the png");
        return false;
    }

    return true;
}

uint32_t get_first_idat_chunk_size(FILE *file) {
    printf("\n");
    while (1) {
        byte chunk_size[PNG_LENGTH_FIELD_SIZE];
        if (fread(chunk_size, sizeof(byte), PNG_LENGTH_FIELD_SIZE, file) != PNG_LENGTH_FIELD_SIZE) {
            FATAL("can not read the chunk size");
            return 0;
        }
        byte chunk_id[PNG_CHUNK_ID_SIZE];
        if (fread(chunk_id, sizeof(byte), PNG_LENGTH_FIELD_SIZE, file) != PNG_LENGTH_FIELD_SIZE) {
            FATAL("can not read the chunk id");
            return 0;
        }
        uint32_t size = hexa_to_decimal(BIG_ENDIAN, chunk_size, PNG_LENGTH_FIELD_SIZE);
        DEBUG("chunk id found : %.4s, skipping : %u bytes", chunk_id, size);
        if (0 == strcmp(PNG_IDAT_CHUNK_ID, (char *)chunk_id)) {
            fseek(file, PNG_CHUNK_ID_SIZE + PNG_LENGTH_FIELD_SIZE, SEEK_CUR);
            DEBUG("found the IDAT chunk, exit the finding process");
            return size;
        }
        if (0 == strcmp(PNG_IEND_CHUNK_ID, (char *)chunk_id)) {
            DEBUG("reached the final IEND chunk, IDAT nout found! possible "
                  "corrupted file");
            return 0;
        }
        fseek(file, size + PNG_CRC_FIELD_SIZE, SEEK_CUR);
    }
    return 0;
}
