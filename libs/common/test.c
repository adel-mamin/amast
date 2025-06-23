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
 * alignment API unit tests
 */

#include <stdint.h>

#include "compiler.h"
#include "macros.h"
#include "alignment.h"

struct test_align {
    /* cppcheck-suppress unusedStructMember */
    unsigned char a;
    /* cppcheck-suppress unusedStructMember */
    unsigned short b;
    /* cppcheck-suppress unusedStructMember */
    unsigned long c;
    /* cppcheck-suppress unusedStructMember */
    unsigned long long d;
};

AM_ASSERT_STATIC(16 == AM_ALIGN_SIZE(1u, 16u));
AM_ASSERT_STATIC(16 == AM_ALIGN_SIZE(16u, 16u));

int main(void) {
    unsigned long data = 0;
    AM_ASSERT(AM_ALIGNOF_PTR(&data) >= 4);

    {
        uintptr_t ptr = 0x10;
        AM_ASSERT(0x10 == AM_CAST(uintptr_t, AM_ALIGN_PTR_UP((void *)ptr, 16)));
    }

    {
        uintptr_t ptr = 0x1F;
        AM_ASSERT(0x20 == AM_CAST(uintptr_t, AM_ALIGN_PTR_UP((void *)ptr, 16)));
    }

    {
        uintptr_t ptr = 0x10;
        AM_ASSERT(
            0x10 == AM_CAST(uintptr_t, AM_ALIGN_PTR_DOWN((void *)ptr, 16))
        );
    }

    {
        uintptr_t ptr = 0x1F;
        AM_ASSERT(
            0x10 == AM_CAST(uintptr_t, AM_ALIGN_PTR_DOWN((void *)ptr, 16))
        );
    }
    {
        AM_ASSERT(AM_ALIGN_SIZE(3, 4) == 4);
        AM_ASSERT(AM_ALIGN_SIZE(4, 4) == 4);
        AM_ASSERT(AM_ALIGN_SIZE(5, 4) == 8);
    }

    return 0;
}
