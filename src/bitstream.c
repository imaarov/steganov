#include "../include/bitstream.h"
#include "utils.h"
#include <stdint.h>
#include <string.h>

void bitstream_init(bitstream_t *stream) {
    if (!stream || !stream->data || stream->capacity == 0) {
        FATAL("invalid bitstream");
        return;
    }
    stream->bit_pos  = 7;
    stream->byte_pos = 0;
    memset(stream->data, 0, stream->capacity);
}

void bitstream_write_bits_lsb(bitstream_t *stream, uint32_t value, uint8_t count) {
    if (count == 0)
        return;

    if (count > 32) {
        FATAL("invalid bit count");
        return;
    }

    for (uint8_t i = 0; i < count; i++) {
        if (stream->byte_pos >= stream->capacity) {
            FATAL("stream capacity exceeded");
            return;
        }

        // write bit from LSB to MSB
        if (value & (1u << i)) {
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

void bitstream_write_bits_msb(bitstream_t *stream, uint32_t value, uint8_t count) {
    if (count == 0)
        return;

    if (count > 32) {
        FATAL("invalid bit count");
        return;
    }

    for (int i = count - 1; i >= 0; i--) {
        if (stream->byte_pos >= stream->capacity) {
            FATAL("stream capacity exceeded");
            return;
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
    return stream->byte_pos + (stream->bit_pos == 7 ? 0 : 1);
}

void print_bit_stream(const bitstream_t *stream) {
    size_t size = bitstream_size(stream);
    for (size_t i = 0; i < size; i++)
        printf("%02X ", stream->data[i]);

    printf("\n");
}

void bitstream_flush(bitstream_t *stream) {
    if (stream->bit_pos != 7 && stream->byte_pos < stream->capacity) {
        stream->byte_pos++;
    }

    stream->bit_pos = 7;
}
