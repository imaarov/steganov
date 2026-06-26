#ifndef HUFFMAN_H
#define HUFFMAN_H

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
    huffman_tree_t *literal_len_leaves;
    huffman_tree_t *distance_leaves;
    uint16_t literal_len_active_symbol;
    uint8_t distance_active_symbol;
} huffman_init_leaves;

typedef struct {
    uint16_t length;
    uint16_t code;
    uint16_t symbol;
} huffman_code_t;

#endif
