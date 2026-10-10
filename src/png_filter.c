#include "../include/png_filter.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static byte paeth_predictor(byte a, byte b, byte c);
static byte predict(byte filter_type, byte a, byte b, byte c);

bool png_unfilter(const byte *filtered, uint32_t height, uint32_t stride, uint32_t bpp, byte *raw,
                  byte *filter_types) {
    if (NULL == filtered || NULL == raw || NULL == filter_types || 0 == bpp) {
        FATAL("png_unfilter: invalid arguments");
        return false;
    }

    for (uint32_t y = 0; y < height; y++) {
        const byte *src = filtered + (size_t)y * (stride + PNG_FILTER_BYTE_SIZE);
        byte *cur       = raw + (size_t)y * stride;
        // the row above is already un filtered; there is no row above the first scanline
        const byte *prior = (y > 0) ? cur - stride : NULL;

        byte filter_type = src[0];
        if (filter_type > PNG_FILTER_PAETH) {
            FATAL("png_unfilter: scanline %u has invalid filter type %u", y, filter_type);
            return false;
        }
        filter_types[y] = filter_type;
        src += PNG_FILTER_BYTE_SIZE;

        for (uint32_t x = 0; x < stride; x++) {
            byte a = (x >= bpp) ? cur[x - bpp] : 0;
            byte b = (NULL != prior) ? prior[x] : 0;
            byte c = (NULL != prior && x >= bpp) ? prior[x - bpp] : 0;
            cur[x] = (byte)(src[x] + predict(filter_type, a, b, c));
        }
    }
    return true;
}

bool png_filter(const byte *raw, uint32_t height, uint32_t stride, uint32_t bpp,
                const byte *filter_types, byte *filtered) {
    if (NULL == raw || NULL == filter_types || NULL == filtered || 0 == bpp) {
        FATAL("png_filter: invalid arguments");
        return false;
    }

    for (uint32_t y = 0; y < height; y++) {
        byte *dst         = filtered + (size_t)y * (stride + PNG_FILTER_BYTE_SIZE);
        const byte *cur   = raw + (size_t)y * stride;
        const byte *prior = (y > 0) ? cur - stride : NULL;

        byte filter_type = filter_types[y];
        if (filter_type > PNG_FILTER_PAETH) {
            FATAL("png_filter: scanline %u has invalid filter type %u", y, filter_type);
            return false;
        }
        dst[0] = filter_type;
        dst += PNG_FILTER_BYTE_SIZE;

        for (uint32_t x = 0; x < stride; x++) {
            byte a = (x >= bpp) ? cur[x - bpp] : 0;
            byte b = (NULL != prior) ? prior[x] : 0;
            byte c = (NULL != prior && x >= bpp) ? prior[x - bpp] : 0;
            dst[x] = (byte)(cur[x] - predict(filter_type, a, b, c));
        }
    }
    return true;
}

static byte predict(byte filter_type, byte a, byte b, byte c) {
    switch (filter_type) {
    case PNG_FILTER_SUB:
        return a;
    case PNG_FILTER_UP:
        return b;
    case PNG_FILTER_AVERAGE:
        // floor((a + b) / 2), the sum must not be done in 8 bits
        return (byte)(((unsigned int)a + (unsigned int)b) / 2);
    case PNG_FILTER_PAETH:
        return paeth_predictor(a, b, c);
    case PNG_FILTER_NONE:
    default:
        return 0;
    }
}

// pick whichever of a, b, c is closest to the "gradient" estimate p = a + b - c
static byte paeth_predictor(byte a, byte b, byte c) {
    int p  = (int)a + (int)b - (int)c;
    int pa = abs(p - (int)a);
    int pb = abs(p - (int)b);
    int pc = abs(p - (int)c);

    // the tie break order (a, then b, then c) is part of the spec, dont reorder it
    if (pa <= pb && pa <= pc)
        return a;
    if (pb <= pc)
        return b;
    return c;
}
