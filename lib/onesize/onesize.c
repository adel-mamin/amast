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
 * onesize memory allocator implementation
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"
#include "slist/slist.h"
#include "blk/blk.h"
#include "onesize/onesize.h"

/**
 * Handle allocation statistics.
 * @param hnd       the allocator
 * @param subtract  the number of allocated blocks
 * @param add the number of blocks to add to statistics
 */
static void onesize_run_stats(struct onesize *hnd, int subtract, int add) {
    ASSERT(hnd->nfree >= subtract);
    hnd->nfree -= subtract;
    hnd->nfree += add;
    hnd->minfree = MIN(hnd->minfree, hnd->nfree);
}

void *onesize_allocate(struct onesize *hnd, int size) {
    ASSERT(hnd);
    ASSERT(size >= 0);

    if (size > hnd->block_size) {
        return NULL;
    }

    if (slist_is_empty(&hnd->fl)) {
        return NULL;
    }

    struct slist_item *elem = slist_pop_front(&hnd->fl);
    ASSERT(elem);

    onesize_run_stats(hnd, 1, 0);

    return elem;
}

void onesize_free(struct onesize *hnd, const void *ptr) {
    ASSERT(hnd);
    ASSERT(ptr);

    /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) */
    ASSERT(ptr >= hnd->pool.ptr);
    ASSERT(ptr < (void *)((char *)hnd->pool.ptr + hnd->pool.size));

    struct slist_item *p = A1CAST(struct slist_item *, ptr);

    slist_push_front(&hnd->fl, p);
    onesize_run_stats(hnd, 0, 1);
}

/**
 * Internal initialization routine.
 * @param hnd  the allocator
 */
static void onesize_init_internal(struct onesize *hnd) {
    slist_init(&hnd->fl);

    char *ptr = (char *)hnd->pool.ptr;
    int num = hnd->pool.size / hnd->block_size;
    for (int i = 0; i < num; i++) {
        struct slist_item *item = A1CAST(struct slist_item *, ptr);
        slist_push_front(&hnd->fl, item);
        ptr += hnd->block_size;
    }
    hnd->ntotal = hnd->nfree = hnd->minfree = num;
}

void onesize_free_all(struct onesize *hnd) {
    ASSERT(hnd);

    struct onesize *s = (struct onesize *)hnd;

    int minfree = hnd->minfree;
    onesize_init_internal(s);
    hnd->minfree = minfree;
}

void onesize_iterate_over_allocated(
    struct onesize *hnd, int num, void *ctx, onesize_iterate_func cb
) {
    ASSERT(hnd);
    ASSERT(cb);
    ASSERT(num != 0);

    struct onesize *impl = (struct onesize *)hnd;
    char *ptr = (char *)impl->pool.ptr;
    int total = impl->pool.size / impl->block_size;
    if (num < 0) {
        num = total;
    }
    int iterated = 0;
    num = MIN(num, total);
    for (int i = 0; (i < total) && (iterated < num); i++) {
        ASSERT(A1_ALIGNOF_PTR(ptr) >= A1_ALIGNOF(struct slist_item));
        struct slist_item *item = A1CAST(struct slist_item *, ptr);
        if (slist_owns(&impl->fl, item)) {
            continue; /* the item is free */
        }
        /* the item is allocated */
        cb(ctx, iterated, (char *)item, impl->block_size);
        ptr += impl->block_size;
        iterated++;
    }
}

int onesize_get_nfree(struct onesize *hnd) {
    ASSERT(hnd);
    return hnd->nfree;
}

int onesize_get_min_nfree(struct onesize *hnd) {
    ASSERT(hnd);
    return hnd->minfree;
}

int onesize_get_block_size(struct onesize *hnd) {
    ASSERT(hnd);
    return hnd->block_size;
}

int onesize_get_nblocks(struct onesize *hnd) {
    ASSERT(hnd);
    return hnd->ntotal;
}

void onesize_init(
    struct onesize *hnd, struct blk *pool, int block_size, int alignment
) {
    ASSERT(hnd);
    ASSERT(pool);
    ASSERT(pool->ptr);
    ASSERT(pool->size > 0);
    ASSERT(pool->size >= block_size);
    ASSERT(alignment >= A1_ALIGNOF(struct slist_item));

    memset(hnd, 0, sizeof(*hnd));

    void *alignedptr = A1_ALIGN_PTR_UP(pool->ptr, alignment);
    int affix = (int)((uintptr_t)alignedptr - (uintptr_t)pool->ptr);
    ASSERT(affix < pool->size);
    pool->size -= affix;
    pool->ptr = alignedptr;

    block_size = MAX(block_size, (int)sizeof(struct slist_item));
    block_size = MAX(block_size, alignment);

    ASSERT(pool->size >= block_size);

    hnd->pool = *pool;
    hnd->block_size = block_size;

    onesize_init_internal(hnd);
}
