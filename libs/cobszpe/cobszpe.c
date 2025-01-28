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

#include <stdbool.h>
#include <stdint.h>

#include "common/macros.h"
#include "cobszpe.h"

#define AM_FINISH_BLOCK(x)           \
    (*code_ptr = (unsigned char)(x), \
     code_ptr = dst++,               \
     code = 0x01,                    \
     zero_cnt = 0,                   \
     block_finished = true)

int am_cobszpe_encode(void *to, int to_size, const void *from, int from_size) {
    AM_ASSERT(to);
    AM_ASSERT(to_size > 0);
    AM_ASSERT(from);
    AM_ASSERT(from_size > 0);
    AM_ASSERT(to_size > AM_COBSZPE_ENCODED_SIZE_FOR(from_size) - 1);

    const unsigned char *src = from;
    const unsigned char *end = src + from_size;
    unsigned char *dst = to;
    unsigned char *dst_orig = dst;
    unsigned char *code_ptr = dst++;
    unsigned char code = 1;
    int zero_cnt = 0;
    bool block_finished = false;
    while (src < end) {
        block_finished = false;
        if (*src != 0) {
            if (zero_cnt > 0) {
                AM_FINISH_BLOCK(code);
            }
            *dst++ = *src++;
            ++code;
            if ((AM_COBSZPE_ZEROLESS_MAX + 1) == code) {
                AM_FINISH_BLOCK(code);
            }
            continue;
        }
        ++zero_cnt;
        ++src;
        if (code >= (UINT8_MAX - AM_COBSZPE_ZEROLESS_MAX)) {
            AM_FINISH_BLOCK(code);
            continue;
        }
        if (zero_cnt < 2) {
            continue;
        }
        /* ZPE case */
        AM_FINISH_BLOCK(code + AM_COBSZPE_ZEROLESS_MAX + 1);
    }
    if (block_finished) {
        *code_ptr = 0;
    } else {
        *code_ptr = code;
        *dst++ = 0;
    }

    return (int)(dst - dst_orig);
}

int am_cobszpe_decode(void *dst, int dst_size, const void *src, int src_size) {
    AM_ASSERT(dst);
    AM_ASSERT(dst_size > 0);
    AM_ASSERT(src);
    AM_ASSERT(src_size > 0);
    AM_ASSERT(AM_COBSZPE_DECODED_SIZE_FOR(src_size) <= dst_size);
    ((uint8_t *)dst)[0] = 0;
    return 0;
}
