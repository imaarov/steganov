/**
 * DEFLATE pipeline encode/decode
 * based on RFC 1951
 * TODO: i think we should split this into two files
 * deflate_encode.c and deflate_decode.c
 */

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
static void write_dynamic_header(bitstream_t *stream, const canonical_huffman_code *huffman_code,
                                 byte *ll_lengths, uint32_t hlit_count, byte *dist_lengths,
                                 uint32_t hdist_count);

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
    // LZ77 compression
    lz_tokens_stream_t *lz_stream = lz_77_encode(data, data_len);
    if (NULL == lz_stream) {
        FATAL("failed to encode lz77 in deflate encoder");
        return;
    }

    // make lz77 deflated version
    deflate_lz77_stream_t *deflate_lz77_stream = deflate_lz77_encode(lz_stream);
    if (NULL == deflate_lz77_stream) {
        FATAL("failed to encode deflate lz77 in deflate encoder");
        free_lz77(lz_stream);
        return;
    }

    // build canonical huffman tree
    canonical_huffman_code huffman_code = build_huffman_tree(deflate_lz77_stream);
    if (!huffman_code.status) {
        FATAL("failed to build huffman tree");
        free_lz77(lz_stream);
        free_deflate_lz77_stream(deflate_lz77_stream);
        return;
    }

    // DEFLATe require that all length code be <= 15
    bool valid_lengths = true;
    for (uint32_t i = 0; i < huffman_code.literal_len_size; i++) {
        if (huffman_code.literal_len[i].length > 15) {
            valid_lengths = false;
            break;
        }
    }
    if (valid_lengths) {
        for (uint32_t i = 0; i < huffman_code.distance_size; i++) {
            if (huffman_code.distance[i].length > 15) {
                valid_lengths = false;
                break;
            }
        }
    }

    // if length is bigger than 15, fall back for no compression method
    if (!valid_lengths) {
        DEBUG("huffman code lengths > 15 detected, falling back to no compression");
        free_lz77(lz_stream);
        free_deflate_lz77_stream(deflate_lz77_stream);
        free_huffman(huffman_code);

        encode_deflate_no_compression(data, data_len, bitstream);
        return;
    }

    // write block header if the length are valid
    bitstream_write_bits_lsb(bitstream, BFINAL_LAST_BLOCK, 1);
    bitstream_write_bits_lsb(bitstream, BTYPE_DYNAMIC_HUFFMAN, 2);

    // prepare the length arrays for dynamic header
    uint16_t max_ll_symbol = EOB;
    for (uint32_t i = 0; i < huffman_code.literal_len_size; i++)
        if (huffman_code.literal_len[i].symbol > max_ll_symbol)
            max_ll_symbol = huffman_code.literal_len[i].symbol;
    uint32_t hlit_count = (uint32_t)max_ll_symbol + 1;

    bool have_distances      = huffman_code.distance_size > 0;
    uint16_t max_dist_symbol = 0;
    for (uint32_t i = 0; i < huffman_code.distance_size; i++)
        if (huffman_code.distance[i].symbol > max_dist_symbol)
            max_dist_symbol = huffman_code.distance[i].symbol;
    uint32_t hdist_count = have_distances ? (uint32_t)max_dist_symbol + 1 : 1;

    byte *ll_lengths = calloc(hlit_count, sizeof(byte));
    for (uint32_t i = 0; i < huffman_code.literal_len_size; i++)
        ll_lengths[huffman_code.literal_len[i].symbol] = (byte)huffman_code.literal_len[i].length;

    byte *dist_lengths = calloc(hdist_count, sizeof(byte));
    if (have_distances) {
        for (uint32_t i = 0; i < huffman_code.distance_size; i++)
            dist_lengths[huffman_code.distance[i].symbol] = (byte)huffman_code.distance[i].length;
    } else {
        dist_lengths[0] = 1; // distance code length of 1 if no distances used
    }

    // write dynamic header (HLIT, HDIST, HCLEN ...)
    write_dynamic_header(bitstream, &huffman_code, ll_lengths, hlit_count, dist_lengths,
                         hdist_count);
    free(ll_lengths);
    free(dist_lengths);

    // poof, finally write the compressed data tokens
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
            if (tok->length_extra_count > 0)
                bitstream_write_bits_lsb(bitstream, tok->length_extra_value,
                                         tok->length_extra_count);

            huffman_code_t *dc =
                find_code(huffman_code.distance, huffman_code.distance_size, tok->distance_symbol);
            bitstream_write_bits_msb(bitstream, dc->code, dc->length);
            if (tok->distance_extra_count > 0)
                bitstream_write_bits_lsb(bitstream, tok->distance_extra_value,
                                         tok->distance_extra_count);

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

static void write_dynamic_header(bitstream_t *stream, const canonical_huffman_code *huffman_code,
                                 byte *ll_lengths, uint32_t hlit_count, byte *dist_lengths,
                                 uint32_t hdist_count) {
    uint32_t combined_size = hlit_count + hdist_count;
    byte *combined         = malloc(combined_size);
    memcpy(combined, ll_lengths, hlit_count);
    if (hdist_count > 0)
        memcpy(combined + hlit_count, dist_lengths, hdist_count);

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

    uint8_t hclen_count = CODE_LENGTH_ALPHABET_SIZE;
    while (hclen_count > 4 && cl_lengths_in_order[hclen_count - 1] == 0)
        hclen_count--;

    DEBUG("rle.count=%u cl_code.size=%u", rle.count, cl_code.size);
    DEBUG("huffman_code.literal_len_size=%u", huffman_code->literal_len_size);

    bitstream_write_bits_lsb(stream, hlit_count - 257, 5);
    bitstream_write_bits_lsb(stream, hdist_count > 0 ? hdist_count - 1 : 0, 5);
    bitstream_write_bits_lsb(stream, hclen_count - 4, 4);

    for (uint8_t i = 0; i < hclen_count; i++)
        bitstream_write_bits_lsb(stream, cl_lengths_in_order[i], 3);

    for (uint32_t i = 0; i < rle.count; i++) {
        huffman_code_t *hc = find_code(cl_code.codes, cl_code.size, rle.items[i].symbol);
        bitstream_write_bits_msb(stream, hc->code, hc->length);
        if (rle.items[i].extra_bits > 0)
            bitstream_write_bits_lsb(stream, rle.items[i].extra_value, rle.items[i].extra_bits);
    }

    free(rle.items);
    free_huffman_code_set(cl_code);
}

static bool cl_rle_push(cl_rle_stream_t *s, uint8_t symbol, uint8_t extra_bits,
                        uint16_t extra_value, uint32_t *freq) {
    if (symbol >= CODE_LENGTH_ALPHABET_SIZE) {
        FATAL("Invalid CL symbol %u", symbol);
        abort();
    }
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

// decode
static bool out_push(byte **out, uint32_t *out_size, uint32_t *out_cap, byte b) {
    if (*out_size == *out_cap) {
        uint32_t new_cap = (*out_cap == 0) ? 256 : (*out_cap * 2);
        byte *tmp        = realloc(*out, new_cap);
        if (!tmp)
            return false;
        *out     = tmp;
        *out_cap = new_cap;
    }
    (*out)[(*out_size)++] = b;
    return true;
}

static const length_code_t *find_length_code_by_symbol(uint16_t symbol) {
    for (uint8_t i = 0; i < DEFLATE_LENGTH_CODE_TABLE_SIZE; i++)
        if (LENGTH_CODES[i].symbol == symbol)
            return &LENGTH_CODES[i];
    return NULL;
}

static const distance_code_t *find_distance_code_by_symbol(uint16_t symbol) {
    for (uint8_t i = 0; i < DEFLATE_DISTANCE_CODE_TABLE_SIZE; i++)
        if (DISTANCE_CODES[i].symbol == symbol)
            return &DISTANCE_CODES[i];
    return NULL;
}

static bool decode_stored_block(bitstream_t *reader, byte **out, uint32_t *out_size,
                                uint32_t *out_cap) {
    bitstream_align_byte(reader);
    if (reader->byte_pos + 4 > reader->capacity)
        return false;

    uint16_t len =
        (uint16_t)(reader->data[reader->byte_pos] | (reader->data[reader->byte_pos + 1] << 8));
    uint16_t nlen =
        (uint16_t)(reader->data[reader->byte_pos + 2] | (reader->data[reader->byte_pos + 3] << 8));
    reader->byte_pos += 4;

    if ((uint16_t)(~len & 0xFFFF) != nlen) {
        FATAL("stored block LEN/NLEN mismatch");
        return false;
    }
    if (reader->byte_pos + len > reader->capacity)
        return false;

    for (uint16_t i = 0; i < len; i++)
        if (!out_push(out, out_size, out_cap, reader->data[reader->byte_pos + i]))
            return false;
    reader->byte_pos += len;
    return true;
}

static bool decode_huffman_body(bitstream_t *reader, const huffman_code_t *ll_codes,
                                uint32_t ll_count, const huffman_code_t *dist_codes,
                                uint32_t dist_count, byte **out, uint32_t *out_size,
                                uint32_t *out_cap) {
    while (1) {
        int sym = decode_huffman_symbol(reader, ll_codes, ll_count);
        if (sym < 0)
            return false;

        if (sym < 256) {
            if (!out_push(out, out_size, out_cap, (byte)sym))
                return false;

        } else if (sym == EOB) {
            return true;

        } else {
            const length_code_t *lc = find_length_code_by_symbol((uint16_t)sym);
            if (!lc)
                return false;
            int32_t extra = bitstream_read_bits_lsb(reader, lc->extra_bits);
            if (extra < 0)
                return false;
            uint32_t length = lc->min_length + (uint32_t)extra;

            int dsym = decode_huffman_symbol(reader, dist_codes, dist_count);
            if (dsym < 0)
                return false;
            const distance_code_t *dc = find_distance_code_by_symbol((uint16_t)dsym);
            if (!dc)
                return false;
            int32_t dextra = bitstream_read_bits_lsb(reader, dc->extra_bits);
            if (dextra < 0)
                return false;
            uint32_t distance = dc->min_length + (uint32_t)dextra;

            if (distance > *out_size) {
                FATAL("back-reference distance exceeds decoded output so far");
                return false;
            }
            uint32_t start = *out_size - distance;
            for (uint32_t k = 0; k < length; k++)
                if (!out_push(out, out_size, out_cap, (*out)[start + k]))
                    return false;
        }
    }
}

static bool decode_fixed_block(bitstream_t *reader, byte **out, uint32_t *out_size,
                               uint32_t *out_cap) {
    byte ll_lengths[288];
    for (int i = 0; i <= 143; i++)
        ll_lengths[i] = 8;
    for (int i = 144; i <= 255; i++)
        ll_lengths[i] = 9;
    for (int i = 256; i <= 279; i++)
        ll_lengths[i] = 7;
    for (int i = 280; i <= 287; i++)
        ll_lengths[i] = 8;
    huffman_code_t ll_codes[288];
    uint32_t ll_count = assign_canonical_codes_from_lengths(ll_lengths, 288, ll_codes);

    byte dist_lengths[30];
    for (int i = 0; i < 30; i++)
        dist_lengths[i] = 5;
    huffman_code_t dist_codes[30];
    uint32_t dist_count = assign_canonical_codes_from_lengths(dist_lengths, 30, dist_codes);

    return decode_huffman_body(reader, ll_codes, ll_count, dist_codes, dist_count, out, out_size,
                               out_cap);
}

static bool decode_dynamic_block(bitstream_t *reader, byte **out, uint32_t *out_size,
                                 uint32_t *out_cap) {
    int32_t hlit_raw  = bitstream_read_bits_lsb(reader, 5);
    int32_t hdist_raw = bitstream_read_bits_lsb(reader, 5);
    int32_t hclen_raw = bitstream_read_bits_lsb(reader, 4);
    if (hlit_raw < 0 || hdist_raw < 0 || hclen_raw < 0)
        return false;

    uint32_t hlit_count  = (uint32_t)hlit_raw + 257;
    uint32_t hdist_count = (uint32_t)hdist_raw + 1;
    uint32_t hclen_count = (uint32_t)hclen_raw + 4;

    byte cl_lengths[CODE_LENGTH_ALPHABET_SIZE] = {0};
    for (uint32_t i = 0; i < hclen_count; i++) {
        int32_t v = bitstream_read_bits_lsb(reader, 3);
        if (v < 0)
            return false;
        cl_lengths[CLC_ORDER[i]] = (byte)v;
    }
    huffman_code_t cl_codes[CODE_LENGTH_ALPHABET_SIZE];
    uint32_t cl_code_count =
        assign_canonical_codes_from_lengths(cl_lengths, CODE_LENGTH_ALPHABET_SIZE, cl_codes);

    uint32_t total_lengths = hlit_count + hdist_count;
    byte *combined         = calloc(total_lengths, 1);
    if (!combined)
        return false;

    uint32_t pos = 0;
    while (pos < total_lengths) {
        int sym = decode_huffman_symbol(reader, cl_codes, cl_code_count);
        if (sym < 0) {
            free(combined);
            return false;
        }

        if (sym <= 15) {
            combined[pos++] = (byte)sym;
        } else if (sym == 16) {
            int32_t extra = bitstream_read_bits_lsb(reader, 2);
            if (extra < 0 || pos == 0) {
                free(combined);
                return false;
            }
            byte prev = combined[pos - 1];
            for (int32_t k = 0; k < extra + 3 && pos < total_lengths; k++)
                combined[pos++] = prev;
        } else if (sym == 17) {
            int32_t extra = bitstream_read_bits_lsb(reader, 3);
            if (extra < 0) {
                free(combined);
                return false;
            }
            for (int32_t k = 0; k < extra + 3 && pos < total_lengths; k++)
                combined[pos++] = 0;
        } else if (sym == 18) {
            int32_t extra = bitstream_read_bits_lsb(reader, 7);
            if (extra < 0) {
                free(combined);
                return false;
            }
            for (int32_t k = 0; k < extra + 11 && pos < total_lengths; k++)
                combined[pos++] = 0;
        } else {
            free(combined);
            return false;
        }
    }

    byte *ll_lengths   = combined;
    byte *dist_lengths = combined + hlit_count;

    huffman_code_t *ll_codes = malloc(sizeof(huffman_code_t) * hlit_count);
    uint32_t ll_count = assign_canonical_codes_from_lengths(ll_lengths, hlit_count, ll_codes);
    huffman_code_t *dist_codes = malloc(sizeof(huffman_code_t) * hdist_count);
    uint32_t dist_count =
        assign_canonical_codes_from_lengths(dist_lengths, hdist_count, dist_codes);

    bool ok = decode_huffman_body(reader, ll_codes, ll_count, dist_codes, dist_count, out, out_size,
                                  out_cap);

    free(ll_codes);
    free(dist_codes);
    free(combined);
    return ok;
}

byte *deflate_decode(const byte *compressed, uint32_t compressed_size, uint32_t *out_len) {
    bitstream_t reader;
    bitstream_init_from_buffer(&reader, compressed, compressed_size);

    byte *out         = NULL;
    uint32_t out_size = 0, out_cap = 0;

    bool final = false;
    while (!final) {
        int bfinal    = bitstream_read_bit(&reader);
        int32_t btype = bitstream_read_bits_lsb(&reader, 2);
        if (bfinal < 0 || btype < 0) {
            free(out);
            return NULL;
        }
        final = (bfinal == 1);

        bool ok;
        if (btype == BTYPE_NO_COMPRESSION)
            ok = decode_stored_block(&reader, &out, &out_size, &out_cap);
        else if (btype == BTYPE_FIXED_HUFFMAN)
            ok = decode_fixed_block(&reader, &out, &out_size, &out_cap);
        else if (btype == BTYPE_DYNAMIC_HUFFMAN)
            ok = decode_dynamic_block(&reader, &out, &out_size, &out_cap);
        else {
            FATAL("invalid BTYPE in compressed stream");
            free(out);
            return NULL;
        }

        if (!ok) {
            FATAL("failed to decode deflate block");
            free(out);
            return NULL;
        }
    }

    *out_len = out_size;
    return out;
}
