#include "../include/deflate.h"
#include "../include/lz77.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void test_deflated_lz77_token_stream() {
    char *data       = "aaaabbbccd";
    uint8_t data_len = strlen(data);

    lz_tokens_stream_t *stream = lz_77_encode((Byte *)data, data_len);

    printf("print lz77 tokens\n");
    print_lz77_tokens(stream, stream->size);

    assert(stream != NULL);
    assert(stream->size == 9);

    deflate_stream_t *deflate = deflate_encode(stream);
    assert(deflate != NULL);
    assert(deflate->size == stream->size);
    assert(deflate->tokens[0].literal == stream->tokens[0].literal);
    assert(deflate->tokens[1].length_symbol == LENGTH_CODES[0].symbol);
    assert(deflate->tokens[8].literal == EOB);

    free_lz77(stream);
    printf("print deflated tokens\n");
    print_deflate(deflate);
}
