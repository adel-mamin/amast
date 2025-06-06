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
 * @file
 *
 * Bit utilities interface.
 */

#ifndef AM_BIT_H_INCLUDED
#define AM_BIT_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

/** A 64 bit array. */
struct am_bit_u64 {
    unsigned char bytes;   /**< redundant byte mask */
    unsigned char bits[8]; /**< the 64 bit array itself */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if bit array has no bits set to 1.
 *
 * @param u64  the bit array
 *
 * @retval true   the bit array is empty
 * @retval false  the bit array is not empty
 */
bool am_bit_u64_is_empty(const struct am_bit_u64 *u64);

/**
 * Return the index of the most significant bit (MSB) set to 1.
 *
 * @param u64  the bit array
 *
 * @return the MSB index
 */
int am_bit_u64_msb(const struct am_bit_u64 *u64);

/**
 * Return the index of the most significant bit (MSB) set to 1.
 *
 * @param u8  the bit array
 *
 * @return the MSB index
 */
int am_bit_u8_msb(uint8_t u8);

/**
 * Set bit with index \p n to 1.
 *
 * @param u64  the bit array
 * @param n    the index of bit to set. Zero based. The valid range [0..63].
 */
void am_bit_u64_set(struct am_bit_u64 *u64, int n);

/**
 * Clear a bit with index n to 1.
 *
 * @param u64  the bit array
 * @param n    the index. Zero based. The valid range [0..63].
 */
void am_bit_u64_clear(struct am_bit_u64 *u64, int n);

#ifdef __cplusplus
}
#endif

#endif /* AM_BIT_H_INCLUDED */
