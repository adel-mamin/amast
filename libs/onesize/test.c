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
 * Onesize memory allocator unit tests.
 */

#include <stddef.h> /* IWYU pragma: keep */

#include "common/macros.h"
#include "common/alignment.h"
#include "blk/blk.h"
#include "onesize/onesize.h"

int main(void) {
    struct am_onesize ma;
    struct test {
        int a;
        float b;
        unsigned *c;
    } test_arr[2];
    struct am_blk blk = {.ptr = &test_arr[0], .size = sizeof(test_arr)};

    struct am_onesize_cfg cfg = {
        .pool = &blk,
        .block_size = sizeof(struct test),
        .alignment = AM_ALIGNOF(struct test)
    };
    am_onesize_ctor(&ma, &cfg);

    AM_ASSERT(am_onesize_get_nfree(&ma) == 2);

    const void *ptr1 = am_onesize_allocate(&ma, 1);
    AM_ASSERT(ptr1);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 1);

    const void *ptr2 = am_onesize_allocate(&ma, 1);
    AM_ASSERT(ptr2);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 0);

    const void *ptr3 = am_onesize_allocate(&ma, 1);
    AM_ASSERT(!ptr3);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 0);

    am_onesize_free(&ma, ptr1);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 1);

    am_onesize_free(&ma, ptr2);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 2);

    ptr1 = am_onesize_allocate(&ma, 1);
    AM_ASSERT(ptr1);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 1);

    am_onesize_free_all(&ma);
    AM_ASSERT(am_onesize_get_nfree(&ma) == 2);

    AM_ASSERT(0 == am_onesize_get_min_nfree(&ma));

    return 0;
}
