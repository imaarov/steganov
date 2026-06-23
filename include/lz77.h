#ifndef LZ77
#define LZ77

#include "utils.h"
#include <stdint.h>

#define SEARCH_BUFFER_SIZE     32768
#define LOOK_AHEAD_BUFFER_SIZE 258
#define MIN_MATCH_LENGTH       3
#define MAX_MATCH_LENGTH       258
#define WINDOW_SIZE            SEARCH_BUFFER_SIZE + LOOK_AHEAD_BUFFER_SIZE
#define EOB                    256

typedef struct {
    uint32_t longest_match_len;
    uint32_t longest_match_index;
} match_t;

typedef enum {
    LITERAL,
    MATCH,
} token_type;

typedef struct {
    token_type type;
    uint8_t literal;
    uint16_t length;
    uint16_t distance;
} lz_token_t;

void lz_77_encode(Byte *data, uint32_t data_len);

#endif
