#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "deflate_lz77.h"
#include <stdbool.h>
#include <stdint.h>

#define DEFLATE_LITERAL_LENGTH_SIZE 286
#define DEFLATE_DISTANCE_SIZE       30

typedef struct {
    uint32_t *literal_length_frequency;
    uint32_t *distance_frequency;
} frequency_t;

typedef struct huffman_tree_t {
    uint16_t symbol;
    uint32_t frequency;
    struct huffman_tree_t *left;
    struct huffman_tree_t *right;
} huffman_tree_t;

typedef struct {
    uint16_t length;
    uint32_t code;
    uint16_t symbol;
} huffman_code_t;

typedef struct {
    huffman_code_t *literal_len;
    uint16_t literal_len_size;
    huffman_code_t *distance;
    uint16_t distance_size;
    bool status;
} canonical_huffman_code;

typedef struct {
    huffman_code_t *codes;
    uint16_t size;
    bool status;
} huffman_code_set_t;

canonical_huffman_code build_huffman_tree(deflate_lz77_stream_t *deflate);
void free_huffman(canonical_huffman_code code);
void free_huffman_code_set(huffman_code_set_t set);
void print_huffman(const canonical_huffman_code *code);
huffman_code_set_t build_canonical_huffman(const uint32_t *frequency, uint16_t alphabet_size);
#endif
