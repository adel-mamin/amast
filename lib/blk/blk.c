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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "blk/blk.h"

struct blk blk_ctor(void *ptr, int size) {
    struct blk blk;
    blk.ptr = ptr;
    blk.size = size;
    return blk;
}

struct blk blk_ctor_empty(void) { return blk_ctor(/*ptr=*/NULL, /*size=*/0); }

bool blk_is_empty(const struct blk *blk) {
    return (NULL == blk) || (NULL == blk->ptr) || (0 == blk->size);
}

int blk_cmp(const struct blk *a, const struct blk *b) {
    ASSERT(a);
    ASSERT(a->ptr);
    ASSERT(a->size > 0);
    ASSERT(b);
    ASSERT(b->ptr);
    ASSERT(b->size > 0);

    int minsize = MIN(a->size, b->size);
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

void blk_zero(struct blk *blk) {
    ASSERT(blk);
    ASSERT(blk->ptr);
    ASSERT(blk->size > 0);
    memset(blk->ptr, 0, (size_t)blk->size);
}

void *blk_cpy(struct blk *dst, const struct blk *src) {
    ASSERT(dst);
    ASSERT(dst->ptr);
    ASSERT(src);
    ASSERT(src->ptr);
    ASSERT(dst->size > 0);
    ASSERT(dst->size == src->size);

    return memcpy(dst->ptr, src->ptr, src->size);
}