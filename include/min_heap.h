#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include "huffman.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t capacity;
    uint32_t size;
    huffman_tree_t **huffman_nodes;
} min_heap_t;

min_heap_t *min_heap_create(uint32_t capacity);
void min_heap_destroy(min_heap_t *min_heap);
bool min_heap_insert(min_heap_t *min_heap, huffman_tree_t *huffman);
huffman_tree_t *min_heap_get_min(min_heap_t *min_heap);

#endif
