#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include "../include/zlib_wrapper.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool png_header_is_not_valid(FILE *file);
static bool read_ihdr_chunk(FILE *file);
static byte *read_all_idat_data(FILE *file, uint32_t *out_size);
static bool lsb_embed(byte *data, uint32_t data_size, const char *message);
static char *lsb_extract(const byte *data, uint32_t data_size);

bool encode_png(FILE *file, char *secretMessage) {
    (void)secretMessage; // TODO
    if (png_header_is_not_valid(file))
        return false;
    if (!read_ihdr_chunk(file))
        return false;

    uint32_t idat_size = 0;
    byte *idat_data    = read_all_idat_data(file, &idat_size);
    if (NULL == idat_data || idat_size == 0) {
        FATAL("no IDAT data found or failed to read it");
        return false;
    }

    uint32_t decoded_size = 0;
    byte *decoded         = zlib_decode(idat_data, idat_size, &decoded_size);
    free(idat_data);

    if (NULL == decoded) {
        FATAL("failed to zlib-decode IDAT data");
        return false;
    }

    // TODO `decoded` is now the raw filtered scanline data (still needs PNG
    // TODO filter type unfiltering per row

    const char *secret = "hi";
    assert(lsb_embed(decoded, decoded_size, secret));

    char *extracted = lsb_extract(decoded, decoded_size);
    printf("extracted: %s\n", extracted ? extracted : "(NULL)");
    assert(extracted && strcmp(extracted, secret) == 0);

    free(extracted);
    free(decoded);
    return false;
}

char *decode_png(FILE *file) {
    (void)file;
    return NULL;
}

static bool png_header_is_not_valid(FILE *file) {
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

static bool read_ihdr_chunk(FILE *file) {
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

static byte *read_all_idat_data(FILE *file, uint32_t *out_size) {
    byte *idat_data    = NULL;
    uint32_t idat_size = 0;

    while (1) {
        byte chunk_size_bytes[PNG_LENGTH_FIELD_SIZE];
        if (fread(chunk_size_bytes, sizeof(byte), PNG_LENGTH_FIELD_SIZE, file) !=
            PNG_LENGTH_FIELD_SIZE) {
            FATAL("can not read the chunk size");
            free(idat_data);
            return NULL;
        }
        byte chunk_id[PNG_CHUNK_ID_SIZE];
        if (fread(chunk_id, sizeof(byte), PNG_CHUNK_ID_SIZE, file) != PNG_CHUNK_ID_SIZE) {
            FATAL("can not read the chunk id");
            free(idat_data);
            return NULL;
        }

        uint32_t chunk_size = hexa_to_decimal(BIG_ENDIAN, chunk_size_bytes, PNG_LENGTH_FIELD_SIZE);

        if (0 == strncmp(PNG_IDAT_CHUNK_ID, (char *)chunk_id, PNG_CHUNK_ID_SIZE)) {
            byte *grown = realloc(idat_data, idat_size + chunk_size);
            if (NULL == grown) {
                FATAL("failed to grow idat buffer");
                free(idat_data);
                return NULL;
            }
            idat_data = grown;

            if (fread(idat_data + idat_size, sizeof(byte), chunk_size, file) != chunk_size) {
                FATAL("failed to read IDAT chunk payload");
                free(idat_data);
                return NULL;
            }
            idat_size += chunk_size;

            fseek(file, PNG_CRC_FIELD_SIZE, SEEK_CUR);
            continue;
        }

        if (0 == strncmp(PNG_IEND_CHUNK_ID, (char *)chunk_id, PNG_CHUNK_ID_SIZE)) {
            break;
        }

        fseek(file, chunk_size + PNG_CRC_FIELD_SIZE, SEEK_CUR);
    }

    *out_size = idat_size;
    return idat_data;
}

static bool lsb_embed(byte *data, uint32_t data_size, const char *message) {
    uint32_t message_len = (uint32_t)strlen(message) + 1;
    uint32_t bits_needed = message_len * 8;

    if (bits_needed > data_size) {
        FATAL("lsb_embed: not enough bytes to embed the message");
        return false;
    }

    uint32_t bit_index = 0;
    for (uint32_t i = 0; i < message_len; i++) {
        byte c = (byte)message[i];
        for (int b = 0; b < 8; b++) {
            byte bit        = (c >> b) & 1;
            data[bit_index] = (byte)((data[bit_index] & 0xFE) | bit);
            bit_index++;
        }
    }
    return true;
}

static char *lsb_extract(const byte *data, uint32_t data_size) {
    uint32_t max_chars = data_size / 8;
    char *message      = malloc(max_chars + 1);
    if (NULL == message) {
        FATAL("lsb_extract: allocation failed");
        return NULL;
    }

    uint32_t bit_index = 0;
    for (uint32_t i = 0; i < max_chars; i++) {
        byte c = 0;
        for (int b = 0; b < 8; b++) {
            byte bit = data[bit_index] & 1;
            c |= (byte)(bit << b);
            bit_index++;
        }
        message[i] = (char)c;
        if (c == '\0') {
            return message;
        }
    }

    FATAL("lsb_extract: no null terminator found within available data");
    free(message);
    return NULL;
}
