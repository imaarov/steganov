#include "../include/deflate.h"
#include "../include/bitstream.h"
#include "deflate_lz77.h"
#include "lz77.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

static void encode_deflate_no_compression(byte *data, uint32_t data_len, bitstream_t *bitstream);

bitstream_t deflate_encode(byte *un_compressed_data, uint32_t data_len, BTYPE btype) {
    bitstream_t stream;
    bitstream_init(&stream);

    if (btype == BTYPE_NO_COMPRESSION) {
        encode_deflate_no_compression(un_compressed_data, data_len, &stream);
        return stream;
    }

    lz_tokens_stream_t *lz_stream = lz_77_encode(un_compressed_data, data_len);
    if (NULL == lz_stream) {
        FATAL("failed to encode lz77 in deflate encoder");
        return stream;
    }

    deflate_lz77_stream_t *deflate_lz77_stream = deflate_lz77_encode(lz_stream);
    if (NULL == deflate_lz77_stream) {
        FATAL("failed to encode deflate lz77 in deflate encoder");
        free_lz77(lz_stream);
        return stream;
    }

    // should add the others BTYPE here but i start from the BTYPE_NO_COMPRESSION for now
    return stream;
}

static void encode_deflate_no_compression(byte *data, uint32_t data_len, bitstream_t *stream) {
    uint32_t process_bytes = 0;

    while (process_bytes < data_len) {
        // embed the deflate block header
        uint32_t block_size = data_len - process_bytes;
        if (block_size > UNCOMPRESSED_BLOCK_SIZE) {
            block_size = UNCOMPRESSED_BLOCK_SIZE;
        }
        bitstream_write_bits_lsb(stream, process_bytes + block_size >= data_len ? 1 : 0, 1);
        bitstream_write_bits_lsb(stream, BTYPE_NO_COMPRESSION, 2);
        // based on the 1951 RFC we should skip bits remain from the byte after emben deflate block
        // header
        uint8_t padding_bits = stream->bit_pos == 7 ? 0 : 8 - stream->bit_pos;
        bitstream_write_bits_lsb(stream, 0, padding_bits);

        // 1951 RFC: embed 16 bits LEN (data len)
        bitstream_write_bits_lsb(stream, block_size, 16);
        // 1951 RFC: embed 16 bits NLEN (data len in ones complement)
        bitstream_write_bits_lsb(stream, ~block_size & 0xFFFF, 16);

        for (uint32_t i = 0; i < block_size; i++) {
            bitstream_write_bits_lsb(stream, data[process_bytes + i], 8);
        }

        process_bytes += block_size;
    }
}
