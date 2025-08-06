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
 * onesize memory allocator implementation
 */

#include <stdint.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"
#include "common/types.h"
#include "slist/slist.h"
#include "onesize/onesize.h"

static void am_assert_memptr_validity(struct am_onesize *hnd, const void *ptr) {
    AM_ASSERT(ptr >= hnd->pool_beg);
    AM_ASSERT(ptr < hnd->pool_end);
    uintptr_t offset = (uintptr_t)((const char *)ptr - (char *)hnd->pool_beg);
    AM_ASSERT((offset % (uintptr_t)hnd->block_size) == 0);
}

void *am_onesize_allocate_x(struct am_onesize *hnd, int margin) {
    AM_ASSERT(hnd);
    AM_ASSERT(margin >= 0);

    hnd->crit_enter();

    if (hnd->nfree <= margin) {
        hnd->crit_exit();
        return NULL;
    }

    --hnd->nfree;
    hnd->nfree_min = AM_MIN(hnd->nfree_min, hnd->nfree);

    void *ptr = am_slist_pop_front(&hnd->fl);
    if (ptr) {
        /*
         * make sure that onesize freelist bookkeeping state
         * was not corrupted by someone
         */
        am_assert_memptr_validity(hnd, ptr);

        const struct am_slist_item *next = am_slist_peek_front(&hnd->fl);
        if (next) {
            am_assert_memptr_validity(hnd, next);
        }
    } else {
        AM_ASSERT(hnd->nbump < hnd->ntotal);
        ptr = (char *)hnd->pool_beg + hnd->block_size * hnd->nbump;
        ++hnd->nbump;
    }

    hnd->crit_exit();

    return ptr;
}

void *am_onesize_allocate(struct am_onesize *hnd) {
    void *ptr = am_onesize_allocate_x(hnd, /*margin=*/0);
    AM_ASSERT(ptr);
    return ptr;
}

void am_onesize_free(struct am_onesize *hnd, const void *ptr) {
    AM_ASSERT(hnd);
    AM_ASSERT(ptr);

    /* make sure the provided pointer is valid */
    am_assert_memptr_validity(hnd, ptr);

    struct am_slist_item *p = AM_CAST(struct am_slist_item *, ptr);

    hnd->crit_enter();

    const struct am_slist_item *head = am_slist_peek_front(&hnd->fl);
    if (head) {
        AM_ASSERT(head != ptr); /* double free? */
        am_assert_memptr_validity(hnd, head);
    }

    AM_ASSERT(hnd->nfree < hnd->ntotal);
    ++hnd->nfree;

    am_slist_push_front(&hnd->fl, p);

    hnd->crit_exit();
}

void am_onesize_free_all(struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();

    am_slist_ctor(&hnd->fl);
    hnd->nbump = 0;
    hnd->nfree = hnd->ntotal;

    hnd->crit_exit();
}

void am_onesize_iterate_over_allocated_unsafe(
    struct am_onesize *hnd, int num, am_onesize_iterate_fn cb, void *ctx
) {
    AM_ASSERT(hnd);
    AM_ASSERT(cb);
    AM_ASSERT(num != 0);

    char *ptr = (char *)hnd->pool_beg;
    if (num < 0) {
        num = hnd->nbump;
    }
    int iterated = 0;
    num = AM_MIN(num, hnd->nbump);

    for (int i = 0; (i < hnd->nbump) && (iterated < num); ++i) {
        AM_ASSERT(AM_ALIGNOF_PTR(ptr) >= AM_ALIGNOF(am_slist_item_t));
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        if (am_slist_owns(&hnd->fl, item)) {
            continue; /* the item is free */
        }

        cb(ctx, iterated, (char *)item, hnd->block_size);

        ptr += hnd->block_size;
        ++iterated;
    }
}

int am_onesize_get_nfree(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();
    int nfree = hnd->nfree;
    hnd->crit_exit();

    return nfree;
}

int am_onesize_get_nfree_min(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();
    int nfree_min = hnd->nfree_min;
    hnd->crit_exit();

    return nfree_min;
}

int am_onesize_get_block_size(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->block_size;
}

int am_onesize_get_nblocks(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);
    return hnd->ntotal;
}

static void am_onesize_crit_enter(void) {}
static void am_onesize_crit_exit(void) {}

void am_onesize_ctor(struct am_onesize *hnd, const struct am_onesize_cfg *cfg) {
    AM_ASSERT(hnd);
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->pool.ptr);
    AM_ASSERT(cfg->pool.size > 0);
    AM_ASSERT(cfg->block_size > 0);
    AM_ASSERT(cfg->pool.size >= cfg->block_size);

    int alignment = AM_MAX(cfg->alignment, AM_ALIGNOF(am_slist_item_t));
    AM_ASSERT(AM_ALIGNOF_PTR(cfg->pool.ptr) >= alignment);

    memset(hnd, 0, sizeof(*hnd));

    hnd->block_size =
        AM_MAX(cfg->block_size, (int)sizeof(struct am_slist_item));
    hnd->block_size = (int)AM_ALIGN_SIZE(hnd->block_size, alignment);

    AM_ASSERT(cfg->pool.size >= hnd->block_size);
    hnd->ntotal = cfg->pool.size / hnd->block_size;
    hnd->nfree = hnd->nfree_min = hnd->ntotal;
    hnd->pool_beg = cfg->pool.ptr;
    hnd->pool_end = (char *)cfg->pool.ptr + hnd->ntotal * hnd->block_size;
    hnd->nbump = 0;

    if (cfg->crit_enter && cfg->crit_exit) {
        hnd->crit_enter = cfg->crit_enter;
        hnd->crit_exit = cfg->crit_exit;
    } else {
        hnd->crit_enter = am_onesize_crit_enter;
        hnd->crit_exit = am_onesize_crit_exit;
    }

    am_slist_ctor(&hnd->fl);
}
