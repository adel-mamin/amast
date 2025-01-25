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
#include <stdint.h>
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

    hnd->crit_enter();

    int minfree = hnd->minfree;
    am_onesize_ctor_internal(s);
    hnd->minfree = minfree; /* cppcheck-suppress redundantAssignment */

    hnd->crit_exit();
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

    hnd->crit_enter();

    for (int i = 0; (i < total) && (iterated < num); i++) {
        AM_ASSERT(AM_ALIGNOF_PTR(ptr) >= AM_ALIGNOF_SLIST_ITEM);
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        if (am_slist_owns(&impl->fl, item)) {
            continue; /* the item is free */
        }
        /* the item is allocated */
        hnd->crit_exit();
        cb(ctx, iterated, (char *)item, impl->block_size);
        hnd->crit_enter();
        ptr += impl->block_size;
        iterated++;
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

void am_onesize_ctor(struct am_onesize *hnd, struct am_onesize_cfg *cfg) {
    AM_ASSERT(hnd);
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->pool);
    AM_ASSERT(cfg->pool->ptr);
    AM_ASSERT(cfg->pool->size > 0);
    AM_ASSERT(cfg->pool->size >= cfg->block_size);
    AM_ASSERT(cfg->alignment >= AM_ALIGNOF_SLIST_ITEM);

    memset(hnd, 0, sizeof(*hnd));

    void *aligned_ptr = AM_ALIGN_PTR_UP(cfg->pool->ptr, cfg->alignment);
    int affix = (int)((uintptr_t)aligned_ptr - (uintptr_t)cfg->pool->ptr);
    AM_ASSERT(affix < cfg->pool->size);
    cfg->pool->size -= affix;
    cfg->pool->ptr = aligned_ptr;

    cfg->block_size =
        AM_MAX(cfg->block_size, (int)sizeof(struct am_slist_item));
    cfg->block_size = AM_MAX(cfg->block_size, cfg->alignment);

    AM_ASSERT(cfg->pool->size >= cfg->block_size);

    hnd->pool = *cfg->pool;
    hnd->block_size = cfg->block_size;

    if (cfg->crit_enter && cfg->crit_exit) {
        hnd->crit_enter = cfg->crit_enter;
        hnd->crit_exit = cfg->crit_exit;
    } else {
        hnd->crit_enter = am_onesize_crit_enter;
        hnd->crit_exit = am_onesize_crit_exit;
    }

    am_onesize_ctor_internal(hnd);
}
