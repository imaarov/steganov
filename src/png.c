#include "../include/png.h"
#include "../include/crc32.h"
#include "../include/png_filter.h"
#include "../include/utils.h"
#include "../include/zlib_wrapper.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IHDR_BIT_DEPTH_OFFSET   8
#define IHDR_COLOR_TYPE_OFFSET  9
#define IHDR_COMPRESSION_OFFSET 10
#define IHDR_FILTER_OFFSET      11
#define IHDR_INTERLACE_OFFSET   12

#define PNG_COLOR_TYPE_GRAY       0
#define PNG_COLOR_TYPE_RGB        2
#define PNG_COLOR_TYPE_PALETTE    3
#define PNG_COLOR_TYPE_GRAY_ALPHA 4
#define PNG_COLOR_TYPE_RGBA       6

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t bytes_per_sample;
    uint32_t bpp;
    uint32_t stride;
    uint64_t sample_count;
} png_info_t;

typedef struct {
    byte *raw;
    byte *filter_types;
} png_pixels_t;

static bool png_header_is_not_valid(FILE *file);
static bool read_ihdr_chunk(FILE *file, png_info_t *info);
static bool parse_ihdr(const byte *ihdr, png_info_t *info);
static byte *read_all_idat_data(FILE *file, uint32_t *out_size);
static bool load_pixels(const byte *idat_data, uint32_t idat_size, const png_info_t *info,
                        png_pixels_t *pixels);
static void free_pixels(png_pixels_t *pixels);
static uint64_t sample_offset(const png_info_t *info, uint64_t sample_index);
static bool lsb_embed(byte *raw, const png_info_t *info, const char *message);
static char *lsb_extract(const byte *raw, const png_info_t *info);
static void write_be32(byte *out, uint32_t value);
static bool write_chunk(FILE *out_file, const char *type, const byte *data, uint32_t data_len);
static bool write_new_idat_chunk(FILE *out_file, const byte *idat_data, uint32_t idat_size,
                                 const png_info_t *info, const char *secretMessage);

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
    png_info_t info    = {0};
    bool have_ihdr     = false;

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

        bool is_ihdr = (0 == strncmp(PNG_IHDR_CHUNK_ID, (char *)type_bytes, PNG_CHUNK_ID_SIZE));
        bool is_idat = (0 == strncmp(PNG_IDAT_CHUNK_ID, (char *)type_bytes, PNG_CHUNK_ID_SIZE));
        bool is_iend = (0 == strncmp(PNG_IEND_CHUNK_ID, (char *)type_bytes, PNG_CHUNK_ID_SIZE));

        if (is_ihdr) {
            // we need the image geometry to be able to un filter the scanlines later
            if (chunk_len != PNG_IHDR_DATA_SIZE || !parse_ihdr(chunk_data, &info)) {
                FATAL("encode_png: invalid or unsupported IHDR chunk");
                free(chunk_data);
                free(idat_data);
                return false;
            }
            have_ihdr = true;
        }

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
            if (!have_ihdr) {
                FATAL("encode_png: no IHDR chunk found before IEND");
                free(chunk_data);
                free(idat_data);
                return false;
            }
            if (!write_new_idat_chunk(out_file, idat_data, idat_size, &info, secretMessage)) {
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

    png_info_t info = {0};
    if (!read_ihdr_chunk(file, &info))
        return NULL;

    uint32_t idat_size = 0;
    byte *idat_data    = read_all_idat_data(file, &idat_size);
    if (NULL == idat_data || idat_size == 0) {
        FATAL("decode_png: no IDAT data found");
        free(idat_data);
        return NULL;
    }

    // the message is in the real pixel samples, so zlib decode AND un filter before reading
    png_pixels_t pixels = {0};
    bool loaded         = load_pixels(idat_data, idat_size, &info, &pixels);
    free(idat_data);
    if (!loaded)
        return NULL;

    char *message = lsb_extract(pixels.raw, &info);
    free_pixels(&pixels);
    return message;
}

/**
 * re creates the IDAT chunk with the message hidden in it.
 *
 *   zlib decode -> UN FILTER -> embed LSBs in the raw samples -> FILTER AGAIN -> zlib encode
 *
 * Every scanline keeps the filter type it originally had, so the only bytes that differ from the
 * original image (after un filtering) are the LSBs we touched on purpose.
 */
static bool write_new_idat_chunk(FILE *out_file, const byte *idat_data, uint32_t idat_size,
                                 const png_info_t *info, const char *secretMessage) {
    png_pixels_t pixels = {0};
    if (!load_pixels(idat_data, idat_size, info, &pixels)) {
        FATAL("write_new_idat_chunk: failed to read the original pixels");
        return false;
    }

    if (!lsb_embed(pixels.raw, info, secretMessage)) {
        free_pixels(&pixels);
        return false;
    }

    // load_pixels already proved height * (stride + 1) fits in 32 bits
    uint32_t filtered_size = info->height * (info->stride + PNG_FILTER_BYTE_SIZE);
    byte *filtered         = malloc(filtered_size);
    if (NULL == filtered) {
        FATAL("write_new_idat_chunk: failed to allocate the filtered scanline buffer");
        free_pixels(&pixels);
        return false;
    }
    bool filtered_ok = png_filter(pixels.raw, info->height, info->stride, info->bpp,
                                  pixels.filter_types, filtered);
    free_pixels(&pixels);
    if (!filtered_ok) {
        free(filtered);
        return false;
    }

    uint32_t new_zlib_size = 0;
    byte *new_zlib_data    = zlib_encode(filtered, filtered_size, &new_zlib_size);
    free(filtered);
    if (NULL == new_zlib_data) {
        FATAL("write_new_idat_chunk: zlib_encode failed");
        return false;
    }

    bool ok = write_chunk(out_file, PNG_IDAT_CHUNK_ID, new_zlib_data, new_zlib_size);
    free(new_zlib_data);
    return ok;
}

/** zlib decode the concatenated IDAT payload and un filter it into raw pixel samples */
static bool load_pixels(const byte *idat_data, uint32_t idat_size, const png_info_t *info,
                        png_pixels_t *pixels) {
    pixels->raw          = NULL;
    pixels->filter_types = NULL;

    uint32_t filtered_size = 0;
    byte *filtered         = zlib_decode(idat_data, idat_size, &filtered_size);
    if (NULL == filtered) {
        FATAL("load_pixels: zlib_decode failed");
        return false;
    }

    // IHDR says how many bytes the scanlines must add up to, if they don't match something is off
    uint64_t expected_size = (uint64_t)info->height * (info->stride + PNG_FILTER_BYTE_SIZE);
    if (filtered_size != expected_size) {
        FATAL("load_pixels: IDAT holds %u bytes but IHDR describes %llu bytes", filtered_size,
              (unsigned long long)expected_size);
        free(filtered);
        return false;
    }

    pixels->raw          = malloc((size_t)info->height * info->stride);
    pixels->filter_types = malloc(info->height);
    if (NULL == pixels->raw || NULL == pixels->filter_types) {
        FATAL("load_pixels: allocation failed");
        free(filtered);
        free_pixels(pixels);
        return false;
    }

    bool ok = png_unfilter(filtered, info->height, info->stride, info->bpp, pixels->raw,
                           pixels->filter_types);
    free(filtered);
    if (!ok)
        free_pixels(pixels);
    return ok;
}

static void free_pixels(png_pixels_t *pixels) {
    free(pixels->raw);
    free(pixels->filter_types);
    pixels->raw          = NULL;
    pixels->filter_types = NULL;
}

/**
 * validates IHDR and derives the numbers needed to walk the pixel data.
 * anything that would make LSB embedding visible or ambiguous is rejected up front instead of
 * silently producing a damaged image:
 *   - palette images: the samples are palette *indices*, flipping an LSB jumps to a different
 * colour
 *   - bit depth < 8: several pixels are packed into one byte
 *   - Adam7 interlacing: pixels are stored in 7 separate passes, not row by row
 */
static bool parse_ihdr(const byte *ihdr, png_info_t *info) {
    uint32_t width = hexa_to_decimal(BIG_ENDIAN, ihdr, PNG_IHDR_WIDTH_FIELD_SIZE);
    uint32_t height =
        hexa_to_decimal(BIG_ENDIAN, ihdr + PNG_IHDR_WIDTH_FIELD_SIZE, PNG_IHDR_HEIGHT_FIELD_SIZE);
    byte bit_depth  = ihdr[IHDR_BIT_DEPTH_OFFSET];
    byte color_type = ihdr[IHDR_COLOR_TYPE_OFFSET];

    if (0 == width || 0 == height) {
        FATAL("IHDR: image dimensions must not be zero");
        return false;
    }
    if (0 != ihdr[IHDR_COMPRESSION_OFFSET] || 0 != ihdr[IHDR_FILTER_OFFSET]) {
        FATAL("IHDR: unknown compression or filter method");
        return false;
    }
    if (0 != ihdr[IHDR_INTERLACE_OFFSET]) {
        FATAL("IHDR: interlaced (Adam7) PNGs are not supported yet");
        return false;
    }

    uint32_t channels = 0;
    switch (color_type) {
    case PNG_COLOR_TYPE_GRAY:
        channels = 1;
        break;
    case PNG_COLOR_TYPE_RGB:
        channels = 3;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        channels = 2;
        break;
    case PNG_COLOR_TYPE_RGBA:
        channels = 4;
        break;
    case PNG_COLOR_TYPE_PALETTE:
        FATAL("IHDR: palette PNGs are not supported, LSB of an index changes the colour a lot");
        return false;
    default:
        FATAL("IHDR: invalid color type %u", color_type);
        return false;
    }
    if (8 != bit_depth && 16 != bit_depth) {
        FATAL("IHDR: bit depth %u is not supported (only 8 and 16)", bit_depth);
        return false;
    }

    info->width            = width;
    info->height           = height;
    info->channels         = channels;
    info->bytes_per_sample = bit_depth / 8;
    info->bpp              = channels * info->bytes_per_sample;

    // every scanline = 1 filter byte + width * bpp bytes, the decompressed size must fit uint32
    uint64_t stride = (uint64_t)width * info->bpp;
    uint64_t total  = (uint64_t)height * (stride + PNG_FILTER_BYTE_SIZE);
    if (total > UINT32_MAX) {
        FATAL("IHDR: image is too large (%llu bytes of scanlines)", (unsigned long long)total);
        return false;
    }
    info->stride       = (uint32_t)stride;
    info->sample_count = (uint64_t)width * height * channels;
    return true;
}

/**
 * sample number `sample_index` (counted left to right, top to bottom, every channel) -> byte
 * offset inside the raw buffer. For 16 bit images that is the LOW byte of the sample (samples are
 * big-endian), so a flipped LSB changes the value by 1/65535 and not by 1/255.
 */
static uint64_t sample_offset(const png_info_t *info, uint64_t sample_index) {
    uint64_t samples_per_row = (uint64_t)info->width * info->channels;
    uint64_t row             = sample_index / samples_per_row;
    uint64_t col             = sample_index % samples_per_row;
    return row * info->stride + col * info->bytes_per_sample + (info->bytes_per_sample - 1);
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

static bool read_ihdr_chunk(FILE *file, png_info_t *info) {
    PNGIHDRHeader ihdr_header;
    if (fread(&ihdr_header, sizeof(PNGIHDRHeader) - sizeof(ihdr_header.gap_pad), 1, file) != 1) {
        FATAL("failed to read the IHDR header");
        return false;
    }
    uint32_t ihdr_size = hexa_to_decimal(BIG_ENDIAN, ihdr_header.size, PNG_LENGTH_FIELD_SIZE);
    // ihdr_size feeds the CRC calculation below, never should be point past our 13 byte struct
    if (PNG_IHDR_DATA_SIZE != ihdr_size ||
        0 != strncmp(PNG_IHDR_CHUNK_ID, (char *)ihdr_header.ID, PNG_CHUNK_ID_SIZE)) {
        FATAL("the first chunk is not a valid IHDR chunk");
        return false;
    }
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
    const byte *ihdr_payload =
        (const byte *)&ihdr_header + PNG_LENGTH_FIELD_SIZE + PNG_CHUNK_ID_SIZE;
    return parse_ihdr(ihdr_payload, info);
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

/**
 * hide the message (plus its '\0' terminator) one bit per sample, lowest message bit first.
 * `raw` must be UN FILTERED pixel data, never the filtered scanline bytes.
 */
static bool lsb_embed(byte *raw, const png_info_t *info, const char *message) {
    uint64_t message_len = (uint64_t)strlen(message) + 1;
    uint64_t bits_needed = message_len * BITS_PER_BYTE;

    if (bits_needed > info->sample_count) {
        FATAL("lsb_embed: the message needs %llu bits but this image can only hide %llu",
              (unsigned long long)bits_needed, (unsigned long long)info->sample_count);
        return false;
    }

    uint64_t bit_index = 0;
    for (uint64_t i = 0; i < message_len; i++) {
        byte c = (byte)message[i];
        for (int b = 0; b < BITS_PER_BYTE; b++) {
            byte bit        = (c >> b) & 1;
            uint64_t offset = sample_offset(info, bit_index);
            raw[offset]     = (byte)((raw[offset] & LSB_MASK) | bit);
            bit_index++;
        }
    }
    return true;
}

static char *lsb_extract(const byte *raw, const png_info_t *info) {
    uint64_t max_chars = info->sample_count / BITS_PER_BYTE;
    char *message      = malloc(max_chars + 1);
    if (NULL == message) {
        FATAL("lsb_extract: allocation failed");
        return NULL;
    }

    uint64_t bit_index = 0;
    for (uint64_t i = 0; i < max_chars; i++) {
        byte c = 0;
        for (int b = 0; b < BITS_PER_BYTE; b++) {
            byte bit = raw[sample_offset(info, bit_index)] & 1;
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
