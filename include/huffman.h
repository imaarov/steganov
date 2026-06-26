#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "deflate.h"
#include <stdint.h>
#include <stdbool.h>

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
    huffman_tree_t **literal_len_leaves;
    huffman_tree_t **distance_leaves;
    uint16_t literal_len_active_symbol;
    uint8_t distance_active_symbol;
} huffman_init_leaves;

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

canonical_huffman_code build_huffman_tree(deflate_stream_t *deflate);
void print_huffman(const canonical_huffman_code *code);

#endif
