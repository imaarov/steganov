/**
 *              DEFLATE transformer header conventions:
 *              ---------------------------------------
 *              symbol/literal range : 0-255
 *              end of block(EOB)    : 256
 *              length range         : 257-285
 *              distance range       : 1-32768
 */
#ifndef DEFLATE_LZ77_H
#define DEFLATE_LZ77_H

#include "lz77.h"
#include <stdint.h>

#define DEFLATE_LENGTH_CODE_TABLE_SIZE   29
#define DEFLATE_DISTANCE_CODE_TABLE_SIZE 30
#define INVALID_DEFLATE_MIN_CODE         0

typedef struct {
    uint16_t symbol;
    uint16_t min_length;
    uint16_t max_length;
    uint8_t extra_bits;
} length_code_t;

typedef struct {
    uint8_t symbol;
    uint16_t min_length;
    uint16_t max_length;
    uint8_t extra_bits;
} distance_code_t;

typedef struct {
    token_type type;
    uint16_t literal;
    uint16_t length_symbol;
    uint16_t length_extra_value;
    uint8_t length_extra_count;
    uint16_t distance_symbol;
    uint16_t distance_extra_value;
    uint8_t distance_extra_count;
} deflate_lz77_token_t;

typedef struct {
    deflate_lz77_token_t *tokens;
    uint32_t size;
} deflate_lz77_stream_t;

deflate_lz77_stream_t *deflate_lz77_encode(const lz_tokens_stream_t *token_stream);
void free_deflate_lz77_stream(deflate_lz77_stream_t *deflate_lz77_stream);
void print_deflate_lz77(const deflate_lz77_stream_t *deflate_lz77_stream);

// clang-format off
static const length_code_t LENGTH_CODES[DEFLATE_LENGTH_CODE_TABLE_SIZE] = {
    {257,   3,   3, 0},
    {258,   4,   4, 0},
    {259,   5,   5, 0},
    {260,   6,   6, 0},
    {261,   7,   7, 0},
    {262,   8,   8, 0},
    {263,   9,   9, 0},
    {264,  10,  10, 0},

    {265,  11,  12, 1},
    {266,  13,  14, 1},
    {267,  15,  16, 1},
    {268,  17,  18, 1},

    {269,  19,  22, 2},
    {270,  23,  26, 2},
    {271,  27,  30, 2},
    {272,  31,  34, 2},

    {273,  35,  42, 3},
    {274,  43,  50, 3},
    {275,  51,  58, 3},
    {276,  59,  66, 3},

    {277,  67,  82, 4},
    {278,  83,  98, 4},
    {279,  99, 114, 4},
    {280, 115, 130, 4},

    {281, 131, 162, 5},
    {282, 163, 194, 5},
    {283, 195, 226, 5},
    {284, 227, 257, 5},

    {285, 258, 258, 0}
};

static const distance_code_t DISTANCE_CODES[DEFLATE_DISTANCE_CODE_TABLE_SIZE] = {
    { 0,     1,     1,  0},
    { 1,     2,     2,  0},
    { 2,     3,     3,  0},
    { 3,     4,     4,  0},

    { 4,     5,     6,  1},
    { 5,     7,     8,  1},

    { 6,     9,    12,  2},
    { 7,    13,    16,  2},

    { 8,    17,    24,  3},
    { 9,    25,    32,  3},

    {10,    33,    48,  4},
    {11,    49,    64,  4},

    {12,    65,    96,  5},
    {13,    97,   128,  5},

    {14,   129,   192,  6},
    {15,   193,   256,  6},

    {16,   257,   384,  7},
    {17,   385,   512,  7},

    {18,   513,   768,  8},
    {19,   769,  1024,  8},

    {20,  1025,  1536,  9},
    {21,  1537,  2048,  9},

    {22,  2049,  3072, 10},
    {23,  3073,  4096, 10},

    {24,  4097,  6144, 11},
    {25,  6145,  8192, 11},

    {26,  8193, 12288, 12},
    {27, 12289, 16384, 12},

    {28, 16385, 24576, 13},
    {29, 24577, 32768, 13}
};
// clang-format on

#endif
