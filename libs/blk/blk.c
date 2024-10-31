/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2024 Adel Mamin
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

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "common/compiler.h" /* IWYU pragma: keep */
#include "common/macros.h"
#include "blk/blk.h"

struct am_blk am_blk_ctor(void *ptr, int size) {
    struct am_blk blk;
    blk.ptr = ptr;
    blk.size = size;
    return blk;
}

struct am_blk am_blk_ctor_empty(void) {
    return am_blk_ctor(/*ptr=*/NULL, /*size=*/0);
}

bool am_blk_is_empty(const struct am_blk *blk) {
    return (NULL == blk) || (NULL == blk->ptr) || (0 == blk->size);
}

int am_blk_cmp(const struct am_blk *a, const struct am_blk *b) {
    AM_ASSERT(a);
    AM_ASSERT(a->ptr);
    AM_ASSERT(a->size > 0);
    AM_ASSERT(b);
    AM_ASSERT(b->ptr);
    AM_ASSERT(b->size > 0);

    int minsize = AM_MIN(a->size, b->size);
    int cmp = memcmp(a->ptr, b->ptr, (size_t)minsize);
    if (cmp) {
        return cmp;
    }
    if (a->size < b->size) {
        return -1;
    }
    if (a->size > b->size) {
        return 1;
    }
    return 0;
}

void am_blk_zero(struct am_blk *blk) {
    AM_ASSERT(blk);
    AM_ASSERT(blk->ptr);
    AM_ASSERT(blk->size > 0);
    memset(blk->ptr, 0, (size_t)blk->size);
}

void *am_blk_copy(struct am_blk *dst, const struct am_blk *src) {
    AM_ASSERT(dst);
    AM_ASSERT(dst->ptr);
    AM_ASSERT(src);
    AM_ASSERT(src->ptr);
    AM_ASSERT(dst->size > 0);
    AM_ASSERT(dst->size == src->size);

    return memcpy(dst->ptr, src->ptr, (size_t)src->size);
}
