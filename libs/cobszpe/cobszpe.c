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

#include "common/macros.h"
#include "cobszpe.h"

#define AM_FINISH_BLOCK(x) \
    (*code_ptr = (x), code_ptr = dst_++, code = 0x01, zero_cnt = 0)

int am_cobszpe_encode(void *dst, int dst_size, const void *src, int src_size) {
    AM_ASSERT(dst);
    AM_ASSERT(dst_size > 0);
    AM_ASSERT(src);
    AM_ASSERT(src_size > 0);
    AM_ASSERT(AM_COBSZPE_ENCODED_SIZE_FOR(src_size) <= dst_size);

    const unsigned char *src_ = src;
    const unsigned char *end = src_ + src_size;
    unsigned char *dst_ = dst;
    unsigned char *code_ptr = dst_++;
    unsigned char code = 0x01;
    int zero_cnt = 0;
    while (src_ < end) {
        if (*src_) {
            if (zero_cnt) {
                AM_FINISH_BLOCK(code);
            }
            *dst_++ = *src_++;
            code++;
            if (code == 0xE0) {
                AM_FINISH_BLOCK(code);
            }
            continue;
        }
        zero_cnt++;
        src_++;
        if (zero_cnt < 2) {
            continue;
        }
        if (code >= 30) {
            AM_FINISH_BLOCK(code);
        } else { /* ZPE case */
            AM_FINISH_BLOCK(code + 0xE0);
        }
    }
    *dst_ = 0;
    AM_FINISH_BLOCK(code);

    return (int)(dst_ - (unsigned char*)dst);
}

int am_cobszpe_decode(void *dst, int dst_size, const void *src, int src_size) {
    AM_ASSERT(dst);
    AM_ASSERT(dst_size > 0);
    AM_ASSERT(src);
    AM_ASSERT(src_size > 0);
    AM_ASSERT(AM_COBSZPE_DECODED_SIZE_FOR(src_size) <= dst_size);
    ((uint8_t*)dst)[0] = 0;
    return 0;
}
