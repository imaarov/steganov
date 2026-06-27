#include "../include/lz77.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void test_lz77_token_stream() {
    char *data = "aaaabbbccd";
    uint8_t data_len = strlen(data);
    printf("print lz77 tokens\n");
    lz_tokens_stream_t *stream = lz_77_encode((Byte *)data, data_len);
    print_lz77_tokens(stream, stream->size);


    assert(stream != NULL);
    assert(stream->size == 9);
    assert(stream->tokens[0].type == LITERAL);
    assert(stream->tokens[0].literal == 'a');

    assert(stream->tokens[1].type == MATCH);
    assert(stream->tokens[1].distance == 1);
    assert(stream->tokens[1].length == 3);

    assert(stream->tokens[8].type == EOB_TOKEN);

    free_lz77(stream);
}
