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
 * onesize memory allocator implementation
 */

#include <stddef.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"
#include "common/types.h"
#include "slist/slist.h"
#include "onesize/onesize.h"

void *am_onesize_allocate(struct am_onesize *hnd, int size) {
    AM_ASSERT(hnd);
    AM_ASSERT(size >= 0);

    if (size > hnd->block_size) {
        return NULL;
    }

    hnd->crit_enter();

    if (am_slist_is_empty(&hnd->fl)) {
        hnd->crit_exit();
        return NULL;
    }

    struct am_slist_item *elem = am_slist_pop_front(&hnd->fl);
    AM_ASSERT(elem);

    AM_ASSERT(hnd->nfree);
    --hnd->nfree;
    hnd->minfree = AM_MIN(hnd->minfree, hnd->nfree);

    hnd->crit_exit();

    return elem;
}

void am_onesize_free(struct am_onesize *hnd, const void *ptr) {
    AM_ASSERT(hnd);
    AM_ASSERT(ptr);

    /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) */
    AM_ASSERT(ptr >= hnd->pool.ptr);
    AM_ASSERT(ptr < (void *)((char *)hnd->pool.ptr + hnd->pool.size));

    struct am_slist_item *p = AM_CAST(struct am_slist_item *, ptr);

    hnd->crit_enter();

    AM_ASSERT(hnd->nfree < hnd->ntotal);
    ++hnd->nfree;

    am_slist_push_front(&hnd->fl, p);

    hnd->crit_exit();
}

/**
 * Internal initialization routine.
 * @param hnd  the allocator
 */
static void am_onesize_ctor_internal(struct am_onesize *hnd) {
    am_slist_init(&hnd->fl);

    char *ptr = (char *)hnd->pool.ptr;
    int num = hnd->pool.size / hnd->block_size;
    for (int i = 0; i < num; ++i) {
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        am_slist_push_front(&hnd->fl, item);
        ptr += hnd->block_size;
    }
    hnd->ntotal = hnd->nfree = hnd->minfree = num;
}

void am_onesize_free_all(struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();

    int minfree = hnd->minfree;
    am_onesize_ctor_internal(hnd);
    hnd->minfree = minfree; /* cppcheck-suppress redundantAssignment */

    hnd->crit_exit();
}

void am_onesize_iterate_over_allocated(
    struct am_onesize *hnd, int num, am_onesize_iterate_func cb, void *ctx
) {
    AM_ASSERT(hnd);
    AM_ASSERT(cb);
    AM_ASSERT(num != 0);

    char *ptr = (char *)hnd->pool.ptr;
    if (num < 0) {
        num = hnd->ntotal;
    }
    int iterated = 0;
    num = AM_MIN(num, hnd->ntotal);

    hnd->crit_enter();

    for (int i = 0; (i < hnd->ntotal) && (iterated < num); ++i) {
        AM_ASSERT(AM_ALIGNOF_PTR(ptr) >= AM_ALIGNOF_SLIST_ITEM);
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        if (am_slist_owns(&hnd->fl, item)) {
            continue; /* the item is free */
        }
        /* the item is allocated */
        hnd->crit_exit();

        cb(ctx, iterated, (char *)item, hnd->block_size);

        hnd->crit_enter();

        ptr += hnd->block_size;
        ++iterated;
    }

    hnd->crit_exit();
}

int am_onesize_get_nfree(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();

    int nfree = hnd->nfree;

    hnd->crit_exit();

    return nfree;
}

int am_onesize_get_min_nfree(const struct am_onesize *hnd) {
    AM_ASSERT(hnd);

    hnd->crit_enter();

    int minfree = hnd->minfree;

    hnd->crit_exit();

    return minfree;
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

    int alignment = AM_MAX(cfg->alignment, AM_ALIGNOF_SLIST_ITEM);
    AM_ASSERT(AM_ALIGNOF_PTR(cfg->pool.ptr) >= alignment);

    memset(hnd, 0, sizeof(*hnd));

    hnd->pool = cfg->pool;
    hnd->block_size =
        AM_MAX(cfg->block_size, (int)sizeof(struct am_slist_item));
    hnd->block_size = AM_ALIGN_SIZE(hnd->block_size, alignment);

    AM_ASSERT(hnd->pool.size >= hnd->block_size);

    if (cfg->crit_enter && cfg->crit_exit) {
        hnd->crit_enter = cfg->crit_enter;
        hnd->crit_exit = cfg->crit_exit;
    } else {
        hnd->crit_enter = am_onesize_crit_enter;
        hnd->crit_exit = am_onesize_crit_exit;
    }

    am_onesize_ctor_internal(hnd);
}
