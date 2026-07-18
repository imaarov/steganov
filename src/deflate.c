#include "../include/deflate.h"
#include "../include/bitstream.h"
#include "deflate_lz77.h"
#include "huffman.h"
#include "lz77.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void encode_deflate_no_compression(byte *data, uint32_t data_len, bitstream_t *bitstream);
static void encode_deflate_dynamic_huffman(byte *data, uint32_t data_len, bitstream_t *bitstream);
static huffman_code_t *find_code(huffman_code_t *codes, uint32_t size, uint16_t symbol);
static bool run_length_encode(const byte *lengths, uint32_t total, cl_rle_stream_t *out,
                              uint32_t *freq);

bitstream_t deflate_encode(byte *un_compressed_data, uint32_t data_len, BTYPE btype) {
    bitstream_t stream;
    bitstream_init(&stream);

    if (btype == BTYPE_NO_COMPRESSION) {
        encode_deflate_no_compression(un_compressed_data, data_len, &stream);
        return stream;
    }

    if (btype == BTYPE_DYNAMIC_HUFFMAN) {
        encode_deflate_dynamic_huffman(un_compressed_data, data_len, &stream);
        return stream;
    }

    // should add the others BTYPE here but i start from the BTYPE_NO_COMPRESSION for now
    return stream;
}

static void encode_deflate_no_compression(byte *data, uint32_t data_len, bitstream_t *stream) {
    uint32_t process_bytes = 0;

    while (process_bytes < data_len) {
        // embed the deflate block header
        uint32_t block_size = data_len - process_bytes;
        if (block_size > UNCOMPRESSED_BLOCK_SIZE) {
            block_size = UNCOMPRESSED_BLOCK_SIZE;
        }
        bitstream_write_bits_lsb(
            stream,
            process_bytes + block_size >= data_len ? BFINAL_LAST_BLOCK : BFINAL_NOT_LAST_BLOCK, 1);
        bitstream_write_bits_lsb(stream, BTYPE_NO_COMPRESSION, 2);
        // based on the 1951 RFC we should skip bits remain from the byte after emben deflate block
        // header
        uint8_t padding_bits = stream->bit_pos == 7 ? 0 : 8 - stream->bit_pos;
        bitstream_write_bits_lsb(stream, 0, padding_bits);

        // 1951 RFC: embed 16 bits LEN (data len)
        bitstream_write_bits_lsb(stream, block_size, 16);
        // 1951 RFC: embed 16 bits NLEN (data len in ones complement)
        bitstream_write_bits_lsb(stream, ~block_size & 0xFFFF, 16);

        for (uint32_t i = 0; i < block_size; i++) {
            bitstream_write_bits_lsb(stream, data[process_bytes + i], 8);
        }

        process_bytes += block_size;
    }
}

static void encode_deflate_dynamic_huffman(byte *data, uint32_t data_len, bitstream_t *bitstream) {
    lz_tokens_stream_t *lz_stream = lz_77_encode(data, data_len);
    if (NULL == lz_stream) {
        FATAL("failed to encode lz77 in deflate encoder");
        return;
    }

    deflate_lz77_stream_t *deflate_lz77_stream = deflate_lz77_encode(lz_stream);
    if (NULL == deflate_lz77_stream) {
        FATAL("failed to encode deflate lz77 in deflate encoder");
        free_lz77(lz_stream);
        return;
    }

    canonical_huffman_code huffman_code = build_huffman_tree(deflate_lz77_stream);
    if (!huffman_code.status) {
        FATAL("failed to build huffman tree");
        free_lz77(lz_stream);
        free_deflate_lz77_stream(deflate_lz77_stream);
        return;
    }

    bitstream_write_bits_lsb(bitstream, BFINAL_LAST_BLOCK, 1);
    bitstream_write_bits_lsb(bitstream, BTYPE_DYNAMIC_HUFFMAN, 2);

    uint16_t max_ll_symbol = EOB;
    for (uint32_t i = 0; i < huffman_code.literal_len_size; i++)
        if (huffman_code.literal_len[i].symbol > max_ll_symbol)
            max_ll_symbol = huffman_code.literal_len[i].symbol;
    uint32_t hlit_count = (uint32_t)max_ll_symbol + 1;
    uint32_t hlit       = hlit_count - 257;

    bool have_distances      = huffman_code.distance_size > 0;
    uint16_t max_dist_symbol = 0;
    for (uint32_t i = 0; i < huffman_code.distance_size; i++)
        if (huffman_code.distance[i].symbol > max_dist_symbol)
            max_dist_symbol = huffman_code.distance[i].symbol;
    uint32_t hdist_count = have_distances ? (uint32_t)max_dist_symbol + 1 : 1;
    uint32_t hdist       = hdist_count - 1;

    byte *ll_lengths = calloc(hlit_count, sizeof(byte));
    for (uint32_t i = 0; i < huffman_code.literal_len_size; i++)
        ll_lengths[huffman_code.literal_len[i].symbol] = (byte)huffman_code.literal_len[i].length;

    byte *dist_lengths = calloc(hdist_count, sizeof(byte));
    if (have_distances) {
        for (uint32_t i = 0; i < huffman_code.distance_size; i++)
            dist_lengths[huffman_code.distance[i].symbol] = (byte)huffman_code.distance[i].length;
    } else {
        dist_lengths[0] = 1;
    }

    uint32_t combined_size = hlit_count + hdist_count;
    byte *combined         = malloc(combined_size);
    memcpy(combined, ll_lengths, hlit_count);
    memcpy(combined + hlit_count, dist_lengths, hdist_count);
    free(ll_lengths);
    free(dist_lengths);

    cl_rle_stream_t rle                         = {0};
    uint32_t cl_freq[CODE_LENGTH_ALPHABET_SIZE] = {0};
    run_length_encode(combined, combined_size, &rle, cl_freq);
    free(combined);

    huffman_code_set_t cl_code = build_canonical_huffman(cl_freq, CODE_LENGTH_ALPHABET_SIZE);

    uint8_t cl_lengths_in_order[CODE_LENGTH_ALPHABET_SIZE] = {0};
    for (uint16_t i = 0; i < cl_code.size; i++) {
        for (uint8_t p = 0; p < CODE_LENGTH_ALPHABET_SIZE; p++) {
            if (CLC_ORDER[p] == cl_code.codes[i].symbol) {
                cl_lengths_in_order[p] = (uint8_t)cl_code.codes[i].length;
                break;
            }
        }
    }
    fprintf(stderr, "DEBUG rle.count=%u cl_code.size=%u\n", rle.count, cl_code.size);
    fprintf(stderr, "DEBUG deflate_lz77_stream->size=%u huffman_code.literal_len_size=%u\n",
            deflate_lz77_stream->size, huffman_code.literal_len_size);
    uint8_t hclen_count = CODE_LENGTH_ALPHABET_SIZE;
    while (hclen_count > 4 && cl_lengths_in_order[hclen_count - 1] == 0)
        hclen_count--;

    bitstream_write_bits_lsb(bitstream, hlit, 5);
    bitstream_write_bits_lsb(bitstream, hdist, 5);
    bitstream_write_bits_lsb(bitstream, hclen_count - 4, 4);

    for (uint8_t i = 0; i < hclen_count; i++)
        bitstream_write_bits_lsb(bitstream, cl_lengths_in_order[i], 3);

    for (uint32_t i = 0; i < rle.count; i++) {
        huffman_code_t *hc = find_code(cl_code.codes, cl_code.size, rle.items[i].symbol);
        bitstream_write_bits_msb(bitstream, hc->code, hc->length); // Huffman codes: MSB-first
        if (rle.items[i].extra_bits > 0)
            bitstream_write_bits_lsb(bitstream, rle.items[i].extra_value, rle.items[i].extra_bits);
    }
    free(rle.items);
    free_huffman_code_set(cl_code);

    for (uint32_t i = 0; i < deflate_lz77_stream->size; i++) {
        deflate_lz77_token_t *tok = &deflate_lz77_stream->tokens[i];

        if (tok->type == LITERAL) {
            huffman_code_t *hc =
                find_code(huffman_code.literal_len, huffman_code.literal_len_size, tok->literal);
            bitstream_write_bits_msb(bitstream, hc->code, hc->length);

        } else if (tok->type == MATCH) {
            huffman_code_t *lc = find_code(huffman_code.literal_len, huffman_code.literal_len_size,
                                           tok->length_symbol);
            bitstream_write_bits_msb(bitstream, lc->code, lc->length);
            if (tok->length_extra_value > 0)
                bitstream_write_bits_lsb(bitstream, tok->length_extra_value,
                                         tok->length_extra_count);

            huffman_code_t *dc =
                find_code(huffman_code.distance, huffman_code.distance_size, tok->distance_symbol);
            bitstream_write_bits_msb(bitstream, dc->code, dc->length);
            if (tok->distance_extra_value > 0)
                bitstream_write_bits_lsb(bitstream, tok->distance_extra_value,
                                         tok->distance_extra_value);

        } else if (tok->type == EOB_TOKEN) {
            huffman_code_t *hc =
                find_code(huffman_code.literal_len, huffman_code.literal_len_size, EOB);
            bitstream_write_bits_msb(bitstream, hc->code, hc->length);
        }
    }

    free_lz77(lz_stream);
    free_deflate_lz77_stream(deflate_lz77_stream);
    free_huffman(huffman_code);
}

static bool cl_rle_push(cl_rle_stream_t *s, uint8_t symbol, uint8_t extra_bits,
                        uint16_t extra_value, uint32_t *freq) {
    if (s->count == s->capacity) {
        uint32_t new_cap     = s->capacity == 0 ? 64 : s->capacity * 2;
        cl_rle_symbol_t *tmp = realloc(s->items, new_cap * sizeof(cl_rle_symbol_t));
        if (!tmp) {
            return false;
        }
        s->items    = tmp;
        s->capacity = new_cap;
    }

    s->items[s->count++] = (cl_rle_symbol_t){symbol, extra_bits, extra_value};
    freq[symbol]++;
    return true;
}
static bool run_length_encode(const byte *lengths, uint32_t total, cl_rle_stream_t *out,
                              uint32_t *freq) {
    uint32_t i = 0;
    while (i < total) {
        byte len     = lengths[i];
        uint32_t run = 1;
        while (i + run < total && lengths[i + run] == len)
            run++;

        if (len == 0) {
            uint32_t remaining = run;
            while (remaining > 0) {
                if (remaining >= 11) {
                    uint32_t take = remaining > 138 ? 138 : remaining;
                    if (!cl_rle_push(out, 18, 7, (uint16_t)(take - 11), freq))
                        return false;
                    remaining -= take;
                } else if (remaining >= 3) {
                    if (!cl_rle_push(out, 17, 3, (uint16_t)(remaining - 3), freq))
                        return false;
                    remaining = 0;
                } else {
                    for (uint32_t k = 0; k < remaining; k++)
                        if (!cl_rle_push(out, 0, 0, 0, freq))
                            return false;
                    remaining = 0;
                }
            }
        } else {
            if (!cl_rle_push(out, len, 0, 0, freq))
                return false;
            uint32_t remaining = run - 1;
            while (remaining > 0) {
                if (remaining >= 3) {
                    uint32_t take = remaining > 6 ? 6 : remaining;
                    if (!cl_rle_push(out, 16, 2, (uint16_t)(take - 3), freq))
                        return false;
                    remaining -= take;
                } else {
                    for (uint32_t k = 0; k < remaining; k++)
                        if (!cl_rle_push(out, len, 0, 0, freq))
                            return false;
                    remaining = 0;
                }
            }
        }
        i += run;
    }
    return true;
}

static huffman_code_t *find_code(huffman_code_t *codes, uint32_t size, uint16_t symbol) {
    for (uint32_t i = 0; i < size; i++)
        if (codes[i].symbol == symbol)
            return &codes[i];
    return NULL;
}
