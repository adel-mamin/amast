/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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
 * Bit utilities interface.
 */

#ifndef BIT_H_INCLUDED
#define BIT_H_INCLUDED

/** A 64 bit array */
struct am_bit_u64 {
    unsigned char bytes;   /**< redundant byte mask */
    unsigned char bits[8]; /**< the 64 bit array itself */
};

/**
 * Check if bit array has no bits set to 1.
 *
 * @param u64  the bit array to check
 * @retval true   bit array is empty
 * @retval false  bit array is not empty
 */
bool am_bit_u64_is_empty(const struct am_bit_u64 *u64);

/**
 * Return the index of the most significant bit (MSB) set to 1.
 * @param u64  the bit array to check
 * @return the MSB index
 */
int am_bit_u64_msb(const struct am_bit_u64 *u64);

/**
 * Set bit with index n to 1.
 * @param u64  the bit array
 * @param n    the index of bit to set. Zero based. The valid range [0..63].
 */
void am_bit_u64_set(struct am_bit_u64 *u64, int n);

/**
 * Clears a bit to 1 with index n.
 * @param u64 the bit array.
 * @param n the index. Zero based. The valid range [0..63].
 */
void am_bit_u64_clear(struct am_bit_u64 *u64, int n);

#endif /* BIT_H_INCLUDED */
