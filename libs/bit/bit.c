/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 *
 * Source: https://github.com/adel-mamin/amast
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * Bit utilities.
 */

#include <stdbool.h>
#include <stdint.h>

#include "common/macros.h"
#include "bit/bit.h"

static const int8_t am_bit_msb_from_u8[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

bool am_bit_u64_is_empty(const struct am_bit_u64 *u64) {
    return 0 == u64->bytes;
}

int am_bit_u8_msb(uint8_t u8) { return am_bit_msb_from_u8[u8]; }

int am_bit_u64_msb(const struct am_bit_u64 *u64) {
    int i = am_bit_msb_from_u8[u64->bytes];
    return am_bit_msb_from_u8[u64->bits[i]] + i * 8;
}

void am_bit_u64_set(struct am_bit_u64 *u64, int n) {
    AM_ASSERT(n >= 0);
    AM_ASSERT(n < 64);

    unsigned i = (unsigned)n >> 3U;
    unsigned mask = u64->bytes;
    mask |= 1U << i;
    u64->bytes = (unsigned char)mask;

    mask = u64->bits[i];
    mask |= 1U << ((unsigned)n & 7U);
    u64->bits[i] = (unsigned char)mask;
}

void am_bit_u64_clear(struct am_bit_u64 *u64, int n) {
    AM_ASSERT(n >= 0);
    AM_ASSERT(n < 64);

    unsigned i = (unsigned)n >> 3U;
    unsigned mask = u64->bits[i];
    mask &= ~(1U << ((unsigned)n & 7U));
    u64->bits[i] = (unsigned char)mask;

    if (mask) {
        return;
    }
    mask = u64->bytes;
    mask &= ~(1U << i);
    u64->bytes = (unsigned char)mask;
}
