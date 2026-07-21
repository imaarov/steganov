#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/utils.h"
#include "../include/zlib_wrapper.h"
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
static void write_be32(byte *out, uint32_t value);
static bool write_chunk(FILE *out_file, const char *type, const byte *data, uint32_t data_len);
static bool write_new_idat_chunk(FILE *out_file, byte *idat_data, uint32_t idat_size,
                                 const char *secretMessage);

bool encode_png(FILE *file, FILE *out_file, char *secretMessage) {
    if (NULL == file || NULL == out_file || NULL == secretMessage) {
        FATAL("encode_png: invalid arguments");
        return false;
    }

    rewind(file);

    byte signature[PNG_HEADER_SIZE];
    if (fread(signature, sizeof(byte), PNG_HEADER_SIZE, file) != PNG_HEADER_SIZE) {
        FATAL("failed to read PNG signature");
        return false;
    }
    unsigned int signA = hexa_to_decimal(LITTLE_ENDIAN, signature, PNG_HEADER_SIZE / 2);
    unsigned int signB =
        hexa_to_decimal(LITTLE_ENDIAN, signature + PNG_HEADER_SIZE / 2, PNG_HEADER_SIZE / 2);
    if (!(PNG_HEADER_SIGNATURE_A == signA && PNG_HEADER_SIGNATURE_B == signB)) {
        FATAL("png file signature is NOT standard");
        return false;
    }
    if (fwrite(signature, sizeof(byte), PNG_HEADER_SIZE, out_file) != PNG_HEADER_SIZE) {
        FATAL("failed to write PNG signature to output");
        return false;
    }

    byte *idat_data    = NULL;
    uint32_t idat_size = 0;
    bool idat_written  = false;

    while (1) {
        byte length_bytes[PNG_LENGTH_FIELD_SIZE];
        if (fread(length_bytes, sizeof(byte), PNG_LENGTH_FIELD_SIZE, file) !=
            PNG_LENGTH_FIELD_SIZE) {
            FATAL("failed to read chunk length");
            free(idat_data);
            return false;
        }
        byte type_bytes[PNG_CHUNK_ID_SIZE];
        if (fread(type_bytes, sizeof(byte), PNG_CHUNK_ID_SIZE, file) != PNG_CHUNK_ID_SIZE) {
            FATAL("failed to read chunk type");
            free(idat_data);
            return false;
        }
        uint32_t chunk_len = hexa_to_decimal(BIG_ENDIAN, length_bytes, PNG_LENGTH_FIELD_SIZE);

        byte *chunk_data = NULL;
        if (chunk_len > 0) {
            chunk_data = malloc(chunk_len);
            if (NULL == chunk_data) {
                FATAL("failed to allocate memory for chunk data");
                free(idat_data);
                return false;
            }
            if (fread(chunk_data, sizeof(byte), chunk_len, file) != chunk_len) {
                FATAL("failed to read chunk data");
                free(chunk_data);
                free(idat_data);
                return false;
            }
        }
        byte crc_bytes[PNG_CRC_FIELD_SIZE];
        if (fread(crc_bytes, sizeof(byte), PNG_CRC_FIELD_SIZE, file) != PNG_CRC_FIELD_SIZE) {
            FATAL("failed to read chunk crc");
            free(chunk_data);
            free(idat_data);
            return false;
        }

        bool is_idat = (0 == strncmp(PNG_IDAT_CHUNK_ID, (char *)type_bytes, PNG_CHUNK_ID_SIZE));
        bool is_iend = (0 == strncmp(PNG_IEND_CHUNK_ID, (char *)type_bytes, PNG_CHUNK_ID_SIZE));

        if (is_idat) {
            byte *grown = realloc(idat_data, idat_size + chunk_len);
            if (NULL == grown) {
                FATAL("failed to grow idat buffer");
                free(chunk_data);
                free(idat_data);
                return false;
            }
            idat_data = grown;
            memcpy(idat_data + idat_size, chunk_data, chunk_len);
            idat_size += chunk_len;
            free(chunk_data);
            continue;
        }

        if (is_iend && !idat_written) {
            if (!write_new_idat_chunk(out_file, idat_data, idat_size, secretMessage)) {
                free(chunk_data);
                free(idat_data);
                return false;
            }
            idat_written = true;
        }

        if (fwrite(length_bytes, sizeof(byte), PNG_LENGTH_FIELD_SIZE, out_file) !=
                PNG_LENGTH_FIELD_SIZE ||
            fwrite(type_bytes, sizeof(byte), PNG_CHUNK_ID_SIZE, out_file) != PNG_CHUNK_ID_SIZE ||
            (chunk_len > 0 && fwrite(chunk_data, sizeof(byte), chunk_len, out_file) != chunk_len) ||
            fwrite(crc_bytes, sizeof(byte), PNG_CRC_FIELD_SIZE, out_file) != PNG_CRC_FIELD_SIZE) {
            FATAL("failed to write chunk to output");
            free(chunk_data);
            free(idat_data);
            return false;
        }
        free(chunk_data);

        if (is_iend)
            break;
    }

    free(idat_data);
    return true;
}

char *decode_png(FILE *file) {
    if (NULL == file)
        return NULL;
    rewind(file);

    if (png_header_is_not_valid(file))
        return NULL;
    if (!read_ihdr_chunk(file))
        return NULL;

    uint32_t idat_size = 0;
    byte *idat_data    = read_all_idat_data(file, &idat_size);
    if (NULL == idat_data || idat_size == 0) {
        FATAL("decode_png: no IDAT data found");
        return NULL;
    }

    uint32_t decoded_size = 0;
    byte *decoded         = zlib_decode(idat_data, idat_size, &decoded_size);
    free(idat_data);
    if (NULL == decoded) {
        FATAL("decode_png: zlib_decode failed");
        return NULL;
    }

    char *message = lsb_extract(decoded, decoded_size);
    free(decoded);
    return message;
}

static bool write_new_idat_chunk(FILE *out_file, byte *idat_data, uint32_t idat_size,
                                 const char *secretMessage) {
    uint32_t decoded_size = 0;
    byte *decoded         = zlib_decode(idat_data, idat_size, &decoded_size);
    if (NULL == decoded) {
        FATAL("write_new_idat_chunk: failed to zlib-decode original IDAT data");
        return false;
    }

    // TODO: unfilter scanlines before embedding -- currently embedding directly
    // into the filtered scanline bytes

    if (!lsb_embed(decoded, decoded_size, secretMessage)) {
        free(decoded);
        return false;
    }

    uint32_t new_zlib_size = 0;
    byte *new_zlib_data    = zlib_encode(decoded, decoded_size, &new_zlib_size);
    free(decoded);
    if (NULL == new_zlib_data) {
        FATAL("write_new_idat_chunk: zlib_encode failed");
        return false;
    }

    bool ok = write_chunk(out_file, PNG_IDAT_CHUNK_ID, new_zlib_data, new_zlib_size);
    free(new_zlib_data);
    return ok;
}

static void write_be32(byte *out, uint32_t value) {
    out[0] = (byte)(value >> 24);
    out[1] = (byte)(value >> 16);
    out[2] = (byte)(value >> 8);
    out[3] = (byte)(value);
}

static bool write_chunk(FILE *out_file, const char *type, const byte *data, uint32_t data_len) {
    byte length_bytes[PNG_LENGTH_FIELD_SIZE];
    write_be32(length_bytes, data_len);
    if (fwrite(length_bytes, sizeof(byte), PNG_LENGTH_FIELD_SIZE, out_file) !=
        PNG_LENGTH_FIELD_SIZE)
        return false;
    if (fwrite(type, sizeof(byte), PNG_CHUNK_ID_SIZE, out_file) != PNG_CHUNK_ID_SIZE)
        return false;
    if (data_len > 0 && fwrite(data, sizeof(byte), data_len, out_file) != data_len)
        return false;

    uint32_t crc_input_len = PNG_CHUNK_ID_SIZE + data_len;
    byte *crc_input        = malloc(crc_input_len);
    if (NULL == crc_input)
        return false;
    memcpy(crc_input, type, PNG_CHUNK_ID_SIZE);
    if (data_len > 0)
        memcpy(crc_input + PNG_CHUNK_ID_SIZE, data, data_len);
    uint32_t crc = crc32_calculate(crc_input, crc_input_len);
    free(crc_input);

    byte crc_bytes[PNG_CRC_FIELD_SIZE];
    write_be32(crc_bytes, crc);
    return fwrite(crc_bytes, sizeof(byte), PNG_CRC_FIELD_SIZE, out_file) == PNG_CRC_FIELD_SIZE;
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
    uint32_t ihdr_size     = hexa_to_decimal(BIG_ENDIAN, ihdr_header.size, PNG_LENGTH_FIELD_SIZE);
    uint32_t crc_data_size = ihdr_size + PNG_CHUNK_ID_SIZE;
    byte *data             = (byte *)&ihdr_header;
    data                   = data + PNG_LENGTH_FIELD_SIZE;
    uint32_t crc           = crc32_calculate(data, crc_data_size);

    DEBUG("IHDR chunk size: %d", ihdr_size);
    DEBUG("IHDR ID: %.4s", (char *)ihdr_header.ID);
    DEBUG("IHDR width: %d",
          hexa_to_decimal(BIG_ENDIAN, ihdr_header.width, PNG_IHDR_WIDTH_FIELD_SIZE));
    DEBUG("IHDR height: %d",
          hexa_to_decimal(BIG_ENDIAN, ihdr_header.height, PNG_IHDR_HEIGHT_FIELD_SIZE));
    DEBUG("IHDR color type : %d", ihdr_header.colorType);
    DEBUG("IHDR compression type: %d", ihdr_header.compressionType);
    DEBUG("IHDR filter type: %d", ihdr_header.filterType);
    DEBUG("IHDR interlace method: %d", ihdr_header.interlaceMethod);

    byte ihdr_crc[PNG_CRC_FIELD_SIZE];
    if (fread(ihdr_crc, sizeof(byte), PNG_CRC_FIELD_SIZE, file) != PNG_CRC_FIELD_SIZE) {
        FATAL("failed to read the CRC Checksum of IHDR chunk");
        return false;
    }
    uint32_t decimal_ihdr_crc = hexa_to_decimal(BIG_ENDIAN, ihdr_crc, PNG_CRC_FIELD_SIZE);
    if (decimal_ihdr_crc != crc) {
        FATAL("Calculated CRC checksum is not the same as the actual CRC in the png");
        return false;
    } else {
        DEBUG("CRC CHECKED");
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
        if (c == '\0')
            return message;
    }

    FATAL("lsb_extract: no null terminator found within available data");
    free(message);
    return NULL;
}
