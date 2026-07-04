#include "../include/bitstream.h"
#include "../include/utils.h"
#include "../include/deflate.h"
#include <stdint.h>
#include <string.h>

void test_deflate_no_compression() {
    byte *data = (byte *)"hello";
    uint32_t data_len = strlen((char *)data);
    bitstream_t stream = deflate_encode(data, data_len, BTYPE_NO_COMPRESSION); 

    print_bit_stream(&stream);
    bitstream_destroy(&stream);
}
