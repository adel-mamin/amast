/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017,2018,2020,2021 Adel Mamin
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
#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"

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

ASSERT_STATIC(ALIGNOF(struct test_align) >= sizeof(unsigned long long));
ASSERT_STATIC(16 == ALIGN_SIZE(1u, 16u));
ASSERT_STATIC(16 == ALIGN_SIZE(16u, 16u));

int main(void) {
    unsigned long data = 0;
    ASSERT(ALIGNOF_PTR(&data) >= 4);

    {
        uintptr_t ptr = 0x10;
        ASSERT(0x10 == CAST(uintptr_t, ALIGN_PTR_UP((void *)ptr, 16)));
    }

    {
        uintptr_t ptr = 0x1F;
        ASSERT(0x20 == CAST(uintptr_t, ALIGN_PTR_UP((void *)ptr, 16)));
    }

    {
        uintptr_t ptr = 0x10;
        ASSERT(0x10 == CAST(uintptr_t, ALIGN_PTR_DOWN((void *)ptr, 16)));
    }

    {
        uintptr_t ptr = 0x1F;
        ASSERT(0x10 == CAST(uintptr_t, ALIGN_PTR_DOWN((void *)ptr, 16)));
    }

    return 0;
}