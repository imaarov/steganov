#include "../include/lz77.h"
#include "../include/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static match_t find_match(Byte *data, uint32_t data_len, uint32_t pointer);
static void free_lz77_tokens(lz_token_t *tokens);

lz_tokens_stream_t *lz_77_encode(Byte *data, uint32_t data_len) {
    if (NULL == data) {
        FATAL("data passed to the lz_77_encode is NULL");
        return NULL;
    }
    if (data_len == 0) {
        FATAL("data_len of passed data is 0");
        return NULL;
    }

    uint32_t pointer = 0; // point to the first element
                          // of the data
    lz_token_t *tokens   = malloc(sizeof(lz_token_t) * (data_len + 1));
    uint32_t tokens_size = 0;
    if (NULL == tokens) {
        FATAL("failed to allocate memory for the LZ77 tokens");
        return NULL;
    }
    while (pointer < data_len) {
        match_t match = find_match(data, data_len, pointer);
        lz_token_t token;
        if (match.longest_match_len == 0) {
            // no match found
            token.type            = LITERAL;
            token.literal         = data[pointer];
            token.distance        = 0;
            token.length          = 0;
            tokens[tokens_size++] = token;
            ++pointer;
        } else {
            // match founded
            token.type            = MATCH;
            token.distance        = pointer - match.longest_match_index;
            token.length          = match.longest_match_len;
            tokens[tokens_size++] = token;
            pointer += match.longest_match_len;
        }
    }

    lz_token_t token;
    token.type                        = EOB_TOKEN;
    token.distance                    = 0;
    token.length                      = 0;
    tokens[tokens_size]               = token;
    lz_tokens_stream_t *tokens_stream = malloc(sizeof(lz_tokens_stream_t));
    if (NULL == tokens_stream) {
        FATAL("failed to allocate memory for token stream");
        free_lz77_tokens(tokens);
        return NULL;
    }
    tokens_stream->tokens = tokens;
    tokens_stream->size   = tokens_size + 1;
    return tokens_stream;
}

Byte *lz_77_decode(const lz_tokens_stream_t *lz77_stream) {
    if (NULL == lz77_stream) {
        FATAL("lz77 token stream is NULL for decoding");
    }
    uint32_t allocated_size = lz77_stream->size; // initial size
    Byte *decoded_data      = malloc(sizeof(Byte) * allocated_size);
    for (uint32_t data_index = 0, lz_index = 0; lz_index < lz77_stream->size;
         lz_index++, data_index++) {
        if (lz77_stream->tokens[lz_index].type == LITERAL) {
            decoded_data[data_index] = lz77_stream->tokens[lz_index].literal;
        } else if (lz77_stream->tokens[lz_index].type == MATCH) {
            if (lz77_stream->tokens[lz_index].length > 258) {
                FATAL("length is bigger than max match length");
                free(decoded_data);
                return NULL;
            }
            if (data_index + lz77_stream->tokens[lz_index].length > allocated_size) {
                Byte *temp =
                    realloc(decoded_data,
                            allocated_size + (sizeof(Byte) * lz77_stream->tokens[lz_index].length));
                if (NULL == temp) {
                    FATAL("failed to reallocate memory for decoding lz77 tokens");
                    return decoded_data;
                }
                decoded_data = temp;
                allocated_size += lz77_stream->tokens[lz_index].length;
            }
            for (uint32_t match_begin = data_index - lz77_stream->tokens[lz_index].distance,
                          len         = 0;
                 len < lz77_stream->tokens[lz_index].length; match_begin++, len++) {
                if (match_begin > (data_index - 1)) {
                    match_begin = data_index - lz77_stream->tokens[lz_index].distance;
                }
                decoded_data[data_index++] = decoded_data[match_begin];
            }
            data_index -= 1;
        }
    }
    return decoded_data;
}

static match_t find_match(Byte *data, uint32_t data_len, uint32_t pointer) {
    uint32_t search_buffer_index_start =
        pointer < SEARCH_BUFFER_SIZE ? 0 : pointer - SEARCH_BUFFER_SIZE;
    uint32_t longest_match_len   = 0;
    uint32_t longest_match_index = 0;

    for (uint32_t i = search_buffer_index_start; i < pointer; i++) {
        if (data[pointer] == data[i]) {
            uint32_t temp_match_len = 0;
            // temp variables for keep moving next to find the longest match
            uint32_t match_index   = i;
            uint32_t pointer_index = pointer;
            while (pointer_index < data_len && match_index < data_len &&
                   data[match_index] == data[pointer_index] && temp_match_len < MAX_MATCH_LENGTH) {
                ++match_index;
                ++pointer_index;
                ++temp_match_len;
            }
            if (temp_match_len > longest_match_len) {
                longest_match_len   = temp_match_len;
                longest_match_index = i;
            }
        }
    }
    if (longest_match_len < MIN_MATCH_LENGTH) {
        longest_match_len   = 0;
        longest_match_index = 0;
    }
    return (match_t){.longest_match_index = longest_match_index,
                     .longest_match_len   = longest_match_len};
}

void print_lz77_tokens(lz_tokens_stream_t *stream, uint32_t tokens_size) {
    for (uint32_t i = 0; i < tokens_size; i++) {
        if (stream->tokens[i].type == LITERAL) {
            printf("[LITERAL] %c\n", (unsigned char)stream->tokens[i].literal);
        } else if (stream->tokens[i].type == MATCH) {
            printf("[MATCH] D %u, L %u\n", stream->tokens[i].distance, stream->tokens[i].length);
        } else {
            printf("[EOB]\n");
        }
    }
}

static void free_lz77_tokens(lz_token_t *tokens) { free(tokens); }

void free_lz77(lz_tokens_stream_t *token_stream) {
    free(token_stream);
}
