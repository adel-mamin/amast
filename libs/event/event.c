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
 *
 * Event API implementation.
 */

#include <string.h>
#include <stddef.h>

#include "common/compiler.h" /* IWYU pragma: keep */
#include "common/alignment.h"
#include "common/macros.h"
#include "onesize/onesize.h"
#include "blk/blk.h"
#include "pal/pal.h"
#include "event.h"

#define AM_EVENT_IS_STATIC(event) (0 == (event)->pool_index)

/** Event internal state. */
struct am_event_state {
    /** user defined event memory pools  */
    struct am_onesize pool[AM_EVENT_POOL_NUM_MAX];
    /** the number of user defined event memory pools */
    int npool;
};

static struct am_event_state am_event_state_;

void am_event_add_pool(void *pool, int size, int block_size, int alignment) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(me->npool < AM_EVENT_POOL_NUM_MAX);
    if (me->npool > 0) {
        int prev_size = am_onesize_get_block_size(&me->pool[me->npool - 1]);
        AM_ASSERT(block_size > prev_size);
    }

    struct am_blk blk;
    memset(&blk, 0, sizeof(blk));
    blk.ptr = pool;
    blk.size = size;
    am_onesize_init(&me->pool[me->npool], &blk, block_size, alignment);

    ++me->npool;
}

struct am_event *am_event_allocate(int id, int size, int margin) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(size > 0);
    AM_ASSERT(me->npool);
    AM_ASSERT(size <= am_onesize_get_block_size(&me->pool[me->npool - 1]));
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    for (int i = 0; i < me->npool; i++) {
        struct am_onesize *osz = &me->pool[i];
        if (size > am_onesize_get_block_size(osz)) {
            continue;
        }

        am_pal_crit_enter();

        int nfree = am_onesize_get_nfree(osz);
        if (margin && (nfree <= margin)) {
            am_pal_crit_exit();
            break;
        }
        if (!nfree) {
            am_pal_crit_exit();
            continue;
        }
        struct am_event *event =
            (struct am_event *)am_onesize_allocate(osz, size);
        am_pal_crit_exit();

        AM_ASSERT(event);

        /* event pointer is guaranteeed to be non-NULL here */
        AM_DISABLE_WARNING(AM_W_NULL_DEREFERENCE);
        memset(event, 0, sizeof(*event));
        event->id = id;
        event->pool_index =
            (unsigned)i & ((1U << AM_EVENT_POOL_INDEX_BITS) - 1);
        AM_ENABLE_WARNING(AM_W_NULL_DEREFERENCE);

        return event;
    }

    AM_ASSERT(margin);

    return NULL;
}

void am_event_free(const struct am_event *event) {
    if (NULL == event) {
        return;
    }
    if (AM_EVENT_IS_STATIC(event)) {
        return; /* the event is statically allocated */
    }

    am_pal_crit_enter();

    if (event->ref_counter > 1) {
        --AM_CAST(struct am_event *, event)->ref_counter;
        am_pal_crit_exit();
        return;
    }

    AM_ASSERT(event->pool_index <= AM_EVENT_POOL_NUM_MAX);
    am_onesize_free(&am_event_state_.pool[event->pool_index - 1], event);

    am_pal_crit_exit();
}

int am_event_get_pool_min_nfree(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_min_nfree(&me->pool[index]);
}

int am_event_get_pool_nfree_now(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_nfree(&me->pool[index]);
}

int am_event_get_pool_nblocks(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_nblocks(&me->pool[index]);
}

int am_event_get_pools_num(void) { return am_event_state_.npool; }

struct am_event *am_event_dup(
    const struct am_event *event, int size, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(size >= (int)sizeof(struct am_event));
    const struct am_event_state *me = &am_event_state_;
    AM_ASSERT(me->npool > 0);
    AM_ASSERT(event->id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    // cppcheck-suppress nullPointerRedundantCheck
    struct am_event *dup = am_event_allocate(event->id, size, margin);
    if (dup && (size > (int)sizeof(struct am_event))) {
        char *dst = (char *)dup + sizeof(*dup);
        const char *src = (const char *)event + sizeof(*event);
        int sz = size - (int)sizeof(struct am_event);
        memcpy(dst, src, (size_t)sz);
    }

    return dup;
}

/** Event log context. */
struct am_event_log_ctx {
    am_event_log_func cb; /**< event log callback */
    int pool_ind;         /**< event pool index */
};

/**
 * A helper callback to be the type am_onesize_iterate_func
 *
 * @param ctx    an instance of struct am_event_log_ctx
 * @param index  the index of event buffer to log
 * @param buf    the event buffer to log
 * @param size   the size of event buffer to log [bytes]
 */
static void am_event_log_cb(void *ctx, int index, const char *buf, int size) {
    AM_ASSERT(ctx);
    AM_ASSERT(buf);
    AM_ASSERT(AM_ALIGNOF_PTR(buf) >= AM_ALIGNOF(struct am_event));
    AM_ASSERT(size >= (int)sizeof(struct am_event));

    struct am_event_log_ctx *log = (struct am_event_log_ctx *)ctx;
    AM_ASSERT(log->cb);
    AM_ASSERT(log->pool_ind >= 0);

    AM_DISABLE_WARNING(AM_W_CAST_ALIGN);
    const struct am_event *event = (const struct am_event *)buf;
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN);
    log->cb(log->pool_ind, index, event, size);
}

void am_event_log_pools(int num, am_event_log_func cb) {
    AM_ASSERT(num > 0);
    AM_ASSERT(cb);

    struct am_event_state *me = &am_event_state_;
    struct am_event_log_ctx ctx = {.cb = cb};
    for (int i = 0; i < me->npool; i++) {
        ctx.pool_ind = i;
        am_onesize_iterate_over_allocated(
            &me->pool[i], num, &ctx, am_event_log_cb
        );
    }
}

void am_event_inc_ref_cnt(struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(event->ref_counter < AM_EVENT_REF_COUNTER_MASK);
    ++event->ref_counter;
}

void am_event_dec_ref_cnt(struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(event->ref_counter > 0);
    --event->ref_counter;
}

int am_event_get_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);
    return event->ref_counter;
}
