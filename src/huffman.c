#include "../include/huffman.h"
#include "../include/deflate.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint16_t get_active_literal_length_symbol_count(frequency_t *frequency);
uint8_t get_active_distance_symbol(frequency_t *frequency);

void initialize_leaves_array_from_frequency(frequency_t *frequency) {
    uint16_t literal_length_active_symbol = get_active_literal_length_symbol_count(frequency);
    uint8_t  distance_active_symbol       = get_active_distance_symbol(frequency);

    huffman_tree_t *literal_length_leaves = malloc(sizeof(huffman_tree_t) * literal_length_active_symbol);
    if (NULL == literal_length_leaves) {
        FATAL("failed to allocated memory for literal length leaves");
        return;
    }
    huffman_tree_t *distance_leaves = malloc(sizeof(huffman_tree_t) * distance_active_symbol);
    if (NULL == distance_leaves) {
        FATAL("failed to allocated memory for distance leaves");
        return;
    }
    for (uint16_t i = 0, j = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            literal_length_leaves[j].frequency = frequency->literal_length_frequency[i];
            literal_length_leaves[j].symbol = i;
            literal_length_leaves[j].left = NULL;
            literal_length_leaves[j].right = NULL;
            ++j;
        }
    }
    for (uint8_t i = 0, j = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            distance_leaves[j].frequency = frequency->distance_frequency[i];
            distance_leaves[j].symbol = i;
            distance_leaves[j].left = NULL;
            distance_leaves[j].right = NULL;
            ++j;
        }
    }
}

frequency_t initialize_frequency_table(deflate_stream_t *deflate_stream) {
    frequency_t frequency = {.distance_frequency = NULL, .literal_length_frequency = NULL};
    if (NULL == deflate_stream) {
        FATAL("failed to initialize frequency table; deflate stream was NULL");
        return frequency;
    }
    uint16_t *literal_and_length_frequency = malloc(sizeof(uint16_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    if (NULL == literal_and_length_frequency) {
        FATAL("failed to allocated memory for literal and length frequency");
        return frequency;
    }
    memset(literal_and_length_frequency, 0, sizeof(uint16_t) * DEFLATE_LITERAL_LENGTH_SIZE);
    uint16_t *distance_frequency = malloc(sizeof(uint16_t) * DEFLATE_DISTANCE_SIZE);
    if (NULL == distance_frequency) {
        FATAL("failed to allocated memory for distance frequency");
        return frequency;
    }
    memset(distance_frequency, 0, sizeof(uint16_t) * DEFLATE_DISTANCE_SIZE);
    for (uint32_t i = 0; i < deflate_stream->size; i++) {
        if (deflate_stream->tokens[i].type == LITERAL) {
            literal_and_length_frequency[deflate_stream->tokens[i].literal]++;
        }else if (deflate_stream->tokens[i].type == MATCH) {
            distance_frequency[deflate_stream->tokens[i].distance_symbol]++;
            literal_and_length_frequency[deflate_stream->tokens[i].length_symbol]++;
        }else {
            literal_and_length_frequency[EOB]++;
        }
    }
    frequency.distance_frequency = distance_frequency;
    frequency.literal_length_frequency = literal_and_length_frequency;
    return frequency;
}

uint16_t get_active_literal_length_symbol_count(frequency_t *frequency) {
    uint16_t literal_length_active_symbol = 0;
    for (uint16_t i = 0; i < DEFLATE_LITERAL_LENGTH_SIZE; i++) {
        if (frequency->literal_length_frequency[i] != 0) {
            ++literal_length_active_symbol;
        }
    }
    return literal_length_active_symbol;
}

uint8_t get_active_distance_symbol(frequency_t *frequency) {
    uint8_t distance_active_symbol = 0;
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_SIZE; i++) {
        if (frequency->distance_frequency[i] != 0) {
            ++distance_active_symbol;
        }
    }
    return distance_active_symbol;
}
