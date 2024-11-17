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

#include "common/alignment.h"
#include "common/macros.h"
#include "onesize/onesize.h"
#include "blk/blk.h"
#include "event.h"

#define AM_EVENT_IS_STATIC(event) (0 == (event)->pool_index)

/** Event internal state. */
struct am_event_state {
    /** user defined event memory pools  */
    struct am_onesize pool[AM_EVENT_POOL_NUM_MAX];
    /** the number of user defined event memory pools */
    int npool;
    /** push event to the front of owner event queue */
    void (*push_front)(void *owner, const struct am_event *event);
    /** notify owner about event queue is busy */
    void (*notify_event_queue_busy)(void *owner);
    /** notify owner about event queue is empty */
    void (*notify_event_queue_empty)(void *owner);
    /** wait owner notification about event queue is busy */
    void (*wait_event_queue_busy)(void *owner);
    /** enter critical section */
    void (*crit_enter)(void);
    /** exit critical section */
    void (*crit_exit)(void);
};

static struct am_event_state event_state_;

static void am_push_front_stub(void *owner, const struct am_event *event) {
    (void)owner;
    (void)event;
}
static void am_notify_event_queue_busy_stub(void *owner) { (void)owner; }
static void am_notify_event_queue_empty_stub(void *owner) { (void)owner; }
static void am_wait_event_queue_busy_stub(void *owner) { (void)owner; }
static void am_event_crit_enter_stub(void) {}
static void am_event_crit_exit_stub(void) {}

void am_event_state_ctor(const struct am_event_cfg *cfg) {
    AM_ASSERT(cfg);

    struct am_event_state *me = &event_state_;
    me->push_front = cfg->push_front;
    me->notify_event_queue_busy = cfg->notify_event_queue_busy;
    me->notify_event_queue_empty = cfg->notify_event_queue_empty;
    me->wait_event_queue_busy = cfg->wait_event_queue_busy;
    me->crit_enter = cfg->crit_enter;
    me->crit_exit = cfg->crit_exit;

    if (!me->push_front) {
        me->push_front = am_push_front_stub;
    }
    if (!me->notify_event_queue_busy) {
        me->notify_event_queue_busy = am_notify_event_queue_busy_stub;
    }
    if (!me->notify_event_queue_empty) {
        me->notify_event_queue_empty = am_notify_event_queue_empty_stub;
    }
    if (!me->wait_event_queue_busy) {
        me->wait_event_queue_busy = am_wait_event_queue_busy_stub;
    }
    if (!me->crit_enter || !me->crit_exit) {
        me->crit_enter = am_event_crit_enter_stub;
        me->crit_exit = am_event_crit_exit_stub;
    }
}

void am_event_add_pool(void *pool, int size, int block_size, int alignment) {
    struct am_event_state *me = &event_state_;
    AM_ASSERT(me->npool < AM_EVENT_POOL_NUM_MAX);
    if (me->npool > 0) {
        int prev_size = am_onesize_get_block_size(&me->pool[me->npool - 1]);
        AM_ASSERT(block_size > prev_size);
    }

    struct am_blk blk;
    memset(&blk, 0, sizeof(blk));
    blk.ptr = pool;
    blk.size = size;
    struct am_onesize_cfg cfg = {
        .pool = &blk, .block_size = block_size, .alignment = alignment
    };
    am_onesize_ctor(&me->pool[me->npool], &cfg);

    ++me->npool;
}

struct am_event *am_event_allocate(int id, int size, int margin) {
    struct am_event_state *me = &event_state_;
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

        me->crit_enter();

        int nfree = am_onesize_get_nfree(osz);
        if (margin && (nfree <= margin)) {
            me->crit_exit();
            break;
        }
        if (!nfree) {
            me->crit_exit();
            continue;
        }
        struct am_event *event =
            (struct am_event *)am_onesize_allocate(osz, size);
        me->crit_exit();

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

    struct am_event_state *me = &event_state_;
    me->crit_enter();

    if (event->ref_counter > 1) {
        --AM_CAST(struct am_event *, event)->ref_counter;
        me->crit_exit();
        return;
    }

    AM_ASSERT(event->pool_index <= AM_EVENT_POOL_NUM_MAX);
    am_onesize_free(&event_state_.pool[event->pool_index - 1], event);

    me->crit_exit();
}

int am_event_get_pool_min_nfree(int index) {
    struct am_event_state *me = &event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_min_nfree(&me->pool[index]);
}

int am_event_get_pool_nfree_now(int index) {
    struct am_event_state *me = &event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_nfree(&me->pool[index]);
}

int am_event_get_pool_nblocks(int index) {
    struct am_event_state *me = &event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npool);
    return am_onesize_get_nblocks(&me->pool[index]);
}

int am_event_get_pools_num(void) { return event_state_.npool; }

struct am_event *am_event_dup(
    const struct am_event *event, int size, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(size >= (int)sizeof(struct am_event));
    const struct am_event_state *me = &event_state_;
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

    struct am_event_state *me = &event_state_;
    struct am_event_log_ctx ctx = {.cb = cb};
    for (int i = 0; i < me->npool; i++) {
        ctx.pool_ind = i;
        am_onesize_iterate_over_allocated(
            &me->pool[i], num, &ctx, am_event_log_cb
        );
    }
}

bool am_event_push_back_x(
    void *owner,
    struct am_queue *queue,
    const struct am_event *event,
    int margin
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);
    const int capacity = am_queue_capacity(queue);
    AM_ASSERT(margin < capacity);

    struct am_event_state *me = &event_state_;
    me->crit_enter();

    const int len = am_queue_length(queue);
    AM_ASSERT(capacity >= len);
    if (margin && ((capacity - len) <= margin)) {
        me->crit_exit();
        am_event_free(event);
        return false;
    }

    if (!am_event_is_static(event)) {
        am_event_inc_ref_cnt(AM_CAST(struct am_event *, event));
    }

    bool wasempty = am_queue_is_empty(queue);
    bool rc = am_queue_push_back(queue, &event, sizeof(event));
    AM_ASSERT(true == rc);

    if (wasempty && owner) {
        me->notify_event_queue_busy(owner);
    }

    me->crit_exit();

    return true;
}

void am_event_push_back(
    void *owner, struct am_queue *queue, const struct am_event *event
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);

    (void)am_event_push_back_x(owner, queue, event, /*margin=*/0);
}

void am_event_push_front(
    void *owner, struct am_queue *queue, const struct am_event *event
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);

    struct am_event_state *me = &event_state_;
    me->crit_enter();

    if (!am_event_is_static(event)) {
        am_event_inc_ref_cnt(AM_CAST(struct am_event *, event));
    }

    bool wasempty = am_queue_is_empty(queue);
    bool rc = am_queue_push_front(queue, &event, sizeof(event));
    AM_ASSERT(true == rc);

    if (wasempty && owner) {
        me->notify_event_queue_busy(owner);
    }

    me->crit_exit();
}

const struct am_event *am_event_pop_front(void *owner, struct am_queue *queue) {
    AM_ASSERT(queue);

    struct am_event_state *me = &event_state_;
    me->crit_enter();
    me->wait_event_queue_busy(owner);

    const struct am_event **e;
    e = (const struct am_event **)am_queue_pop_front(queue);
    if (am_queue_is_empty(queue)) {
        me->notify_event_queue_empty(owner);
    }

    me->crit_exit();

    AM_ASSERT(e);
    AM_ASSERT(*e);

    return *e;
}

void am_event_defer(struct am_queue *queue, const struct am_event *event) {
    AM_ASSERT(queue);
    AM_ASSERT(event);

    am_event_push_back(/*owner=*/NULL, queue, event);
}

void am_event_defer_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);

    const int capacity = am_queue_capacity(queue);
    AM_ASSERT(margin < capacity);

    am_event_push_back_x(/*owner=*/NULL, queue, event, margin);
}

const struct am_event *am_event_recall(void *owner, struct am_queue *queue) {
    struct am_event_state *me = &event_state_;
    me->crit_enter();

    struct am_event **event = (struct am_event **)am_queue_pop_front(queue);
    me->crit_exit();
    if (NULL == event) {
        return NULL;
    }
    struct am_event *e = *event;
    AM_ASSERT(e);
    me->push_front(owner, e);
    if (am_event_is_static(e)) {
        return e;
    }

    me->crit_enter();

    AM_ASSERT(am_event_get_ref_cnt(e) > 1);
    am_event_dec_ref_cnt(e);

    me->crit_exit();

    return e;
}

int am_event_flush_queue(struct am_queue *queue) {
    struct am_event **event = NULL;
    int cnt = 0;
    struct am_event_state *me = &event_state_;

    me->crit_enter();

    while ((event = (struct am_event **)am_queue_pop_front(queue)) != NULL) {
        me->crit_exit();
        cnt++;
        const struct am_event *e = *event;
        AM_ASSERT(e);
        am_event_free(e);
        me->crit_enter();
    }

    me->crit_exit();

    return cnt;
}
