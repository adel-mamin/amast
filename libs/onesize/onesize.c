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

void *am_onesize_allocate(struct am_onesize *me, int size) {
    AM_ASSERT(me);
    AM_ASSERT(size >= 0);

    if (size > me->block_size) {
        return NULL;
    }

    me->crit_enter();

    if (am_slist_is_empty(&me->fl)) {
        me->crit_exit();
        return NULL;
    }

    struct am_slist_item *elem = am_slist_pop_front(&me->fl);
    AM_ASSERT(elem);

    AM_ASSERT(me->nfree);
    --me->nfree;
    me->minfree = AM_MIN(me->minfree, me->nfree);

    me->crit_exit();

    return elem;
}

void am_onesize_free(struct am_onesize *me, const void *ptr) {
    AM_ASSERT(me);
    AM_ASSERT(ptr);

    /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) */
    AM_ASSERT(ptr >= me->pool.ptr);
    AM_ASSERT(ptr < (void *)((char *)me->pool.ptr + me->pool.size));

    struct am_slist_item *p = AM_CAST(struct am_slist_item *, ptr);

    me->crit_enter();

    AM_ASSERT(me->nfree < me->ntotal);
    ++me->nfree;

    am_slist_push_front(&me->fl, p);

    me->crit_exit();
}

/**
 * Internal initialization routine.
 * @param me  the allocator
 */
static void am_onesize_ctor_internal(struct am_onesize *me) {
    am_slist_init(&me->fl);

    char *ptr = (char *)me->pool.ptr;
    int num = me->pool.size / me->block_size;
    for (int i = 0; i < num; ++i) {
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        am_slist_push_front(&me->fl, item);
        ptr += me->block_size;
    }
    me->ntotal = me->nfree = me->minfree = num;
}

void am_onesize_free_all(struct am_onesize *me) {
    AM_ASSERT(me);

    struct am_onesize *s = (struct am_onesize *)me;

    me->crit_enter();

    int minfree = me->minfree;
    am_onesize_ctor_internal(s);
    me->minfree = minfree; /* cppcheck-suppress redundantAssignment */

    me->crit_exit();
}

void am_onesize_iterate_over_allocated(
    struct am_onesize *me, int num, am_onesize_iterate_func cb, void *ctx
) {
    AM_ASSERT(me);
    AM_ASSERT(cb);
    AM_ASSERT(num != 0);

    char *ptr = (char *)me->pool.ptr;
    if (num < 0) {
        num = me->ntotal;
    }
    int iterated = 0;
    num = AM_MIN(num, me->ntotal);

    me->crit_enter();

    for (int i = 0; (i < me->ntotal) && (iterated < num); ++i) {
        AM_ASSERT(AM_ALIGNOF_PTR(ptr) >= AM_ALIGNOF_SLIST_ITEM);
        struct am_slist_item *item = AM_CAST(struct am_slist_item *, ptr);
        if (am_slist_owns(&me->fl, item)) {
            continue; /* the item is free */
        }
        /* the item is allocated */
        me->crit_exit();

        cb(ctx, iterated, (char *)item, me->block_size);

        me->crit_enter();

        ptr += me->block_size;
        ++iterated;
    }

    me->crit_exit();
}

int am_onesize_get_nfree(const struct am_onesize *me) {
    AM_ASSERT(me);

    me->crit_enter();

    int nfree = me->nfree;

    me->crit_exit();

    return nfree;
}

int am_onesize_get_min_nfree(const struct am_onesize *me) {
    AM_ASSERT(me);

    me->crit_enter();

    int minfree = me->minfree;

    me->crit_exit();

    return minfree;
}

int am_onesize_get_block_size(const struct am_onesize *me) {
    AM_ASSERT(me);
    return me->block_size;
}

int am_onesize_get_nblocks(const struct am_onesize *me) {
    AM_ASSERT(me);
    return me->ntotal;
}

static void am_onesize_crit_enter(void) {}
static void am_onesize_crit_exit(void) {}

void am_onesize_ctor(struct am_onesize *me, const struct am_onesize_cfg *cfg) {
    AM_ASSERT(me);
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->pool.ptr);
    AM_ASSERT(cfg->pool.size > 0);
    AM_ASSERT(cfg->block_size > 0);
    AM_ASSERT(cfg->pool.size >= cfg->block_size);

    int alignment = AM_MAX(cfg->alignment, AM_ALIGNOF_SLIST_ITEM);
    AM_ASSERT(AM_ALIGNOF_PTR(cfg->pool.ptr) >= alignment);

    memset(me, 0, sizeof(*me));

    me->pool = cfg->pool;
    me->block_size = AM_MAX(cfg->block_size, (int)sizeof(struct am_slist_item));
    me->block_size = AM_ALIGN_SIZE(me->block_size, alignment);

    AM_ASSERT(me->pool.size >= me->block_size);

    if (cfg->crit_enter && cfg->crit_exit) {
        me->crit_enter = cfg->crit_enter;
        me->crit_exit = cfg->crit_exit;
    } else {
        me->crit_enter = am_onesize_crit_enter;
        me->crit_exit = am_onesize_crit_exit;
    }

    am_onesize_ctor_internal(me);
}
