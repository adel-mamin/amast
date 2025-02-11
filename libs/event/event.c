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
 *
 * Event API implementation.
 */

#include <string.h>
#include <stddef.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "onesize/onesize.h"
#include "event.h"

/** Event internal state. */
struct am_event_state {
    /** user defined event memory pools  */
    struct am_onesize pool[AM_EVENT_POOLS_NUM_MAX];
    /** the number of user defined event memory pools */
    int npool;
    /** enter critical section */
    void (*crit_enter)(void);
    /** exit critical section */
    void (*crit_exit)(void);
};

static struct am_event_state am_event_state_;

struct am_alignof_event {
    char c;            /* cppcheck-suppress unusedStructMember */
    struct am_event d; /* cppcheck-suppress unusedStructMember */
};
const int am_alignof_event = offsetof(struct am_alignof_event, d);

struct am_alignof_event_ptr {
    char c;             /* cppcheck-suppress unusedStructMember */
    struct am_event *d; /* cppcheck-suppress unusedStructMember */
};
const int am_alignof_event_ptr = offsetof(struct am_alignof_event_ptr, d);

void am_event_state_ctor(const struct am_event_state_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->crit_enter);
    AM_ASSERT(cfg->crit_exit);

    struct am_event_state *me = &am_event_state_;
    memset(me, 0, sizeof(*me));

    me->crit_enter = cfg->crit_enter;
    me->crit_exit = cfg->crit_exit;
}

void am_event_add_pool(void *pool, int size, int block_size, int alignment) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(me->npool < AM_EVENT_POOLS_NUM_MAX);
    if (me->npool > 0) {
        int prev_size = am_onesize_get_block_size(&me->pool[me->npool - 1]);
        AM_ASSERT(block_size > prev_size);
    }

    struct am_onesize_cfg cfg = {
        .pool = {.ptr = pool, .size = size},
        .block_size = block_size,
        .alignment = alignment
    };
    am_onesize_ctor(&me->pool[me->npool], &cfg);

    ++me->npool;
}

struct am_event *am_event_allocate_x(int id, int size, int margin) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(size > 0);
    AM_ASSERT(me->npool);
    AM_ASSERT(size <= am_onesize_get_block_size(&me->pool[me->npool - 1]));
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    for (int i = 0; i < me->npool; ++i) {
        struct am_onesize *osz = &me->pool[i];
        if (size > am_onesize_get_block_size(osz)) {
            continue;
        }

        me->crit_enter();

        struct am_event *event = am_onesize_allocate_x(osz, margin);

        me->crit_exit();

        if (!event) {
            return NULL;
        }

        memset(event, 0, sizeof(*event));
        event->id = id;
        event->pool_index_plus_one =
            (unsigned)(i + 1) & AM_EVENT_POOL_INDEX_MASK;

        return event;
    }

    return NULL;
}

struct am_event *am_event_allocate(int id, int size) {
    struct am_event *event = am_event_allocate_x(id, size, /*margin=*/0);
    AM_ASSERT(event);

    return event;
}

void am_event_free(const struct am_event **event) {
    AM_ASSERT(event);
    AM_ASSERT(*event); /* double free? */

    if (am_event_is_static(*event)) {
        return; /* the event is statically allocated */
    }

    struct am_event *e = AM_CAST(struct am_event *, *event);
    AM_ASSERT(e->pool_index_plus_one <= AM_EVENT_POOLS_NUM_MAX);

    struct am_event_state *me = &am_event_state_;
    me->crit_enter();

    if (e->ref_counter > 1) {
        --e->ref_counter;
        me->crit_exit();
        return;
    }

    am_onesize_free(&am_event_state_.pool[e->pool_index_plus_one - 1], e);

    me->crit_exit();

    *event = NULL;
}

int am_event_get_pool_nfree_min(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);

    me->crit_enter();
    int nfree = am_onesize_get_nfree_min(&me->pool[index]);
    me->crit_exit();
    return nfree;
}

int am_event_get_pool_nfree(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);

    me->crit_enter();
    int nfree = am_onesize_get_nfree(&me->pool[index]);
    me->crit_exit();
    return nfree;
}

int am_event_get_pool_nblocks(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);

    me->crit_enter();
    int nblocks = am_onesize_get_nblocks(&me->pool[index]);
    me->crit_exit();
    return nblocks;
}

int am_event_get_pools_num(void) { return am_event_state_.npool; }

struct am_event *am_event_dup_x(
    const struct am_event *event, int size, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(size >= (int)sizeof(struct am_event));
    const struct am_event_state *me = &am_event_state_;
    AM_ASSERT(me->npool > 0);
    AM_ASSERT(event->id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    struct am_event *dup = am_event_allocate_x(event->id, size, margin);
    if (dup && (size > (int)sizeof(struct am_event))) {
        char *dst = (char *)dup + sizeof(struct am_event);
        const char *src = (const char *)event + sizeof(struct am_event);
        int sz = size - (int)sizeof(struct am_event);
        memcpy(dst, src, (size_t)sz);
    }

    return dup;
}

struct am_event *am_event_dup(const struct am_event *event, int size) {
    struct am_event *dup = am_event_dup_x(event, size, /*margin=*/0);
    AM_ASSERT(dup);

    return dup;
}

/** Event log context. */
struct am_event_log_ctx {
    am_event_log_fn cb; /**< event log callback */
    int pool_ind;       /**< event pool index */
};

/**
 * A helper callback of type \ref am_onesize_iterate_fn.
 *
 * Expected to be used together with am_onesize_iterate_over_allocated()
 *
 * @param ctx    an instance of struct am_event_log_ctx
 * @param index  the index of event buffer to log
 * @param buf    the event buffer to log
 * @param size   the size of event buffer to log [bytes]
 */
static void am_event_log_cb(void *ctx, int index, const char *buf, int size) {
    AM_ASSERT(ctx);
    AM_ASSERT(buf);
    AM_ASSERT(AM_ALIGNOF_PTR(buf) >= AM_ALIGNOF_EVENT);
    AM_ASSERT(size >= (int)sizeof(struct am_event));

    struct am_event_log_ctx *log = (struct am_event_log_ctx *)ctx;
    AM_ASSERT(log->cb);
    AM_ASSERT(log->pool_ind >= 0);

    AM_DISABLE_WARNING(AM_W_CAST_ALIGN);
    const struct am_event *event = (const struct am_event *)buf;
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN);
    log->cb(log->pool_ind, index, event, size);
}

void am_event_log_pools(int num, am_event_log_fn cb) {
    AM_ASSERT(num != 0);
    AM_ASSERT(cb);

    struct am_event_state *me = &am_event_state_;
    struct am_event_log_ctx ctx = {.cb = cb};
    for (int i = 0; i < me->npool; ++i) {
        ctx.pool_ind = i;
        am_onesize_iterate_over_allocated(
            &me->pool[i], num, am_event_log_cb, &ctx
        );
    }
}

bool am_event_is_static(const struct am_event *event) {
    AM_ASSERT(event);

    return (0 == (event->pool_index_plus_one & AM_EVENT_POOL_INDEX_MASK));
}

void am_event_inc_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);

    if (am_event_is_static(event)) {
        return;
    }

    AM_ASSERT(event->ref_counter < AM_EVENT_REF_COUNTER_MASK);

    struct am_event *e = AM_CAST(struct am_event *, event);
    struct am_event_state *me = &am_event_state_;
    me->crit_enter();
    ++e->ref_counter;
    me->crit_exit();
}

void am_event_dec_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);
    am_event_free(&event);
}

int am_event_get_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);

    struct am_event_state *me = &am_event_state_;
    me->crit_enter();
    int cnt = event->ref_counter;
    me->crit_exit();
    return cnt;
}

/**
 * Data push callback type.
 *
 * @param queue  push data to this queue
 * @param ptr    the data to push
 * @param size   the data size [bytes]
 *
 * @retval true   the data was pushed successfully
 * @retval false  the data push failed
 */
typedef bool (*am_push_fn)(struct am_queue *queue, const void *ptr, int size);

static enum am_event_rc am_event_push_x(
    struct am_queue *queue,
    const struct am_event *event,
    int margin,
    am_push_fn push
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);
    const int capacity = am_queue_get_capacity(queue);
    AM_ASSERT(margin < capacity);
    AM_ASSERT(push);

    struct am_event *e = AM_CAST(struct am_event *, event);
    struct am_event_state *me = &am_event_state_;

    me->crit_enter();

    int nfree = am_queue_get_nfree(queue);
    if (margin && (nfree <= margin)) {
        me->crit_exit();
        am_event_free(&event);
        return AM_EVENT_RC_ERR;
    }
    AM_ASSERT(nfree > 0);

    if (!am_event_is_static(e)) {
        ++e->ref_counter;
    }

    bool rc = push(queue, &event, sizeof(struct am_event));

    me->crit_exit();

    AM_ASSERT(true == rc);

    if (capacity == nfree) {
        return AM_EVENT_RC_OK_QUEUE_WAS_EMPTY;
    }
    return AM_EVENT_RC_OK;
}

enum am_event_rc am_event_push_back_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    return am_event_push_x(queue, event, margin, am_queue_push_back);
}

enum am_event_rc am_event_push_back(
    struct am_queue *queue, const struct am_event *event
) {
    return am_event_push_back_x(queue, event, /*margin=*/0);
}

enum am_event_rc am_event_push_front_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    return am_event_push_x(queue, event, margin, am_queue_push_front);
}

enum am_event_rc am_event_push_front(
    struct am_queue *queue, const struct am_event *event
) {
    return am_event_push_front_x(queue, event, /*margin=*/0);
}

const struct am_event *am_event_pop_front(struct am_queue *queue) {
    AM_ASSERT(queue);

    struct am_event_state *me = &am_event_state_;

    me->crit_enter();

    const struct am_event **e;
    e = (const struct am_event **)am_queue_pop_front(queue);

    me->crit_exit();

    if (!e) {
        return NULL;
    }

    AM_ASSERT(*e);

    return *e;
}

void am_event_defer(struct am_queue *queue, const struct am_event *event) {
    am_event_push_back(queue, event);
}

bool am_event_defer_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    return am_event_push_back_x(queue, event, margin);
}

bool am_event_recall(struct am_queue *queue, am_event_recall_fn cb, void *ctx) {
    AM_ASSERT(queue);
    AM_ASSERT(cb);

    struct am_event_state *me = &am_event_state_;

    me->crit_enter();

    const struct am_event **event =
        (const struct am_event **)am_queue_pop_front(queue);
    me->crit_exit();
    if (NULL == event) {
        return false;
    }
    const struct am_event *e = *event;
    AM_ASSERT(e);
    cb(ctx, e);

    if (am_event_is_static(e)) {
        return true;
    }

    am_event_free(event);

    return true;
}

int am_event_flush_queue(struct am_queue *queue) {
    struct am_event **event = NULL;
    int cnt = 0;
    struct am_event_state *me = &am_event_state_;

    me->crit_enter();

    while ((event = (struct am_event **)am_queue_pop_front(queue)) != NULL) {
        me->crit_exit();
        ++cnt;
        const struct am_event *e = *event;
        AM_ASSERT(e);
        am_event_free(&e);
        me->crit_enter();
    }

    me->crit_exit();

    return cnt;
}
