#include "../include/zlib_wrapper.h"
#include "../include/adler32.h"
#include "../include/deflate.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

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
    byte *decoded    = deflate_decode(deflate_stream, deflate_size, &out_len);
    if (NULL == decoded) {
        FATAL("zlib_decode : deflate_decode failed");
        return NULL;
    }

    const byte *adler_bytes = data + size - ZLIB_ADLER32_SIZE;
    uint32_t stored_adler   = ((uint32_t)adler_bytes[0] << 24) | ((uint32_t)adler_bytes[1] << 16) |
                            ((uint32_t)adler_bytes[2] << 8) | ((uint32_t)adler_bytes[3]);

    uint32_t computed_adler = adler32(decoded, out_len);

    if (stored_adler != computed_adler) {
        FATAL("zlib_decode : Adler-32 mismatch, decoded data does not match checksum");
        free(decoded);
        return NULL;
    }

    *decoded_size = out_len;
    return decoded;
}

byte *zlib_encode(const byte *data, uint32_t size, uint32_t *encoded_size) {
    bitstream_t deflate_stream = deflate_encode((byte *)data, size, BTYPE_DYNAMIC_HUFFMAN);
    uint32_t deflate_size      = (uint32_t)bitstream_size(&deflate_stream);

    uint32_t total_size = ZLIB_HEADER_SIZE + deflate_size + ZLIB_ADLER32_SIZE;
    byte *out           = malloc(total_size);
    if (NULL == out) {
        FATAL("zlib_encode: allocation failed");
        bitstream_destroy(&deflate_stream);
        return NULL;
    }

    out[0] = 0x78; // CMF: CM=8 (deflate), CINFO=7 (32k window)
    out[1] = 0x9C; // FLG: FDICT=0

    memcpy(out + ZLIB_HEADER_SIZE, deflate_stream.data, deflate_size);
    bitstream_destroy(&deflate_stream);

    uint32_t adler  = adler32(data, size);
    byte *adler_pos = out + ZLIB_HEADER_SIZE + deflate_size;
    adler_pos[0]    = (byte)(adler >> 24);
    adler_pos[1]    = (byte)(adler >> 16);
    adler_pos[2]    = (byte)(adler >> 8);
    adler_pos[3]    = (byte)(adler);

    *encoded_size = total_size;
    return out;
}
