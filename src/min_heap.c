#include "../include/min_heap.h"
#include "../include/utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

min_heap_t *min_heap_create(uint32_t capacity) {
    if (0 == capacity) {
        FATAL("can not init min heap with capacity of zero");
        return NULL;
    }
    min_heap_t *min_heap = malloc(sizeof(min_heap_t));
    if (NULL == min_heap) {
        FATAL("failed to allocate memory for min heap");
        return NULL;
    }
    min_heap->huffman_nodes = malloc(sizeof(huffman_tree_t *) * capacity);
    if (NULL == min_heap->huffman_nodes) {
        free(min_heap);
        FATAL("failed to allocate memory for min heap's huffman node");
        return NULL;
    }
    min_heap->capacity = capacity;
    min_heap->size     = 0;
    return min_heap;
}

bool min_heap_insert(min_heap_t *min_heap, huffman_tree_t *huffman) {
    if (NULL == min_heap) {
        FATAL("min heap passed to the heap insert is NULL");
        return false;
    }
    if (NULL == huffman) {
        FATAL("huffman passed to the heap insert is NULL");
        return false;
    }
    if (min_heap->size == min_heap->capacity) {
        FATAL("the min heap is full");
        return false;
    }
    uint32_t index                 = min_heap->size;
    min_heap->huffman_nodes[index] = huffman;
    min_heap->size += 1;

    while (index > 0) {
        uint32_t parent = (index - 1) / 2;
        if (min_heap->huffman_nodes[parent]->frequency <=
            min_heap->huffman_nodes[index]->frequency) {
            break;
        }
        huffman_tree_t *temp            = min_heap->huffman_nodes[parent];
        min_heap->huffman_nodes[parent] = min_heap->huffman_nodes[index];
        min_heap->huffman_nodes[index]  = temp;
        index                           = parent;
    }
    return true;
}

huffman_tree_t *min_heap_get_min(min_heap_t *min_heap) {
    if (NULL == min_heap) {
        FATAL("min heap is null");
        return NULL;
    }
    if (0 == min_heap->size) {
        FATAL("can not get minimum from an empty heap");
        return NULL;
    }
    huffman_tree_t *min        = min_heap->huffman_nodes[0];
    min_heap->huffman_nodes[0] = min_heap->huffman_nodes[min_heap->size - 1];
    min_heap->size -= 1;
    uint32_t index = 0;
    while (1) {
        uint32_t left_child    = (2 * index) + 1;
        uint32_t right_child   = (2 * index) + 2;
        huffman_tree_t *lowest = min_heap->huffman_nodes[index];
        uint32_t lowest_index  = index;
        if (left_child < min_heap->size &&
            min_heap->huffman_nodes[left_child]->frequency < lowest->frequency) {
            lowest       = min_heap->huffman_nodes[left_child];
            lowest_index = left_child;
        }
        if (right_child < min_heap->size &&
            min_heap->huffman_nodes[right_child]->frequency < lowest->frequency) {
            lowest       = min_heap->huffman_nodes[right_child];
            lowest_index = right_child;
        }
        if (lowest_index == index) {
            break;
        }
        huffman_tree_t *temp                  = min_heap->huffman_nodes[index];
        min_heap->huffman_nodes[index]        = min_heap->huffman_nodes[lowest_index];
        min_heap->huffman_nodes[lowest_index] = temp;
        index                                 = lowest_index;
    }
    return min;
}

void min_heap_destroy(min_heap_t *min_heap) {
    if (NULL == min_heap) {
        return;
    }
    free(min_heap->huffman_nodes);
    free(min_heap);
}
