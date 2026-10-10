#ifndef PNG_FILTER_H
#define PNG_FILTER_H

#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

#define PNG_FILTER_NONE    0
#define PNG_FILTER_SUB     1
#define PNG_FILTER_UP      2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH   4

#define PNG_FILTER_BYTE_SIZE 1 // every scanline starts with one filter-type byte

/**
 * Layouts used by both functions below (height = number of scanlines):
 *
 *   filtered : height * (stride + 1) bytes, scanline y lives at y * (stride + 1)
 *              [filter type][stride bytes of filtered data]
 *   raw      : height * stride bytes, scanline y lives at y * stride
 *              (the real pixel samples, no filter byte)
 *
 * stride = bytes in one scanline without the filter byte
 * bpp    = bytes per complete pixel (at least 1), the distance to the "left" neighbour
 */

/** filtered -> raw. Also reports which filter each scanline used (filter_types[height]). */
bool png_unfilter(const byte *filtered, uint32_t height, uint32_t stride, uint32_t bpp, byte *raw,
                  byte *filter_types);

/** raw -> filtered, using the given filter type for every scanline (filter_types[height]). */
bool png_filter(const byte *raw, uint32_t height, uint32_t stride, uint32_t bpp,
                const byte *filter_types, byte *filtered);

#endif
