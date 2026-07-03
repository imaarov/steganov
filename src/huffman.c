#include "../include/huffman.h"
#include "../include/deflate_lz77.h"
#include "../include/min_heap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t get_active_literal_length_symbol_count(const frequency_t *frequency);
static uint8_t get_active_distance_symbol(const frequency_t *frequency);
static frequency_t initialize_frequency_table(deflate_stream_t *deflate_stream);
static huffman_init_leaves *initialize_leaves_array_from_frequency(const frequency_t *frequency);
static void build_huffman_len_code(huffman_tree_t *root, huffman_code_t *code, uint16_t len);
static void destroy_huffman_tree(huffman_tree_t *root);
static bool fill_min_heap(min_heap_t *, huffman_tree_t **, uint16_t);
static huffman_tree_t *construct_huffman_tree(min_heap_t *);
static void sort_active_codes(huffman_code_t *, uint32_t);
static void generate_huffman_code(huffman_code_t *, uint32_t);

canonical_huffman_code build_huffman_tree(deflate_stream_t *deflate) {
    canonical_huffman_code canonical_code = {
        .literal_len = NULL, .distance = NULL, .status = false};
    frequency_t frequency                    = {0};
    huffman_init_leaves *init_leaves         = NULL;
    min_heap_t *min_heap_distance            = NULL;
    huffman_tree_t *literal_len_root         = NULL;
    huffman_tree_t *distance_root            = NULL;
    huffman_code_t *literal_len_codes        = NULL;
    huffman_code_t *distance_codes           = NULL;
    huffman_code_t *literal_len_active_codes = NULL;
    huffman_code_t *distance_active_codes    = NULL;
    min_heap_t *min_heap_literal_len         = NULL;

    if (NULL == deflate) {
        goto cleanup;
    }
    frequency = initialize_frequency_table(deflate);
    if (NULL == frequency.distance_frequency || NULL == frequency.literal_length_frequency) {
        goto cleanup;
    }

    init_leaves = initialize_leaves_array_from_frequency(&frequency);
    if (NULL == init_leaves) {
        goto cleanup;
    }

    uint16_t ll_count = init_leaves->literal_len_active_symbol;
    uint8_t d_count   = init_leaves->distance_active_symbol;

    if (ll_count > 0 && NULL == init_leaves->literal_len_leaves) {
        goto cleanup;
    }
    if (d_count > 0 && NULL == init_leaves->distance_leaves) {
        goto cleanup;
    }

    if (d_count > 0) {
        min_heap_distance = min_heap_create(init_leaves->distance_active_symbol);
        if (NULL == min_heap_distance) {
            goto cleanup;
        }
        if (!fill_min_heap(min_heap_distance, init_leaves->distance_leaves,
                           init_leaves->distance_active_symbol))
            goto cleanup;
        distance_root = construct_huffman_tree(min_heap_distance);
        if (NULL == distance_root) {
            goto cleanup;
        }

        distance_codes = malloc(sizeof(huffman_code_t) * DEFLATE_DISTANCE_SIZE);
        if (NULL == distance_codes) {
            FATAL("failed to allocated memory for distance code's len type");
            goto cleanup;
        }
        memset(distance_codes, 0, sizeof(huffman_code_t) * DEFLATE_DISTANCE_SIZE);
        build_huffman_len_code(distance_root, distance_codes, 0);

        distance_active_codes =
            malloc(sizeof(huffman_code_t) * init_leaves->distance_active_symbol);
        if (NULL == distance_active_codes) {
            goto cleanup;
        }
        for (uint16_t i = 0, j = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
            if (distance_codes[i].length != 0) {
                distance_active_codes[j++] = distance_codes[i];
            }
        }

        sort_active_codes(distance_active_codes, init_leaves->distance_active_symbol);
        generate_huffman_code(distance_active_codes, init_leaves->distance_active_symbol);
    }

    min_heap_literal_len = min_heap_create(init_leaves->literal_len_active_symbol);
    if (NULL == min_heap_literal_len) {
        goto cleanup;
    }

    if (!fill_min_heap(min_heap_literal_len, init_leaves->literal_len_leaves,
                       init_leaves->literal_len_active_symbol))
        goto cleanup;
    literal_len_root = construct_huffman_tree(min_heap_literal_len);
    if (NULL == literal_len_root) {
        goto cleanup;
    }

    literal_len_codes = malloc(sizeof(huffman_code_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    if (NULL == literal_len_codes) {
        FATAL("failed to allocated memory for literal and length code's len type");
        goto cleanup;
    }
    memset(literal_len_codes, 0, sizeof(huffman_code_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    build_huffman_len_code(literal_len_root, literal_len_codes, 0);
    literal_len_active_codes =
        malloc(sizeof(huffman_code_t) * init_leaves->literal_len_active_symbol);
    if (NULL == literal_len_active_codes) {
        goto cleanup;
    }

    for (uint16_t i = 0, j = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (literal_len_codes[i].length != 0) {
            literal_len_active_codes[j++] = literal_len_codes[i];
        }
    }
    sort_active_codes(literal_len_active_codes, init_leaves->literal_len_active_symbol);
    generate_huffman_code(literal_len_active_codes, init_leaves->literal_len_active_symbol);

    canonical_code.literal_len      = literal_len_active_codes;
    canonical_code.literal_len_size = init_leaves->literal_len_active_symbol;
    canonical_code.distance         = distance_active_codes;
    canonical_code.distance_size    = init_leaves->distance_active_symbol;
    canonical_code.status           = true;
cleanup:
    if (init_leaves) {
        for (uint16_t i = 0; i < init_leaves->literal_len_active_symbol; i++) {
            if (init_leaves->literal_len_leaves[i])
                destroy_huffman_tree(init_leaves->literal_len_leaves[i]);
        }
        for (uint8_t i = 0; i < init_leaves->distance_active_symbol; i++) {
            if (init_leaves->distance_leaves[i])
                destroy_huffman_tree(init_leaves->distance_leaves[i]);
        }
        free(init_leaves->distance_leaves);
        free(init_leaves->literal_len_leaves);
        free(init_leaves);
    }
    if (min_heap_literal_len) {
        for (uint32_t i = 0; i < min_heap_literal_len->size; i++) {
            destroy_huffman_tree(min_heap_literal_len->huffman_nodes[i]);
        }
        min_heap_destroy(min_heap_literal_len);
    }
    if (min_heap_distance) {
        for (uint32_t i = 0; i < min_heap_distance->size; i++) {
            destroy_huffman_tree(min_heap_distance->huffman_nodes[i]);
        }
        min_heap_destroy(min_heap_distance);
    }
    if (literal_len_root)
        destroy_huffman_tree(literal_len_root);
    if (distance_root)
        destroy_huffman_tree(distance_root);
    if (!canonical_code.status) {
        free(literal_len_active_codes);
        free(distance_active_codes);
    }
    free(frequency.literal_length_frequency);
    free(frequency.distance_frequency);
    free(literal_len_codes);
    free(distance_codes);

    return canonical_code;
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

static huffman_init_leaves *initialize_leaves_array_from_frequency(const frequency_t *frequency) {
    uint16_t literal_length_active_symbol  = get_active_literal_length_symbol_count(frequency);
    uint8_t distance_active_symbol         = get_active_distance_symbol(frequency);
    huffman_init_leaves *leaves            = NULL;
    uint16_t literal_length_index          = 0;
    uint16_t distance_index                = 0;
    huffman_tree_t **literal_length_leaves = NULL;
    huffman_tree_t **distance_leaves       = NULL;

    if (literal_length_active_symbol > 0) {
        literal_length_leaves = malloc(sizeof(huffman_tree_t *) * literal_length_active_symbol);
        if (NULL == literal_length_leaves) {
            FATAL("failed to allocated memory for literal length leaves");
            return NULL;
        }
    }
    if (distance_active_symbol > 0) {
        distance_leaves = malloc(sizeof(huffman_tree_t *) * distance_active_symbol);
        if (NULL == distance_leaves) {
            FATAL("failed to allocated memory for distance leaves");
            free(literal_length_leaves);
            return NULL;
        }
    }
    for (uint16_t i = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            literal_length_leaves[literal_length_index] = malloc(sizeof(huffman_tree_t));
            if (NULL == literal_length_leaves[literal_length_index]) {
                goto error;
            }
            literal_length_leaves[literal_length_index]->frequency =
                frequency->literal_length_frequency[i];
            literal_length_leaves[literal_length_index]->symbol = i;
            literal_length_leaves[literal_length_index]->left   = NULL;
            literal_length_leaves[literal_length_index]->right  = NULL;
            ++literal_length_index;
        }
    }
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            distance_leaves[distance_index] = malloc(sizeof(huffman_tree_t));
            if (NULL == distance_leaves[distance_index]) {
                goto error;
            }
            distance_leaves[distance_index]->frequency = frequency->distance_frequency[i];
            distance_leaves[distance_index]->symbol    = i;
            distance_leaves[distance_index]->left      = NULL;
            distance_leaves[distance_index]->right     = NULL;
            ++distance_index;
        }
    }
    leaves = malloc(sizeof(huffman_init_leaves));
    if (NULL == leaves) {
        FATAL("failed to allocate memory for initial huffman leaves");
        goto error;
    }
    leaves->literal_len_leaves        = literal_length_leaves;
    leaves->literal_len_active_symbol = literal_length_active_symbol;
    leaves->distance_leaves           = distance_leaves;
    leaves->distance_active_symbol    = distance_active_symbol;
    return leaves;

error:
    for (uint16_t i = 0; i < distance_index; i++) {
        free(distance_leaves[i]);
    }
    free(distance_leaves);
    for (uint16_t i = 0; i < literal_length_index; i++) {
        free(literal_length_leaves[i]);
    }
    free(literal_length_leaves);
    return NULL;
}

static frequency_t initialize_frequency_table(deflate_stream_t *deflate_stream) {
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

static uint16_t get_active_literal_length_symbol_count(const frequency_t *frequency) {
    uint16_t literal_length_active_symbol = 0;
    for (uint16_t i = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            ++literal_length_active_symbol;
        }
    }
    return literal_length_active_symbol;
}

static uint8_t get_active_distance_symbol(const frequency_t *frequency) {
    uint8_t distance_active_symbol = 0;
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            ++distance_active_symbol;
        }
    }
    return distance_active_symbol;
}

void destroy_huffman_tree(huffman_tree_t *root) {
    if (NULL == root) {
        return;
    }
    destroy_huffman_tree(root->left);
    destroy_huffman_tree(root->right);
    free(root);
}

static bool fill_min_heap(min_heap_t *heap, huffman_tree_t **node, uint16_t active_size) {
    for (uint16_t i = 0; i < active_size; i++) {
        if (!min_heap_insert(heap, node[i]))
            return false;
        node[i] = NULL;
    }
    return true;
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
