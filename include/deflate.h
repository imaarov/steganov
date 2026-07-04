/*
 * Deflate conventions and specification based on the RFC 1951
 *
 * 1. BFINAL is a bit starting each blocks of deflate stream determine that if the following block
 * is the final block or not.
 * BFINAL = 1 (FINAL Block)
 * BFINAL = 0 (Not final Block)
 */
#ifndef DEFLATE_H
#define DEFLATE_H

#define FIX_SIZE_BLOCK_SIZE 32768
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

typedef struct {
    BFINAL bfinal;
    BTYPE  btype;
} deflate_block_t;

typedef struct {
} deflate_encode_option_t;

#endif
