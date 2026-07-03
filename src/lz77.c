#include "../include/lz77.h"
#include "../include/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static match_t find_match(byte *data, uint32_t data_len, int32_t data_pointer,
                          hash_chain_t *hash_chain);
static void free_lz77_tokens(lz_token_t *tokens);
static inline size_t hash3(const byte *data, uint32_t pos, uint32_t data_len);
static inline void hash_insert(hash_chain_t *hash_chain, const byte *data, uint32_t pos,
                               uint32_t data_len);
static inline lz_token_t create_literal_token(const byte *data, size_t data_pointer);
static inline lz_token_t create_match_token(size_t data_pointer, match_t match);
static inline void add_eob_token(lz_token_t *tokens, size_t tokens_size);

lz_tokens_stream_t *lz_77_encode(byte *data, uint32_t data_len) {
    if (NULL == data) {
        FATAL("data passed to the lz_77_encode is NULL");
        return NULL;
    }
    if (data_len == 0) {
        FATAL("data_len of passed data is 0");
        return NULL;
    }

    lz_token_t *tokens = malloc(sizeof(lz_token_t) * (data_len + 1));
    if (NULL == tokens) {
        FATAL("failed to allocate memory for the LZ77 tokens");
        return NULL;
    }

    hash_chain_t *hash_chain = malloc(sizeof(hash_chain_t));
    if (NULL == hash_chain) {
        FATAL("failed to allocate memory for hash chain");
        free(tokens);
        return NULL;
    }

    hash_chain->prev = malloc(sizeof(int) * data_len);
    if (NULL == hash_chain->prev) {
        FATAL("failed to allocate memory for chain prev");
        free(tokens);
        free(hash_chain);
        return NULL;
    }
    memset(hash_chain->head, HASH_INIT_VALUE, sizeof(hash_chain->head));
    memset(hash_chain->prev, HASH_INIT_VALUE, sizeof(int) * data_len);

    uint32_t data_pointer = 0;
    uint32_t tokens_size  = 0;

    while (data_pointer < data_len) {
        if (data_pointer + MIN_MATCH_LENGTH <= data_len) {
            match_t match = find_match(data, data_len, (int)data_pointer, hash_chain);
            if (match.longest_match_len == 0) {
                // no match found
                tokens[tokens_size++] = create_literal_token(data, data_pointer);
                hash_insert(hash_chain, data, data_pointer, data_len);
                ++data_pointer;
            } else {
                // match founded
                tokens[tokens_size++] = create_match_token(data_pointer, match);
                for (size_t i = 0; i < match.longest_match_len; i++) {
                    hash_insert(hash_chain, data, data_pointer + i, data_len);
                }
                data_pointer += match.longest_match_len;
            }
        } else {
            // we are at the final x byte/bytes of data
            // x < MIN_MATCH_LENGTH therefor all of the tokens will be process as literal
            tokens[tokens_size++] = create_literal_token(data, data_pointer);
            ++data_pointer;
        }
    }

    add_eob_token(tokens, tokens_size);

    lz_tokens_stream_t *tokens_stream = malloc(sizeof(lz_tokens_stream_t));
    if (NULL == tokens_stream) {
        FATAL("failed to allocate memory for token stream");
        free_lz77_tokens(tokens);
        free(hash_chain->prev);
        free(hash_chain);
        return NULL;
    }

    tokens_stream->tokens = tokens;
    tokens_stream->size   = tokens_size + 1;
    free(hash_chain->prev);
    free(hash_chain);
    return tokens_stream;
}

byte *lz_77_decode(const lz_tokens_stream_t *lz77_stream) {
    if (NULL == lz77_stream) {
        FATAL("lz77 token stream is NULL for decoding");
    }
    uint32_t allocated_size = lz77_stream->size;
    byte *decoded_data      = malloc(sizeof(byte) * allocated_size);
    if (NULL == decoded_data) {
        FATAL("failed to allocate decoded data");
        return NULL;
    }
    for (uint32_t data_index = 0, lz_index = 0; lz_index < lz77_stream->size;
         lz_index++, data_index++) {
        if (lz77_stream->tokens[lz_index].type == LITERAL) {
            if (data_index >= allocated_size) {
                byte *temp = realloc(decoded_data, allocated_size * 2);
                if (NULL == temp) {
                    FATAL("failed to realloc for decoded data in LITERL in decoding");
                    free(decoded_data);
                    return NULL;
                }
                decoded_data = temp;
                allocated_size *= 2;
            }
            decoded_data[data_index] = lz77_stream->tokens[lz_index].literal;
        } else if (lz77_stream->tokens[lz_index].type == MATCH) {
            if (lz77_stream->tokens[lz_index].length > MAX_MATCH_LENGTH) {
                FATAL("length is bigger than max match length");
                printf("fuck");
                free(decoded_data);
                return NULL;
            }
            if (data_index + lz77_stream->tokens[lz_index].length >= allocated_size - 1) {
                byte *temp =
                    realloc(decoded_data,
                            allocated_size + (sizeof(byte) * lz77_stream->tokens[lz_index].length));
                if (NULL == temp) {
                    FATAL("failed to reallocate memory for decoding lz77 tokens");
                    free(decoded_data);
                    return NULL;
                }
                decoded_data = temp;
                allocated_size += lz77_stream->tokens[lz_index].length;
            }
            if (lz77_stream->tokens[lz_index].distance <= 0 ||
                lz77_stream->tokens[lz_index].distance > data_index) {
                FATAL("corrupt lz token");
                free(decoded_data);
                return NULL;
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

static match_t find_match(byte *data, uint32_t data_len, int32_t data_pointer,
                          hash_chain_t *hash_chain) {
    uint32_t longest_match_len   = 0;
    uint32_t longest_match_index = 0;
    uint32_t hash                = hash3(data, data_pointer, data_len);
    int32_t candidate            = hash_chain->head[hash];
    uint32_t chain_length        = 0;
    // just iterating through the candidate (hash that represent data (in SEARCH BUFFER) that
    // matched (MIN_MATCH_LENGTH = 3) bytes with the match data
    while (candidate != HASH_INIT_VALUE && chain_length < MAX_CHAIN_LENGTH) {
        chain_length++;
        uint32_t match_index        = candidate;
        uint32_t data_pointer_index = data_pointer;
        uint32_t temp_match_len     = 0;

        if (data_pointer - candidate > SEARCH_BUFFER_SIZE || candidate >= data_pointer) {
            candidate = hash_chain->prev[candidate];
            continue;
        }

        while (data_pointer_index < data_len && match_index < data_len &&
               temp_match_len < MAX_MATCH_LENGTH && data[match_index] == data[data_pointer_index]) {
            match_index++;
            data_pointer_index++;
            temp_match_len++;
        }

        if (temp_match_len >= MIN_MATCH_LENGTH && temp_match_len > longest_match_len) {
            longest_match_len   = temp_match_len;
            longest_match_index = candidate;
            if (longest_match_len == MAX_MATCH_LENGTH)
                break;
        }

        candidate = hash_chain->prev[candidate];
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

void free_lz77(lz_tokens_stream_t *token_stream) {
    if (!token_stream) {
        return;
    }
    free(token_stream->tokens);
    free(token_stream);
}

static void free_lz77_tokens(lz_token_t *tokens) { free(tokens); }

static inline void hash_insert(hash_chain_t *hash_chain, const byte *data, uint32_t pos,
                               uint32_t data_len) {
    if (pos + MIN_MATCH_LENGTH > data_len)
        return;
    uint32_t hash          = hash3(data, pos, data_len);
    hash_chain->prev[pos]  = hash_chain->head[hash];
    hash_chain->head[hash] = (int)pos;
}

static inline size_t hash3(const byte *data, uint32_t pos, uint32_t data_len) {
    if (pos + MIN_MATCH_LENGTH > data_len) {
        return 0;
    }
    size_t hash = data[pos];

    hash = ((hash << HASH_SHIFT) ^ data[pos + 1]) & HASH_MASK;
    hash = ((hash << HASH_SHIFT) ^ data[pos + 2]) & HASH_MASK;

    return hash;
}

static inline void add_eob_token(lz_token_t *tokens, size_t tokens_size) {
    lz_token_t token;
    token.type          = EOB_TOKEN;
    token.distance      = 0;
    token.length        = 0;
    tokens[tokens_size] = token;
}

static inline lz_token_t create_literal_token(const byte *data, size_t data_pointer) {
    lz_token_t token;
    token.type     = LITERAL;
    token.literal  = data[data_pointer];
    token.distance = 0;
    token.length   = 0;
    return token;
}
static inline lz_token_t create_match_token(size_t data_pointer, match_t match) {
    lz_token_t token;
    token.type     = MATCH;
    token.literal  = 0;
    token.distance = data_pointer - match.longest_match_index;
    token.length   = match.longest_match_len;
    return token;
}
