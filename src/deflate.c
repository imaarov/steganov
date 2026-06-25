#include "../include/deflate.h"
#include "../include/lz77.h"
#include <stdint.h>
#include <stdlib.h>

static length_code_t get_deflate_length_symbol(uint16_t length);
static distance_code_t get_deflate_distance_symbol(uint16_t distance);

deflate_stream_t *deflate_encode(const lz_tokens_stream_t *token_stream) {
    if (NULL == token_stream) {
        FATAL("lz77 token stream is NULL");
        return NULL;
    }

    deflate_token_t *deflate = malloc(sizeof(deflate_token_t) * token_stream->size);
    if (NULL == deflate) {
        FATAL("cannot allocate memory for deflate tokens");
        return NULL;
    }
    for (uint32_t i = 0; i < token_stream->size; i++) {
        if (token_stream->tokens[i].type == LITERAL) {
            deflate[i].type = LITERAL;
            deflate[i].literal             = token_stream->tokens[i].literal;
            deflate[i].length_symbol       = 0;
            deflate[i].length_extra_value  = 0;
            deflate[i].length_extra_count  = 0;
            deflate[i].distance_symbol     = 0;
            deflate[i].distance_extra_value= 0;
            deflate[i].distance_extra_count= 0;
        }else if (token_stream->tokens[i].type == MATCH) {
            deflate[i].type    = MATCH;
            deflate[i].literal = 0;
            length_code_t length_code = get_deflate_length_symbol(token_stream->tokens[i].length);
            distance_code_t distance_code = get_deflate_distance_symbol(token_stream->tokens[i].distance);
            if (length_code.min_length == INVALID_DEFLATE_MIN_CODE || distance_code.min_length == INVALID_DEFLATE_MIN_CODE) {
                free(deflate);
                return NULL;
            }
            deflate[i].length_symbol        = length_code.symbol;
            deflate[i].length_extra_value   = token_stream->tokens[i].length - length_code.min_length;
            deflate[i].length_extra_count   = length_code.extra_bits;
            deflate[i].distance_symbol      = distance_code.symbol;
            deflate[i].distance_extra_value = token_stream->tokens[i].distance - distance_code.min_length;
            deflate[i].distance_extra_count = distance_code.extra_bits;
        }else {
            deflate[i].type                 = EOB_TOKEN;
            deflate[i].literal              = EOB;
            deflate[i].length_symbol        = 0;
            deflate[i].length_extra_value   = 0;
            deflate[i].length_extra_count   = 0;
            deflate[i].distance_symbol      = 0;
            deflate[i].distance_extra_value = 0;
            deflate[i].distance_extra_count = 0;
        }
    }
    deflate_stream_t *deflate_stream = malloc(sizeof(deflate_stream_t));
    if (NULL == deflate_stream) {
        FATAL("failed to allocate defalte stream");
        free(deflate);
        return NULL;
    }
    deflate_stream->tokens = deflate;
    deflate_stream->size   = token_stream->size;
    return deflate_stream;
}

void free_deflate_stream(deflate_stream_t *deflate_stream) {
    if (NULL == deflate_stream) {
        return;
    }
    free(deflate_stream->tokens);
    free(deflate_stream);
}

static length_code_t get_deflate_length_symbol(uint16_t length) {
    for (uint8_t i = 0; i < DEFLATE_LENGTH_CODE_TABLE_SIZE; i++) {
        if (length >= LENGTH_CODES[i].min_length && length <= LENGTH_CODES[i].max_length) {
            return LENGTH_CODES[i];
        }
    }
    FATAL("invalid length");
    return (length_code_t) {.symbol = 0, .min_length = INVALID_DEFLATE_MIN_CODE, .max_length = 0, .extra_bits = 0};
}

static distance_code_t get_deflate_distance_symbol(uint16_t distance) {
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_CODE_TABLE_SIZE; i++) {
        if (distance >= DISTANCE_CODES[i].min_length && distance <= DISTANCE_CODES[i].max_length) {
            return DISTANCE_CODES[i];
        }
    }
    return (distance_code_t) {.symbol = 0, .min_length = INVALID_DEFLATE_MIN_CODE, .max_length = 0, .extra_bits = 0};
}
