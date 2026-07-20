#include "../include/zlib_wrapper.h"
#include "../include/deflate.h"
#include "../include/adler32.h"
#include "utils.h"
#include <stdlib.h>

byte *zlib_decode(const byte *data, uint32_t size, uint32_t *decoded_size) {
    if (NULL == data || NULL == decoded_size) {
        FATAL("zlib_decode : NULL argument");
        return NULL;
    }
    if (size < ZLIB_HEADER_SIZE + ZLIB_ADLER32_SIZE) {
        FATAL("zlib_decode : stream too small to contain a valid zlib header + trailer");
        return NULL;
    }

    byte cmf = data[0];
    byte flg = data[1];

    uint16_t check = (uint16_t)((cmf << 8) | flg);
    if (check % 31 != 0) {
        FATAL("zlib_decode : invalid FCHECK, this is not a valid zlib stream");
        return NULL;
    }

    uint8_t compression_method = cmf & 0x0F;
    if (compression_method != ZLIB_CM_DEFLATE) {
        FATAL("zlib_decode : unsupported compression method (PNG requires DEFLATE, method 8)");
        return NULL;
    }

    bool has_preset_dictionary = (flg & 0x20) != 0;
    if (has_preset_dictionary) {
        FATAL("zlib_decode : stream uses a preset dictionary, unsupported");
        return NULL;
    }

    const byte *deflate_stream = data + ZLIB_HEADER_SIZE;
    uint32_t deflate_size      = size - ZLIB_HEADER_SIZE - ZLIB_ADLER32_SIZE;

    uint32_t out_len = 0;
    byte *decoded = deflate_decode(deflate_stream, deflate_size, &out_len);
    if (NULL == decoded) {
        FATAL("zlib_decode : deflate_decode failed");
        return NULL;
    }

    const byte *adler_bytes = data + size - ZLIB_ADLER32_SIZE;
    uint32_t stored_adler = ((uint32_t)adler_bytes[0] << 24) |
                            ((uint32_t)adler_bytes[1] << 16) |
                            ((uint32_t)adler_bytes[2] << 8)  |
                            ((uint32_t)adler_bytes[3]);

    uint32_t computed_adler = adler32(decoded, out_len);

    if (stored_adler != computed_adler) {
        FATAL("zlib_decode : Adler-32 mismatch, decoded data does not match checksum");
        free(decoded);
        return NULL;
    }

    *decoded_size = out_len;
    return decoded;
}
