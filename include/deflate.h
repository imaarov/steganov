/*
 * @author imaarov
 *
 * Deflate conventions and specification based on the RFC 1951
 *
 * Deflate includes sequence and Blocks of data, it can be multuple or single block
 * Each blocks of DEFLATE start with (not neccesarily start of the byte or any fixed position on
 * byte) 1 bit of BFINAL and 2 bits of BTYPE based on that the other block blueprint will be define
 *
 * 1. BFINAL is a bit starting each blocks of deflate stream determine that if the following block
 * is the final block or not.
 * BFINAL = 1 (FINAL Block)
 * BFINAL = 0 (Not final Block)
 *
 * 2. BTYPE is two bit (00 to 11(reserved)) that includes 4 specification for the compression type
 * BTYPE = 00 (No compression)
 * BTYPE = 01 (Fixed Huffman compression)
 * BTYPE = 10 (Dynamic Huffman compression)
 * BTYPE = 11 (Reserved or Illegal)
 */
#ifndef DEFLATE_H
#define DEFLATE_H

#include "bitstream.h"
#define FIX_SIZE_BLOCK_SIZE     32768
#define UNCOMPRESSED_BLOCK_SIZE 65535

typedef enum {
    BFINAL_LAST_BLOCK     = 1,
    BFINAL_NOT_LAST_BLOCK = 0,
} BFINAL;

typedef enum {
    BTYPE_NO_COMPRESSION  = 0b00,
    BTYPE_FIXED_HUFFMAN   = 0b01,
    BTYPE_DYNAMIC_HUFFMAN = 0b10,
    BTYPE_ILLEGAL         = 0b11,
} BTYPE;

bitstream_t deflate_encode(byte *un_compressed_data, uint32_t data_len, BTYPE btype);

typedef struct {
    BFINAL bfinal;
    BTYPE btype;
} deflate_block_t;

typedef struct {
} deflate_encode_option_t;

#endif
