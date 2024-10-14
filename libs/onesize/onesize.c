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
static void am_onesize_run_stats(
    struct am_onesize *hnd, int subtract, int add
) {
    AM_ASSERT(hnd->nfree >= subtract);
    hnd->nfree -= subtract;
    hnd->nfree += add;
    hnd->minfree = AM_MIN(hnd->minfree, hnd->nfree);
}

void *am_onesize_allocate(struct am_onesize *hnd, int size) {
    AM_ASSERT(hnd);
    AM_ASSERT(size >= 0);

    if (size > hnd->block_size) {
        return NULL;
    }

    if (am_slist_is_empty(&hnd->fl)) {
        return NULL;
    }

    struct am_slist_item *elem = am_slist_pop_front(&hnd->fl);
    AM_ASSERT(elem);

    am_onesize_run_stats(hnd, 1, 0);

    return elem;
}

void am_onesize_free(struct am_onesize *hnd, const void *ptr) {
    AM_ASSERT(hnd);
    AM_ASSERT(ptr);

    /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) */
    AM_ASSERT(ptr >= hnd->pool.ptr);
    AM_ASSERT(ptr < (void *)((char *)hnd->pool.ptr + hnd->pool.size));

    struct am_slist_item *p = AM_CAST(struct am_slist_item *, ptr);

    am_slist_push_front(&hnd->fl, p);
    am_onesize_run_stats(hnd, 0, 1);
}

/**
 * Internal initialization routine.
 * @param hnd  the allocator
 */
static void am_onesize_init_internal(struct am_onesize *hnd) {
    am_slist_init(&hnd->fl);

    char *ptr = (char *)hnd->pool.ptr;
    int num = hnd->pool.size / hnd->block_size;
    for (int i = 0; i < num; i++) {
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        am_slist_push_front(&hnd->fl, item);
        ptr += hnd->block_size;
    }
    hnd->ntotal = hnd->nfree = hnd->minfree = num;
}

void am_onesize_free_all(struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    struct am_onesize *s = (struct am_onesize *)hnd;

    int minfree = hnd->minfree;
    am_onesize_init_internal(s);
    hnd->minfree = minfree;
}

void am_onesize_iterate_over_allocated(
    struct am_onesize *hnd, int num, void *ctx, am_onesize_iterate_func cb
) {
    AM_ASSERT(hnd);
    AM_ASSERT(cb);
    AM_ASSERT(num != 0);

    struct am_onesize *impl = (struct am_onesize *)hnd;
    char *ptr = (char *)impl->pool.ptr;
    int total = impl->pool.size / impl->block_size;
    if (num < 0) {
        num = total;
    }
    int iterated = 0;
    num = AM_MIN(num, total);
    for (int i = 0; (i < total) && (iterated < num); i++) {
        AM_ASSERT(AM_ALIGNOF_PTR(ptr) >= AM_ALIGNOF(struct am_slist_item));
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        if (am_slist_owns(&impl->fl, item)) {
            continue; /* the item is free */
        }
        /* the item is allocated */
        cb(ctx, iterated, (char *)item, impl->block_size);
        ptr += impl->block_size;
        iterated++;
    }
}

int am_onesize_get_nfree(struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->nfree;
}

int am_onesize_get_min_nfree(struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->minfree;
}

int am_onesize_get_block_size(struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->block_size;
}

int am_onesize_get_nblocks(struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->ntotal;
}

void am_onesize_init(
    struct am_onesize *hnd, struct am_blk *pool, int block_size, int alignment
) {
    AM_ASSERT(hnd);
    AM_ASSERT(pool);
    AM_ASSERT(pool->ptr);
    AM_ASSERT(pool->size > 0);
    AM_ASSERT(pool->size >= block_size);
    AM_ASSERT(alignment >= AM_ALIGNOF(struct am_slist_item));

    memset(hnd, 0, sizeof(*hnd));

    void *alignedptr = AM_ALIGN_PTR_UP(pool->ptr, alignment);
    int affix = (int)((uintptr_t)alignedptr - (uintptr_t)pool->ptr);
    AM_ASSERT(affix < pool->size);
    pool->size -= affix;
    pool->ptr = alignedptr;

    block_size = AM_MAX(block_size, (int)sizeof(struct am_slist_item));
    block_size = AM_MAX(block_size, alignment);

    AM_ASSERT(pool->size >= block_size);

    hnd->pool = *pool;
    hnd->block_size = block_size;

    am_onesize_init_internal(hnd);
}
