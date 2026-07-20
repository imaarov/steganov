#include "../include/bitstream.h"
#include "../include/deflate.h"
#include "../include/utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void assert_round_trip(const char *name, const byte *data, uint32_t data_len, BTYPE btype) {
    bitstream_t stream = deflate_encode((byte *)data, data_len, btype);

    uint32_t compressed_size = (uint32_t)bitstream_size(&stream);
    uint32_t decoded_len     = 0;
    byte *decoded            = deflate_decode(stream.data, compressed_size, &decoded_len);

    printf("[%s] original=%u bytes compressed=%u bytes ", name, data_len, compressed_size);

    assert(decoded != NULL && "decode failed / returned NULL");
    assert(decoded_len == data_len && "decoded length does not match original length");
    assert(memcmp(decoded, data, data_len) == 0 && "decoded bytes do not match original bytes");

    printf("decoded=%u bytes -> PASS\n", decoded_len);

    free(decoded);
    bitstream_destroy(&stream);
}

void test_deflate_dynamic_huffman() {
    char *data         = "abcabcabcabc";
    uint32_t data_len  = strlen(data);
    bitstream_t stream = deflate_encode((byte *)data, data_len, BTYPE_DYNAMIC_HUFFMAN);
    print_bit_stream(&stream);
    bitstream_destroy(&stream);
}

void test_deflate_round_trip() {
    assert_round_trip("literals_only", (byte *)"hellp", 5, BTYPE_DYNAMIC_HUFFMAN);

    assert_round_trip("simple_match", (byte *)"abcabcabcabc", 12, BTYPE_DYNAMIC_HUFFMAN);

    {
        uint32_t n = 300;
        byte *buf  = malloc(n);
        memset(buf, 'A', n);
        assert_round_trip("single_byte_run_300", buf, n, BTYPE_DYNAMIC_HUFFMAN);
        free(buf);
    }

    {
        uint32_t n = 256 * 4;
        byte *buf  = malloc(n);
        for (uint32_t i = 0; i < n; i++)
            buf[i] = (byte)(i % 256);
        assert_round_trip("all_byte_values_x4", buf, n, BTYPE_DYNAMIC_HUFFMAN);
        free(buf);
    }

    {
        const char *text = "the quick brown fox jumps over the lazy dog. "
                           "the quick brown fox jumps over the lazy dog again. "
                           "pack my box with five dozen liquor jugs.";
        assert_round_trip("mixed_text", (byte *)text, (uint32_t)strlen(text),
                          BTYPE_DYNAMIC_HUFFMAN);
    }

    assert_round_trip("single_char", (byte *)"x", 1, BTYPE_DYNAMIC_HUFFMAN);

    {
        uint32_t gap = 5000;
        uint32_t n   = gap + 8;
        byte *buf    = malloc(n);
        memcpy(buf, "PATTERN1", 8);
        for (uint32_t i = 8; i < gap; i++)
            buf[i] = (byte)('a' + (i % 26));
        memcpy(buf + gap, "PATTERN1", 8);
        assert_round_trip("long_distance", buf, n, BTYPE_DYNAMIC_HUFFMAN);
        free(buf);
    }

    assert_round_trip("no_compression", (byte *)"hellp", 5, BTYPE_NO_COMPRESSION);

    printf("all deflate round-trip tests passed\n");
}
