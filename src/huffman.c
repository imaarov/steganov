#include "../include/huffman.h"
#include "../include/deflate_lz77.h"
#include "../include/min_heap.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static frequency_t initialize_frequency_table(deflate_lz77_stream_t *deflate_stream);
static void build_huffman_len_code(huffman_tree_t *root, huffman_code_t *code, uint16_t len);
static void destroy_huffman_tree(huffman_tree_t *root);
static huffman_tree_t *construct_huffman_tree(min_heap_t *);
static void sort_active_codes(huffman_code_t *, uint32_t);
static void generate_huffman_code(huffman_code_t *, uint32_t);

canonical_huffman_code build_huffman_tree(deflate_lz77_stream_t *deflate) {
    canonical_huffman_code result = {.literal_len = NULL, .distance = NULL, .status = false};

    if (!deflate)
        return result;

    frequency_t freq = initialize_frequency_table(deflate);
    if (!freq.literal_length_frequency || !freq.distance_frequency) {
        free(freq.literal_length_frequency);
        free(freq.distance_frequency);
        return result;
    }

    huffman_code_set_t ll =
        build_canonical_huffman(freq.literal_length_frequency, DEFLATE_LITERAL_LENGTH_SIZE);
    huffman_code_set_t dist =
        build_canonical_huffman(freq.distance_frequency, DEFLATE_DISTANCE_SIZE);

    free(freq.literal_length_frequency);
    free(freq.distance_frequency);

    if (!ll.status) {
        free_huffman_code_set(dist);
        return result;
    }

    result.literal_len      = ll.codes;
    result.literal_len_size = ll.size;
    result.distance         = dist.codes;
    result.distance_size    = dist.size;
    result.status           = true;
    return result;
}

void free_huffman(canonical_huffman_code huffman_code) {
    free(huffman_code.distance);
    free(huffman_code.literal_len);
}

void print_huffman(const canonical_huffman_code *code) {
    for (uint16_t i = 0; i < code->literal_len_size; i++) {
        printf("literal : length: %u, code: %u, symbol: %u\n", code->literal_len[i].length,
               code->literal_len[i].code, code->literal_len[i].symbol);
    }
    for (uint16_t i = 0; i < code->distance_size; i++) {
        printf("distance : length: %u, code: %u, symbol: %u\n", code->distance[i].length,
               code->distance[i].code, code->distance[i].symbol);
    }
}

huffman_code_set_t build_canonical_huffman(const uint32_t *frequency, uint16_t alphabet_size) {
    huffman_code_set_t result = {.codes = NULL, .size = 0, .status = false};
    if (NULL == frequency || alphabet_size == 0)
        return result;

    huffman_tree_t **leaves = malloc(sizeof(huffman_tree_t *) * alphabet_size);
    if (!leaves)
        return result;

    uint16_t n = 0;
    for (uint16_t s = 0; s < alphabet_size; s++) {
        if (frequency[s] > 0) {
            huffman_tree_t *leaf = malloc(sizeof(huffman_tree_t));
            if (!leaf) {
                for (uint16_t i = 0; i < n; i++)
                    free(leaves[i]);
                free(leaves);
                return result;
            }
            leaf->frequency = frequency[s];
            leaf->symbol    = s;
            leaf->left = leaf->right = NULL;
            leaves[n++]              = leaf;
        }
    }

    if (n == 0) {
        free(leaves);
        result.status = true;
        return result;
    }

    huffman_code_t *active = malloc(sizeof(huffman_code_t) * n);
    if (!active) {
        for (uint16_t i = 0; i < n; i++)
            free(leaves[i]);
        free(leaves);
        return result;
    }

    if (n == 1) {
        active[0] = (huffman_code_t){.length = 1, .symbol = leaves[0]->symbol, .code = 0};
        free(leaves[0]);
        free(leaves);
        result.codes  = active;
        result.size   = 1;
        result.status = true;
        return result;
    }

    min_heap_t *heap = min_heap_create(n);
    if (!heap) {
        for (uint16_t i = 0; i < n; i++)
            free(leaves[i]);
        free(leaves);
        free(active);
        return result;
    }
    for (uint16_t i = 0; i < n; i++) {
        min_heap_insert(heap, leaves[i]);
        leaves[i] = NULL;
    }
    free(leaves);

    huffman_tree_t *root = construct_huffman_tree(heap);
    min_heap_destroy(heap);
    if (!root) {
        free(active);
        return result;
    }

    huffman_code_t *full = calloc(alphabet_size, sizeof(huffman_code_t));
    if (!full) {
        free(active);
        destroy_huffman_tree(root);
        return result;
    }
    build_huffman_len_code(root, full, 0);
    destroy_huffman_tree(root);

    uint16_t j = 0;
    for (uint16_t i = 0; i < alphabet_size; i++) {
        if (full[i].length != 0)
            active[j++] = full[i];
    }
    assert(j == n && "lost a leaf during huffman tree construction");
    free(full);

    sort_active_codes(active, n);
    generate_huffman_code(active, n);

    result.codes  = active;
    result.size   = n;
    result.status = true;
    return result;
}

void free_huffman_code_set(huffman_code_set_t set) { free(set.codes); }

static void build_huffman_len_code(huffman_tree_t *root, huffman_code_t *code, uint16_t len) {
    if (NULL == root) {
        return;
    }
    if (NULL == code) {
        return;
    }
    build_huffman_len_code(root->left, code, len + 1);
    build_huffman_len_code(root->right, code, len + 1);
    if (NULL == root->right && NULL == root->left) {
        code[root->symbol].length = len == 0 ? 1 : len;
        code[root->symbol].symbol = root->symbol;
    }
}

static frequency_t initialize_frequency_table(deflate_lz77_stream_t *deflate_stream) {
    frequency_t frequency = {.distance_frequency = NULL, .literal_length_frequency = NULL};
    if (NULL == deflate_stream) {
        FATAL("failed to initialize frequency table; deflate stream was NULL");
        return frequency;
    }
    uint32_t *literal_and_length_frequency = malloc(sizeof(uint32_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    if (NULL == literal_and_length_frequency) {
        FATAL("failed to allocated memory for literal and length frequency");
        return frequency;
    }
    memset(literal_and_length_frequency, 0, sizeof(uint32_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    uint32_t *distance_frequency = malloc(sizeof(uint32_t) * DEFLATE_DISTANCE_SIZE);
    if (NULL == distance_frequency) {
        FATAL("failed to allocated memory for distance frequency");
        free(literal_and_length_frequency);
        frequency.literal_length_frequency = NULL;
        return frequency;
    }
    memset(distance_frequency, 0, sizeof(uint32_t) * DEFLATE_DISTANCE_SIZE);
    for (uint32_t i = 0; i < deflate_stream->size; i++) {
        if (deflate_stream->tokens[i].type == LITERAL) {
            literal_and_length_frequency[deflate_stream->tokens[i].literal]++;
        } else if (deflate_stream->tokens[i].type == MATCH) {
            distance_frequency[deflate_stream->tokens[i].distance_symbol]++;
            literal_and_length_frequency[deflate_stream->tokens[i].length_symbol]++;
        } else if (deflate_stream->tokens[i].type == EOB_TOKEN) {
            literal_and_length_frequency[EOB]++;
        }
    }
    frequency.distance_frequency       = distance_frequency;
    frequency.literal_length_frequency = literal_and_length_frequency;
    return frequency;
}

void destroy_huffman_tree(huffman_tree_t *root) {
    if (NULL == root) {
        return;
    }
    destroy_huffman_tree(root->left);
    destroy_huffman_tree(root->right);
    free(root);
}

static huffman_tree_t *construct_huffman_tree(min_heap_t *heap) {
    while (heap->size > 1) {
        huffman_tree_t *left   = min_heap_get_min(heap);
        huffman_tree_t *right  = min_heap_get_min(heap);
        huffman_tree_t *parent = malloc(sizeof(huffman_tree_t));
        if (NULL == parent) {
            FATAL("failed to allocated memory for huffman parent node");
            destroy_huffman_tree(left);
            destroy_huffman_tree(right);
            return NULL;
        }
        parent->frequency = left->frequency + right->frequency;
        parent->symbol    = 0;
        parent->left      = left;
        parent->right     = right;
        if (!min_heap_insert(heap, parent)) {
            destroy_huffman_tree(parent);
            return NULL;
        }
    }
    return min_heap_get_min(heap);
}

static void sort_active_codes(huffman_code_t *codes, uint32_t active_size) {
    for (uint16_t i = 0; i < active_size; i++) {
        uint16_t size = i;
        while (size > 0) {
            if (codes[size].length < codes[size - 1].length ||
                (codes[size].length == codes[size - 1].length &&
                 codes[size].symbol < codes[size - 1].symbol)) {
                huffman_code_t temp = codes[size];
                codes[size]         = codes[size - 1];
                codes[size - 1]     = temp;
                --size;
            } else {
                break;
            }
        }
    }
}

static void generate_huffman_code(huffman_code_t *huffman_code, uint32_t active_size) {
    uint32_t code = 0;
    if (active_size > 0) {
        uint16_t prev_len = huffman_code[0].length;
        for (uint16_t i = 0; i < active_size; i++) {
            code <<= (huffman_code[i].length - prev_len);
            huffman_code[i].code = code;
            prev_len             = huffman_code[i].length;
            ++code;
        }
    }
}
