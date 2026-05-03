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
 * Asynchronous event API implementation.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "bit/bit.h"
#include "event_common.h"
#include "event_async.h"
#include "event_queue.h"

/** Asynchronous event state. */
struct am_event_async_state {
    /** User defined pubsub list. */
    struct am_event_subscribe_list* sub;
    /** User defined pubsub list length. */
    int nsub;

    /** Asynchronous event handlers */
    struct am_event_async_handler {
        /** Event handler function */
        am_event_async_fn fn;
        /** Event handler context */
        void* ctx;
    } handlers[AM_EVT_HANDLERS_NUM_MAX]; /**< event handlers */

    struct am_event_alloc* alloc; /**< event allocator */
};

static struct am_event_async_state m_async_state;

void am_event_async_init(
    struct am_event_subscribe_list* sub, int nsub, struct am_event_alloc* alloc
) {
    if (sub) {
        AM_ASSERT(nsub > 0);
        memset(sub, 0, sizeof(*sub) * (size_t)nsub);
    }

    struct am_event_async_state* me = &m_async_state;
    AM_ATOMIC_STORE_N(&me->sub, sub);
    me->nsub = nsub;

    me->alloc = alloc;
}

bool am_event_async_is_pubsub_enabled(void) {
    return AM_ATOMIC_LOAD_N(&m_async_state.sub) != NULL;
}

void am_event_async_subscribe(int handler_id, int event_id) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    struct am_event_async_state* me = &m_async_state;
    AM_ASSERT(me->handlers[handler_id].fn);
    AM_ASSERT(event_id >= AM_EVT_USER);
    AM_ASSERT(event_id < me->nsub);

    int i = handler_id / 8;

    am_event_crit_enter();
    me->sub[event_id].list[i] |= (uint8_t)(1U << (unsigned)(handler_id % 8));
    am_event_crit_exit();
}

void am_event_async_unsubscribe(int handler_id, int event_id) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    struct am_event_async_state* me = &m_async_state;
    AM_ASSERT(me->handlers[handler_id].fn);
    AM_ASSERT(event_id >= AM_EVT_USER);
    AM_ASSERT(event_id < me->nsub);

    int h = handler_id / 8;
    int i = event_id - AM_EVT_USER;

    am_event_crit_enter();
    me->sub[i].list[h] &= (uint8_t)~(1U << (unsigned)(handler_id % 8));
    am_event_crit_exit();
}

void am_event_async_unsubscribe_all(int handler_id) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    struct am_event_async_state* me = &m_async_state;
    AM_ASSERT(me->handlers[handler_id].fn);

    int h = handler_id / 8;
    unsigned clear_mask = ~(1U << (unsigned)(handler_id % 8));

    for (int i = 0; i < me->nsub; ++i) {
        am_event_crit_enter();
        me->sub[i].list[h] &= (uint8_t)clear_mask;
        am_event_crit_exit();
    }
}

void am_event_async_register_with_id(
    am_event_async_fn fn, void* ctx, int handler_id
) {
    AM_ASSERT(fn);
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);

    struct am_event_async_state* me = &m_async_state;

    am_event_crit_enter();

    AM_ASSERT(NULL == me->handlers[handler_id].fn);

    me->handlers[handler_id].fn = fn;
    me->handlers[handler_id].ctx = ctx;

    am_event_crit_exit();
}

void am_event_async_unregister(int handler_id) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    struct am_event_async_state* me = &m_async_state;

    am_event_crit_enter();
    AM_ASSERT(me->handlers[handler_id].fn);
    me->handlers[handler_id].fn = NULL;
    me->handlers[handler_id].ctx = NULL;
    am_event_crit_exit();
}

enum am_rc am_event_async_post(
    int dest_id,
    const struct am_event* event,
    struct am_event_queue_policy policy
) {
    AM_ASSERT(dest_id >= 0);
    AM_ASSERT(dest_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(event->id >= AM_EVT_USER);

    struct am_event_async_state* me = &m_async_state;

    am_event_crit_enter();

    struct am_event_async_handler* handler = &me->handlers[dest_id];
    AM_ASSERT(handler->fn);

    am_event_crit_exit();

    return handler->fn(handler->ctx, event, policy);
}

enum am_rc am_event_async_publish(
    const struct am_event* event, struct am_event_queue_policy policy
) {
    struct am_event_async_state* me = &m_async_state;

    AM_ASSERT(event->id < me->nsub);

    if (!am_event_is_static(event)) {
        /*
         * To avoid a potential race condition, if higher priority
         * event handler preempts the event publishing and frees the event
         * as processed.
         */
        am_event_inc_ref_cnt(event);
    }

    bool all_published = true;

    /*
     * The event publishing is done for higher priority
     * event handlers first to avoid priority inversion.
     */
    struct am_event_subscribe_list* sub = &me->sub[event->id];
    for (int i = AM_COUNTOF(sub->list) - 1; i >= 0; --i) {
        unsigned done = 0;
        int cnt = 0;
        while (true) {
            AM_ASSERT(cnt++ <= 8);

            am_event_crit_enter();

            unsigned list = sub->list[i];
            list &= ~done;
            if (0 == list) {
                am_event_crit_exit();
                break;
            }

            int msb = am_bit_u8_msb((uint8_t)list);
            done |= 1U << (unsigned)msb;

            const int ind = (8 * i) + msb;
            if (policy.exclude_id == ind) {
                am_event_crit_exit();
                continue;
            }

            struct am_event_async_handler* handler = &me->handlers[ind];
            AM_ASSERT(handler->fn);
            enum am_rc rc = handler->fn(handler->ctx, event, policy);

            am_event_crit_exit();

            if (AM_RC_ERR == rc) {
                all_published = false;
            }
        }
    }

    /*
     * Tries to free the event.
     * It is needed to balance the ref counter increment at the beginning of
     * the function. Also takes care of the case when no event handlers
     * subscribed to this event.
     */
    am_event_free(me->alloc, event);

    return all_published;
}
