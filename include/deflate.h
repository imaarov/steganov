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

typedef enum {
    BFINAL_LAST_BLOCK     = 1,
    BFINAL_NOT_LAST_BLOCK = 0,
} BFINAL;

typedef enum {
    NO_COMPRESSION  = 0b00,
    FIXED_HUFFMAN   = 0b01,
    DYNAMIC_HUFFMAN = 0b10,
    ILLEGAL         = 0b11,
} BTYPE;

#endif
