#include "../include/deflate.h"
#include "../include/huffman.h"
#include "../include/lz77.h"
#include <assert.h>
#include <bits/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void test_huffman_code_table() {
    unsigned char *data = malloc(33000);

    memset(data, 'A', 33000);
    // uint16_t data_len = strlen(data);
    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    lz_tokens_stream_t *stream = lz_77_encode((Byte *)data, 33000);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double timespent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("lz 77 elapsed: %.9f sec\n", timespent);

    uint32_t match_count = 0;
    for (uint32_t i = 0; i < stream->size; i++) {
        if (stream->tokens[i].type == MATCH) {
            match_count++;
        }
    }

    assert(stream != NULL);

    printf("print lz77 tokens\n");
    print_lz77_tokens(stream, stream->size);

    clock_gettime(CLOCK_MONOTONIC, &start);
    deflate_stream_t *deflate = deflate_encode(stream);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timespent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("deflate elapsed: %.9f sec\n", timespent);

    printf("print deflate token\n");
    print_deflate(deflate);
    clock_gettime(CLOCK_MONOTONIC, &start);
    canonical_huffman_code huffman_code = build_huffman_tree(deflate);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timespent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("canonical huffman elapsed: %.9f sec\n", timespent);
    printf("print canonical huffman code");
    print_huffman(&huffman_code);

    free_lz77(stream);
}
