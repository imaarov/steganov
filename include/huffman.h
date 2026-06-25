#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>

#define DEFLATE_LITERAL_LENGTH_SIZE 285
#define DEFLATE_DISTANCE_SIZE 30

typedef struct {
    uint16_t *literal_length_frequency;
    uint16_t *distance_frequency;
} frequency_t;

typedef struct huffman_tree_t {
    uint16_t symbol;
    uint32_t frequency;
    struct huffman_tree_t *left;
    struct huffman_tree_t *right;
} huffman_tree_t;

typedef struct {
    uint16_t length;
    uint16_t code;
    uint16_t symbol;
} huffman_code_t;

#endif
