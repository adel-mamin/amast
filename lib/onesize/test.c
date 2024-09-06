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

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"
#include "slist/slist.h"
#include "blk/blk.h"
#include "onesize/onesize.h"

struct test {
    int a;
    float b;
    char c;
};

int main(void) {
    struct onesize ma;
    struct test test[3];
    struct blk blk = {.ptr = test, .size = sizeof(test)};

    onesize_init(&ma, &blk, sizeof(struct test), A1_ALIGN_MAX);

    ASSERT(onesize_get_nfree(&ma) == 2);

    void *ptr1 = onesize_allocate(&ma, 1);
    ASSERT(ptr1);
    ASSERT(onesize_get_nfree(&ma) == 1);

    void *ptr2 = onesize_allocate(&ma, 1);
    ASSERT(ptr2);
    ASSERT(onesize_get_nfree(&ma) == 0);

    void *ptr3 = onesize_allocate(&ma, 1);
    ASSERT(!ptr3);
    ASSERT(onesize_get_nfree(&ma) == 0);

    onesize_free(&ma, ptr1);
    ASSERT(onesize_get_nfree(&ma) == 1);

    onesize_free(&ma, ptr2);
    ASSERT(onesize_get_nfree(&ma) == 2);

    ptr1 = onesize_allocate(&ma, 1);
    ASSERT(ptr1);
    ASSERT(onesize_get_nfree(&ma) == 1);

    onesize_free_all(&ma);
    ASSERT(onesize_get_nfree(&ma) == 2);

    ASSERT(0 == onesize_get_min_nfree(&ma));

    return 0;
}
