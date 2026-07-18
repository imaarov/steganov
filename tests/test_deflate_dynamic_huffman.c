#include "../include/bitstream.h"
#include "../include/deflate.h"
#include "../include/utils.h"
#include <stdint.h>
#include <string.h>

void test_deflate_dynamic_huffman() {
    char *data         = "abcabcabcabc";
    uint32_t data_len  = strlen(data);
    bitstream_t stream = deflate_encode((byte *)data, data_len, BTYPE_DYNAMIC_HUFFMAN);

    print_bit_stream(&stream);
    bitstream_destroy(&stream);
}
