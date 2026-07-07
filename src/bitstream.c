#include "../include/bitstream.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool bitstream_grow(bitstream_t *stream);

void bitstream_init(bitstream_t *stream) {
    if (NULL == stream) {
        FATAL("bitstream is NULL and passed out to the bitstream init");
        return;
    }
    stream->capacity = BITSTREAM_INITIAL_CAPACITY;
    stream->data     = malloc(stream->capacity);
    if (NULL == stream->data) {
        FATAL("bitstream_init: failed to allocate memory");
        stream->capacity = 0;
        return;
    }
    memset(stream->data, 0, stream->capacity);
    stream->bit_pos  = 0;
    stream->byte_pos = 0;
}

void bitstream_write_bits_lsb(bitstream_t *stream, uint32_t value, uint8_t count) {
    if (!stream || !stream->data) {
        FATAL("stream or stream data in bitstream write lsb is NULL");
        return;
    }
    if (count == 0)
        return;

    if (count > 32) {
        FATAL("count is bigger than MAXIMUM supported value");
        return;
    }

    for (uint8_t i = 0; i < count; i++) {
        if (stream->byte_pos >= stream->capacity) {
            if (!bitstream_grow(stream)) {
                return;
            }
        }

        // write bit from LSB to MSB
        // we check the target bit, if its one we only insert one in the stream of data
        // beside thats theres only one other situation left that is the target bit = 0
        // then because we do the zeroing in the init function, meaning that all stream data
        // should be zero from inital or from grow function, we should not insert the 0 cause its
        // already there we just move the bit pointer by one
        if (value & (1u << i)) {
            stream->data[stream->byte_pos] |= (uint8_t)(1u << stream->bit_pos);
        }

        if (stream->bit_pos == 7) {
            stream->bit_pos = 0;
            stream->byte_pos++;
        } else {
            stream->bit_pos++;
        }
    }
}

void bitstream_write_bits_msb(bitstream_t *stream, uint32_t value, uint8_t count) {
    if (!stream || !stream->data) {
        FATAL("stream or stream data in bitstream write msb is NULL");
        return;
    }
    if (count == 0)
        return;

    if (count > 32) {
        FATAL("invalid bit count");
        return;
    }

    for (int i = count - 1; i >= 0; i--) {
        if (stream->byte_pos >= stream->capacity) {
            if (!bitstream_grow(stream)) {
                return;
            }
        }

        // writebit from MSB to LSB
        if ((value >> i) & 1u) {
            stream->data[stream->byte_pos] |= (uint8_t)(1u << stream->bit_pos);
        }

        if (stream->bit_pos == 0) {
            stream->bit_pos = 7;
            stream->byte_pos++;
        } else {
            stream->bit_pos--;
        }
    }
}

size_t bitstream_size(const bitstream_t *stream) {
    if (NULL == stream || NULL == stream->data) {
        return 0;
    }
    size_t size = stream->byte_pos;
    if (stream->bit_pos != 0) {
        ++size;
    }
    return size;
}

void print_bit_stream(const bitstream_t *stream) {
    if (NULL == stream || NULL == stream->data) {
        FATAL("empty stream");
        return;
    }
    size_t size = bitstream_size(stream);
    for (size_t i = 0; i < size; i++)
        printf("%02X ", stream->data[i]);

    printf("\n");
}

void bitstream_destroy(bitstream_t *stream) {
    if (NULL == stream) {
        return;
    }
    free(stream->data);
    stream->capacity = 0;
    stream->byte_pos = 0;
    stream->bit_pos  = 0;
    stream->data     = NULL;
}

static bool bitstream_grow(bitstream_t *stream) {
    if (NULL == stream) {
        return false;
    }
    if (stream->capacity > SIZE_MAX / 2) {
        FATAL("bit stream will be reach the SIZE MAX after grow");
        return false;
    }
    size_t old_capacity = stream->capacity;
    size_t new_capacity = stream->capacity * 2;
    byte *temp          = realloc(stream->data, new_capacity);
    if (NULL == temp) {
        FATAL("failed to allocate memory");
        return false;
    }
    memset(temp + old_capacity, 0, new_capacity - old_capacity);
    stream->data     = temp;
    stream->capacity = new_capacity;
    return true;
}

void bitstream_flush(bitstream_t *stream) {
    if (NULL == stream || NULL == stream->data) {
        return;
    }

    if (stream->bit_pos != 0) {
        stream->byte_pos++;
        stream->bit_pos = 0;
    }
}
