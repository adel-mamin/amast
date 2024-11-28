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
 * Bit API unit tests.
 */

#include "common/macros.h"
#include "bit/bit.h"

static void test_bit_u64(void) {
    struct am_bit_u64 u64 = {0};

    AM_ASSERT(am_bit_u64_is_empty(&u64));

    am_bit_u64_set(&u64, 0);
    int msb = am_bit_u64_msb(&u64);
    AM_ASSERT(0 == msb);
    AM_ASSERT(!am_bit_u64_is_empty(&u64));

    am_bit_u64_set(&u64, 15);
    msb = am_bit_u64_msb(&u64);
    AM_ASSERT(15 == msb);
    AM_ASSERT(!am_bit_u64_is_empty(&u64));

    am_bit_u64_set(&u64, 63);
    msb = am_bit_u64_msb(&u64);
    AM_ASSERT(63 == msb);
    AM_ASSERT(!am_bit_u64_is_empty(&u64));

    am_bit_u64_clear(&u64, 63);
    msb = am_bit_u64_msb(&u64);
    AM_ASSERT(15 == msb);
    AM_ASSERT(!am_bit_u64_is_empty(&u64));

    am_bit_u64_clear(&u64, 15);
    msb = am_bit_u64_msb(&u64);
    AM_ASSERT(0 == msb);
    AM_ASSERT(!am_bit_u64_is_empty(&u64));

    am_bit_u64_clear(&u64, 0);
    AM_ASSERT(am_bit_u64_is_empty(&u64));
}

int main(void) {
    test_bit_u64();

    return 0;
}
