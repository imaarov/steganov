#include "../include/utils.h"
#include "../include/lz77.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>


match_t find_match(Byte *data, uint32_t data_len, uint32_t pointer);
void print_lz77_tokens(lz_token_t *tokens, uint32_t tokens_size);
void lz_77_encode(Byte *data, uint32_t data_len);

int main() {
    Byte *data = (Byte *)"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    uint16_t data_len = strlen((char *)data);
    lz_77_encode(data, data_len);
    return 0;
}

void lz_77_encode(Byte *data, uint32_t data_len) {
    if (NULL == data) {
        FATAL("data passed to the lz_77_encode is NULL");
        return;
    }
    if (data_len == 0) {
        FATAL("data_len of passed data is 0");
        return;
    }

    uint32_t pointer = 0; // point to the first element
                          // of the data
                          // then we go back with the data_len of
                          // SEARCH_BUFFER_data_len' elements
                          // we search for the character that pointer 
                          // pointed to
                          // if we have match or not we move the pointer
                          // until its the end of the data
    lz_token_t *tokens = malloc((sizeof(lz_token_t) * data_len) + 1);
    uint32_t tokens_size = 0;
    if (NULL == tokens) {
        FATAL("failed to allocate memory for the LZ77 tokens");
        return;
    }
    while (pointer < data_len) {
        match_t match = find_match(data, data_len, pointer);
        lz_token_t token;
        if (match.longest_match_len == 0) {
            // no match found
            token.type    = LITERAL;
            token.literal = data[pointer];
            token.distance= 0;
            token.length  = 0;
            tokens[tokens_size++] = token;
            ++pointer;
        }else {
            // match founded
            token.type    = MATCH;
            token.distance= pointer - match.longest_match_index;
            token.length  = match.longest_match_len;
            tokens[tokens_size++] = token;
            pointer += match.longest_match_len; // i dont check if its 
                                                // going over the boundary of
                                                // data, cause after this its
                                                // going to check the pointer 
                                                // in while condition
        }
    }

    print_lz77_tokens(tokens, tokens_size);
}

match_t find_match(Byte *data, uint32_t data_len, uint32_t pointer) {
    uint32_t search_buffer_index_start = pointer < SEARCH_BUFFER_SIZE ? 0 : pointer - SEARCH_BUFFER_SIZE;
    uint32_t longest_match_len   = 0;
    uint32_t longest_match_index = 0;

    for (uint32_t i = search_buffer_index_start; 
            i < pointer; 
            i++) {
        if (data[pointer] == data[i]) {
            uint32_t temp_match_len = 0;
            // temp variables for keep moving next to find the longest match
            uint32_t match_index = i;
            uint32_t pointer_index = pointer;
            while (data[match_index] == data[pointer_index] &&
                    pointer_index < data_len && 
                    temp_match_len < MAX_MATCH_LENGTH) {
                ++match_index;
                ++pointer_index;
                ++temp_match_len;
            }
            if (temp_match_len > longest_match_len) {
                longest_match_len   = temp_match_len;
                longest_match_index = i;
            }
        }else {
            continue;
        }
    }
    if (longest_match_len < MIN_MATCH_LENGTH) {
        longest_match_len   = 0;
        longest_match_index = 0;
    }
    return (match_t) {.longest_match_index = longest_match_index, .longest_match_len = longest_match_len};
}

void print_lz77_tokens(lz_token_t *tokens, uint32_t tokens_size) {
    for (uint32_t i = 0; i < tokens_size; i++) {
        if (tokens[i].type == LITERAL) {
            printf("[LITERAL] %c\n", (unsigned char)tokens[i].literal);
        }else {
            printf("[MATCH] D %u, L %u\n", tokens[i].distance, tokens[i].length);
        }
    }
}
