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
 * Event library API implementation.
 */

#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "onesize/onesize.h"
#include "event.h"

/** Event reference counter bit mask. */
#define AM_EVENT_REF_COUNTER_MASK ((1U << AM_EVENT_REF_COUNTER_BITS) - 1U)
/** Maximum value of reference counter. */
#define AM_EVENT_REF_COUNTER_MAX AM_EVENT_REF_COUNTER_MASK

/** Event ID least significant word bit mask. */
#define AM_EVENT_ID_LSW_MASK ((1U << AM_EVENT_ID_LSW_BITS) - 1U)

/** Pool index bit mask. */
#define AM_EVENT_POOL_INDEX_MASK ((1U << AM_EVENT_POOL_INDEX_BITS) - 1U)
/** Maximum pool index value */
#define AM_EVENT_POOL_INDEX_MAX ((int)AM_EVENT_POOL_INDEX_MASK)
/** Is AM_EVENT_POOLS_NUM_MAX too large? */
AM_ASSERT_STATIC(AM_EVENT_POOLS_NUM_MAX <= (AM_EVENT_POOL_INDEX_MAX + 1));

/** Event library internal state. */
struct am_event_state {
    /** user defined event memory pools  */
    struct am_onesize pools[AM_EVENT_POOLS_NUM_MAX];
    /** the number of user defined event memory pools */
    int npools;
    /** enter critical section */
    void (*crit_enter)(void);
    /** exit critical section */
    void (*crit_exit)(void);
};

static struct am_event_state am_event_state_;

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
    AM_ASSERT(me->npools < AM_EVENT_POOLS_NUM_MAX);
    if (me->npools > 0) {
        int prev_size = am_onesize_get_block_size(&me->pools[me->npools - 1]);
        AM_ASSERT(block_size > prev_size);
    }

    struct am_onesize_cfg cfg = {
        .pool = {.ptr = pool, .size = size},
        .block_size = block_size,
        .alignment = alignment
    };
    am_onesize_ctor(&me->pools[me->npools], &cfg);

    ++me->npools;
}

struct am_event *am_event_allocate_x(int id, int size, int margin) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(size > 0);
    AM_ASSERT(me->npools > 0);
    AM_ASSERT(me->npools <= AM_EVENT_POOL_INDEX_MAX);
    int maxind = me->npools - 1;
    AM_ASSERT(size <= am_onesize_get_block_size(&me->pools[maxind]));
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    /* find allocator using binary search */
    int left = 0;
    int right = maxind;
    while (left < right) {
        int mid = (left + right) / 2;
        int onesize = am_onesize_get_block_size(&me->pools[mid]);
        if (size > onesize) {
            left = mid + 1;
        } else if (size < onesize) {
            right = mid;
        } else {
            left = mid;
            break;
        }
    }
    me->crit_enter();
    struct am_event *event = am_onesize_allocate_x(&me->pools[left], margin);
    me->crit_exit();

    if (!event) { /* cppcheck-suppress knownConditionTrueFalse */
        return NULL;
    }

    memset(event, 0, sizeof(*event));
    event->id = id;
    event->id_lsw = (uint32_t)id & AM_EVENT_ID_LSW_MASK;
    event->pool_index_plus_one =
        (unsigned)(left + 1) & AM_EVENT_POOL_INDEX_MASK;

    return event;
}

struct am_event *am_event_allocate(int id, int size) {
    struct am_event *event = am_event_allocate_x(id, size, /*margin=*/0);
    AM_ASSERT(event);

    return event;
}

static void am_event_free_unsafe(const struct am_event *event) {
    if (am_event_is_static(event)) {
        return; /* the event is statically allocated */
    }

    struct am_event *e = AM_CAST(struct am_event *, event);
    AM_ASSERT(e->pool_index_plus_one <= AM_EVENT_POOLS_NUM_MAX);
    /*
     * Check if event is valid.
     * If the below assert hits, then the reason is likely
     * a double free condition.
     */
    AM_ASSERT(((uint32_t)e->id & AM_EVENT_ID_LSW_MASK) == e->id_lsw);

    if (e->ref_counter > 1) {
        --e->ref_counter;
        return;
    }

    am_onesize_free(&am_event_state_.pools[e->pool_index_plus_one - 1], e);
}

void am_event_free(const struct am_event *event) {
    AM_ASSERT(event);

    struct am_event_state *me = &am_event_state_;
    me->crit_enter();
    am_event_free_unsafe(event);
    me->crit_exit();
}

int am_event_get_pool_nfree_min(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npools);

    me->crit_enter();
    int nfree = am_onesize_get_nfree_min(&me->pools[index]);
    me->crit_exit();
    return nfree;
}

int am_event_get_pool_nfree(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npools);

    me->crit_enter();
    int nfree = am_onesize_get_nfree(&me->pools[index]);
    me->crit_exit();
    return nfree;
}

int am_event_get_pool_nblocks(int index) {
    struct am_event_state *me = &am_event_state_;
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < me->npools);

    return am_onesize_get_nblocks(&me->pools[index]);
}

int am_event_get_npools(void) { return am_event_state_.npools; }

struct am_event *am_event_dup_x(
    const struct am_event *event, int size, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(size >= (int)sizeof(struct am_event));
    const struct am_event_state *me = &am_event_state_;
    AM_ASSERT(me->npools > 0);
    AM_ASSERT(event->id >= AM_EVT_USER);
    if (!am_event_is_static(event)) {
        /*
         * Check if event is valid.
         * If the below assert hits, then the reason is likely
         * a use after free condition.
         */
        AM_ASSERT(
            ((uint32_t)event->id & AM_EVENT_ID_LSW_MASK) == event->id_lsw
        );
    }
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
    AM_ASSERT(AM_ALIGNOF_PTR(buf) >= AM_ALIGNOF(am_event_t));
    AM_ASSERT(size >= (int)sizeof(struct am_event));

    struct am_event_log_ctx *log = (struct am_event_log_ctx *)ctx;
    AM_ASSERT(log->cb);
    AM_ASSERT(log->pool_ind >= 0);

    AM_DISABLE_WARNING(AM_W_CAST_ALIGN);
    const struct am_event *event = (const struct am_event *)buf;
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN);
    log->cb(log->pool_ind, index, event, size);
}

void am_event_log_pools_unsafe(int num, am_event_log_fn cb) {
    AM_ASSERT(num != 0);
    AM_ASSERT(cb);

    struct am_event_state *me = &am_event_state_;
    struct am_event_log_ctx ctx = {.cb = cb};
    for (int i = 0; i < me->npools; ++i) {
        ctx.pool_ind = i;
        am_onesize_iterate_over_allocated_unsafe(
            &me->pools[i], num, am_event_log_cb, &ctx
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

    /*
     * Check if event is valid.
     * If the below assert hits, then the reason is likely
     * a use after free condition.
     */
    AM_ASSERT(((uint32_t)event->id & AM_EVENT_ID_LSW_MASK) == event->id_lsw);

    struct am_event *e = AM_CAST(struct am_event *, event);
    struct am_event_state *me = &am_event_state_;
    me->crit_enter();
    AM_ASSERT(event->ref_counter < AM_EVENT_REF_COUNTER_MAX);
    ++e->ref_counter;
    me->crit_exit();
}

void am_event_dec_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);
    am_event_free(event);
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

static enum am_rc am_event_push_x(
    struct am_queue *queue,
    const struct am_event *event,
    int margin,
    bool safe,
    am_push_fn push
) {
    AM_ASSERT(queue);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);
    const int capacity = am_queue_get_capacity(queue);
    AM_ASSERT(margin < capacity);
    AM_ASSERT(push);
    if (!am_event_is_static(event)) {
        /*
         * Check if event is valid.
         * If the below assert hits, then the reason is likely
         * a use after free condition.
         */
        AM_ASSERT(
            ((uint32_t)event->id & AM_EVENT_ID_LSW_MASK) == event->id_lsw
        );
    }

    struct am_event *e = AM_CAST(struct am_event *, event);
    struct am_event_state *me = &am_event_state_;

    if (safe) {
        me->crit_enter();
    }

    int nfree = am_queue_get_nfree(queue);
    if (nfree <= margin) {
        if (safe) {
            me->crit_exit();
            am_event_free(event);
        } else {
            am_event_free_unsafe(event);
        }
        return AM_RC_ERR;
    }
    AM_ASSERT(nfree > 0);

    if (!am_event_is_static(e)) {
        AM_ASSERT(e->ref_counter < AM_EVENT_REF_COUNTER_MAX);
        ++e->ref_counter;
    }

    bool rc = push(queue, &event, sizeof(struct am_event *));

    if (safe) {
        me->crit_exit();
    }

    AM_ASSERT(true == rc);

    if (capacity == nfree) {
        return AM_RC_QUEUE_WAS_EMPTY;
    }
    return AM_RC_OK;
}

enum am_rc am_event_push_back_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    return am_event_push_x(
        queue, event, margin, /*safe=*/true, am_queue_push_back
    );
}

enum am_rc am_event_push_back(
    struct am_queue *queue, const struct am_event *event
) {
    enum am_rc rc = am_event_push_back_x(queue, event, /*margin=*/0);
    AM_ASSERT(AM_RC_ERR != rc);
    return rc;
}

enum am_rc am_event_push_back_unsafe(
    struct am_queue *queue, const struct am_event *event
) {
    enum am_rc rc = am_event_push_x(
        queue, event, /*margin=*/0, /*safe=*/false, am_queue_push_back
    );
    AM_ASSERT(AM_RC_ERR != rc);
    return rc;
}

enum am_rc am_event_push_front_x(
    struct am_queue *queue, const struct am_event *event, int margin
) {
    return am_event_push_x(
        queue, event, margin, /*safe=*/true, am_queue_push_front
    );
}

enum am_rc am_event_push_front(
    struct am_queue *queue, const struct am_event *event
) {
    enum am_rc rc = am_event_push_front_x(queue, event, /*margin=*/0);
    AM_ASSERT(AM_RC_ERR != rc);
    return rc;
}

bool am_event_pop_front(
    struct am_queue *queue, am_event_handle_fn cb, void *ctx
) {
    AM_ASSERT(queue);

    struct am_event_state *me = &am_event_state_;

    const struct am_event *event = NULL;

    me->crit_enter();
    {
        struct am_event **e = am_queue_pop_front(queue);
        if (e) {
            event = *e;
        }
    }
    me->crit_exit();

    if (!event) {
        return false;
    }

    AM_ASSERT(event);

    const int id = event->id;

    if (cb) {
        cb(ctx, event);
    }

    /*
     * Event was freed / corrupted ?
     *
     * One possible reason could be the following usage scenario:
     *
     *  const struct am_event *e = am_event_allocate(id, size);
     *  am_event_inc_ref_cnt(e); <-- THIS IS MISSING
     *  am_hsm_dispatch(hsm, e);
     *      am_event_push_XXX(queue, e) & am_event_pop_front(queue, ...)
     *      OR
     *      am_event_inc_ref_cnt(e) & am_event_dec_ref_cnt(e)
     *  am_event_free(&e);
     */
    AM_ASSERT(id == event->id); /* cppcheck-suppress knownArgument */

    if (am_event_is_static(event)) {
        return true;
    }

    am_event_free(event);

    return true;
}

int am_event_flush_queue(struct am_queue *queue) {
    int cnt = 0;
    struct am_event_state *me = &am_event_state_;

    int capacity = am_queue_get_capacity(queue);

    me->crit_enter();

    struct am_event **e = NULL;
    while ((e = am_queue_pop_front(queue)) != NULL) {
        me->crit_exit();
        ++cnt;
        AM_ASSERT(cnt <= capacity);
        AM_ASSERT(*e);
        am_event_free(*e);
        me->crit_enter();
    }

    me->crit_exit();

    return cnt;
}
