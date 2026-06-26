#include "../include/huffman.h"
#include "../include/deflate.h"
#include "../include/min_heap.h"
#include <cstdint>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint16_t get_active_literal_length_symbol_count(const frequency_t *frequency);
uint8_t get_active_distance_symbol(const frequency_t *frequency);
frequency_t initialize_frequency_table(deflate_stream_t *deflate_stream);
huffman_init_leaves *initialize_leaves_array_from_frequency(const frequency_t *frequency);
void build_huffman_len_code(huffman_tree_t *root, huffman_code_t *code, uint16_t len);

void build_huffman_tree(deflate_stream_t *deflate) {
    frequency_t frequency = initialize_frequency_table(deflate);
    if (NULL == frequency.distance_frequency) {
        return;
    }
    if (NULL == frequency.literal_length_frequency) {
        return;
    }

    huffman_init_leaves *init_leaves = initialize_leaves_array_from_frequency(&frequency);
    if (NULL == init_leaves) {
        return;
    }
    if (NULL == init_leaves->literal_len_leaves) {
        return;
    }
    if (NULL == init_leaves->distance_leaves) {
        return;
    }

    min_heap_t *min_heap_literal_len = min_heap_create(init_leaves->literal_len_active_symbol);
    if (NULL == min_heap_literal_len) {
        return;
    }

    min_heap_t *min_heap_distance = min_heap_create(init_leaves->distance_active_symbol);
    if (NULL == min_heap_distance) {
        return;
    }

    for (uint16_t i = 0; i < init_leaves->literal_len_active_symbol; i++) {
        min_heap_insert(min_heap_literal_len, &init_leaves->literal_len_leaves[i]);
    }

    for (uint8_t i = 0; i < init_leaves->distance_active_symbol; i++) {
        min_heap_insert(min_heap_distance, &init_leaves->distance_leaves[i]);
    }

    while (min_heap_literal_len->size > 1) {
        huffman_tree_t *left   = min_heap_get_min(min_heap_literal_len);
        huffman_tree_t *right  = min_heap_get_min(min_heap_literal_len);
        huffman_tree_t *parent = malloc(sizeof(huffman_tree_t));
        if (NULL == parent) {
            FATAL("failed to allocated memory for huffman parent node");
            return;
        }
        parent->frequency = left->frequency + right->frequency;
        parent->symbol    = 0;
        parent->left      = left;
        parent->right     = right;
        min_heap_insert(min_heap_literal_len, parent);
    }
    while (min_heap_distance->size > 1) {
        huffman_tree_t *left   = min_heap_get_min(min_heap_distance);
        huffman_tree_t *right  = min_heap_get_min(min_heap_distance);
        huffman_tree_t *parent = malloc(sizeof(huffman_tree_t));
        if (NULL == parent) {
            FATAL("failed to allocated memory for huffman parent node");
            return;
        }
        parent->frequency = left->frequency + right->frequency;
        parent->symbol    = 0;
        parent->left      = left;
        parent->right     = right;
        min_heap_insert(min_heap_distance, parent);
    }
    huffman_tree_t *literal_len_root = min_heap_get_min(min_heap_literal_len);
    if (NULL == literal_len_root) {
        return;
    }
    huffman_tree_t *distance_root = min_heap_get_min(min_heap_distance);
    if (NULL == distance_root) {
        return;
    }
    huffman_code_t *literal_len_codes =
        malloc(sizeof(huffman_code_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    if (NULL == literal_len_codes) {
        FATAL("failed to allocated memory for literal and length code's len type");
        return;
    }
    huffman_code_t *distance_codes = malloc(sizeof(huffman_code_t) * DEFLATE_DISTANCE_SIZE);
    if (NULL == distance_codes) {
        FATAL("failed to allocated memory for distance code's len type");
        return;
    }

    memset(literal_len_codes, 0, sizeof(huffman_code_t) * DEFLATE_LITERAL_LENGTH_SIZE);

    memset(distance_codes, 0, sizeof(huffman_code_t) * DEFLATE_DISTANCE_SIZE);

    build_huffman_len_code(literal_len_root, literal_len_codes, 0);
    build_huffman_len_code(distance_root, distance_codes, 0);
    huffman_code_t literal_len_active_codes[init_leaves->literal_len_active_symbol];
    huffman_code_t distance_active_codes[init_leaves->distance_active_symbol];
    for (uint16_t i = 0, j = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (literal_len_codes[i].length != 0) {
            literal_len_active_codes[j++] = literal_len_codes[i];
        }
    }
    for (uint16_t i = 0, j = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (distance_codes[i].length != 0) {
            distance_active_codes[j++] = distance_codes[i];
        }
    }
    for (uint16_t i = 0; i < init_leaves->literal_len_active_symbol; i++) {
        uint16_t size = i;
        while (size > 0) {
            if (literal_len_active_codes[size].length < literal_len_active_codes[size - 1].length ||
                (literal_len_active_codes[size].length ==
                     literal_len_active_codes[size - 1].length &&
                 literal_len_active_codes[size].symbol <
                     literal_len_active_codes[size - 1].symbol)) {
                huffman_code_t temp                = literal_len_active_codes[size];
                literal_len_active_codes[size]     = literal_len_active_codes[size - 1];
                literal_len_active_codes[size - 1] = temp;
                --size;
            } else {
                break;
            }
        }
    }

    for (uint16_t i = 0; i < init_leaves->distance_active_symbol; i++) {
        uint16_t size = i;
        while (size > 0) {
            if (distance_active_codes[size].length < distance_active_codes[size - 1].length ||
                (distance_active_codes[size].length == distance_active_codes[size - 1].length &&
                 distance_active_codes[size].symbol < distance_active_codes[size - 1].symbol)) {
                huffman_code_t temp             = distance_active_codes[size];
                distance_active_codes[size]     = distance_active_codes[size - 1];
                distance_active_codes[size - 1] = temp;
                --size;
            } else {
                break;
            }
        }
    }
}

void build_huffman_len_code(huffman_tree_t *root, huffman_code_t *code, uint16_t len) {
    if (NULL == root) {
        return;
    }
    if (NULL == code) {
        return;
    }
    build_huffman_len_code(root->left, code, len + 1);
    build_huffman_len_code(root->right, code, len + 1);
    if (NULL == root->right && NULL == root->left) {
        code[root->symbol].length = len;
        code[root->symbol].symbol = root->symbol;
    }
}

huffman_init_leaves *initialize_leaves_array_from_frequency(const frequency_t *frequency) {
    uint16_t literal_length_active_symbol = get_active_literal_length_symbol_count(frequency);
    uint8_t distance_active_symbol        = get_active_distance_symbol(frequency);

    huffman_tree_t *literal_length_leaves =
        malloc(sizeof(huffman_tree_t) * literal_length_active_symbol);
    if (NULL == literal_length_leaves) {
        FATAL("failed to allocated memory for literal length leaves");
        return NULL;
    }
    huffman_tree_t *distance_leaves = malloc(sizeof(huffman_tree_t) * distance_active_symbol);
    if (NULL == distance_leaves) {
        FATAL("failed to allocated memory for distance leaves");
        free(literal_length_leaves);
        return NULL;
    }
    for (uint16_t i = 0, j = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            literal_length_leaves[j].frequency = frequency->literal_length_frequency[i];
            literal_length_leaves[j].symbol    = i;
            literal_length_leaves[j].left      = NULL;
            literal_length_leaves[j].right     = NULL;
            ++j;
        }
    }
    for (uint8_t i = 0, j = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            distance_leaves[j].frequency = frequency->distance_frequency[i];
            distance_leaves[j].symbol    = i;
            distance_leaves[j].left      = NULL;
            distance_leaves[j].right     = NULL;
            ++j;
        }
    }
    huffman_init_leaves *leaves = malloc(sizeof(huffman_init_leaves));
    if (NULL == leaves) {
        free(literal_length_leaves);
        free(distance_leaves);
        FATAL("failed to allocate memory for initial huffman leaves");
        return NULL;
    }
    leaves->literal_len_leaves        = literal_length_leaves;
    leaves->literal_len_active_symbol = literal_length_active_symbol;
    leaves->distance_leaves           = distance_leaves;
    leaves->distance_active_symbol    = distance_active_symbol;
    return leaves;
}

frequency_t initialize_frequency_table(deflate_stream_t *deflate_stream) {
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
        return frequency;
    }
    memset(distance_frequency, 0, sizeof(uint32_t) * DEFLATE_DISTANCE_SIZE);
    for (uint32_t i = 0; i < deflate_stream->size; i++) {
        if (deflate_stream->tokens[i].type == LITERAL) {
            literal_and_length_frequency[deflate_stream->tokens[i].literal]++;
        } else if (deflate_stream->tokens[i].type == MATCH) {
            distance_frequency[deflate_stream->tokens[i].distance_symbol]++;
            literal_and_length_frequency[deflate_stream->tokens[i].length_symbol]++;
        } else {
            literal_and_length_frequency[EOB]++;
        }
    }
    frequency.distance_frequency       = distance_frequency;
    frequency.literal_length_frequency = literal_and_length_frequency;
    return frequency;
}

uint16_t get_active_literal_length_symbol_count(const frequency_t *frequency) {
    uint16_t literal_length_active_symbol = 0;
    for (uint16_t i = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            ++literal_length_active_symbol;
        }
    }
    return literal_length_active_symbol;
}

uint8_t get_active_distance_symbol(const frequency_t *frequency) {
    uint8_t distance_active_symbol = 0;
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            ++distance_active_symbol;
        }
    }
    return distance_active_symbol;
}
