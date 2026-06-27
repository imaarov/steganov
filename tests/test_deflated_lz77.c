#include "../include/lz77.h"
#include "../include/deflate.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void test_deflated_lz77_token_stream() {
    char *data = "aaaabbbccd";
    uint8_t data_len = strlen(data);

    lz_tokens_stream_t *stream = lz_77_encode((Byte *)data, data_len);

    printf("print lz77 tokens\n");
    print_lz77_tokens(stream, stream->size);

    assert(stream != NULL);
    assert(stream->size == 9);

    deflate_stream_t *deflate = deflate_encode(stream);

    printf("print deflated tokens\n");
    print_deflate(deflate);
}
