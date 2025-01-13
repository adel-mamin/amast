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
 * Consistent overhead byte stuffing (COBS) with zero pair elimination (ZPE)
 */

#ifndef AM_COBSZPE_H_INCLUDED
#define AM_COBSZPE_H_INCLUDED

/** maximum data length without explicit zero byte(s) (for internal use) */
#define AM_COBSZPE_ZEROLESS_MAX 223

#define AM_COBSZPE_ENCODED_SIZE_FOR(n) \
    ((n) + ((n) + AM_COBSZPE_ZEROLESS_MAX) / (AM_COBSZPE_ZEROLESS_MAX + 1) + 1)

#define AM_COBSZPE_DECODED_SIZE_FOR(n) (((n) <= 0) ? 0 : (n) - 1)

int am_cobszpe_encode(void *to, int to_size, const void *from, int from_size);
int am_cobszpe_decode(void *to, int to_size, const void *from, int from_size);

#endif /* AM_COBSZPE_H_INCLUDED */
