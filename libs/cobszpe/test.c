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
 * COBS/ZPE API unit tests.
 */

#include <string.h>

#include "common/macros.h"
#include "cobszpe/cobszpe.h"

static void test_cobszpe_encode_run(
    const unsigned char *src, int src_size,
    const unsigned char *expected, int expected_size
    ) {
    unsigned char dst[AM_COBSZPE_ENCODED_SIZE_FOR(src_size)];
    int ret = am_cobszpe_encode(dst, (int)sizeof(dst), src, src_size);

    AM_ASSERT(ret == expected_size);
    AM_ASSERT(0 == memcmp(expected, dst, (size_t)expected_size));
}

static void test_cobszpe_encode(void) {
    static const unsigned char src_[] = {
        0x45, 0x00, 0x00,
        0x2c, 0x4c, 0x79, 0x00, 0x00,
        0x40, 0x06, 0x4f, 0x37
    };
    static const unsigned char exp_[] = {
        0xE2, 0x45,
        0xE4, 0x2c, 0x4c, 0x79,
        0x05, 0x40, 0x06, 0x4f, 0x37,
        0x00
    };
    test_cobszpe_encode_run(src_, (int)sizeof(src_), exp_, (int)sizeof(exp_));
}

int main(void) {
    test_cobszpe_encode();

    return 0;
}
