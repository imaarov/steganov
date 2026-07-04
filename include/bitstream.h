#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "utils.h"
#include <stddef.h>
#include <stdint.h>

#define BITSTREAM_INITIAL_CAPACITY 1024

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t byte_pos;
    uint8_t bit_pos;
} bitstream_t;

void bitstream_init(bitstream_t *stream);
void bitstream_write_bits_lsb(bitstream_t *stream, uint32_t value, uint8_t count);
void bitstream_write_bits_msb(bitstream_t *stream, uint32_t value, uint8_t count);
void bitstream_flush(bitstream_t *stream);
size_t bitstream_size(const bitstream_t *stream);
void print_bit_stream(const bitstream_t *stream);
void bitstream_destroy(bitstream_t *stream);

#endif
