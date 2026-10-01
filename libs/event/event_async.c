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

void am_event_async_init(
    struct am_event_async_hub* hub, const struct am_event_async_cfg* cfg
) {
    if (cfg->subscription_count || cfg->subscriptions) {
        AM_ASSERT(cfg->subscriptions != 0);
        AM_ASSERT(cfg->subscription_count > 0);
        memset(
            cfg->subscriptions,
            0,
            sizeof(*cfg->subscriptions) * (size_t)cfg->subscription_count
        );
    }

    memset(hub, 0, sizeof(*hub));

    hub->subscriptions = cfg->subscriptions;
    hub->subscription_count = cfg->subscription_count;

    hub->alloc = cfg->alloc;
}

bool am_event_async_is_pubsub_enabled(struct am_event_async_hub* hub) {
    return AM_ATOMIC_LOAD_N(&hub->subscriptions) != NULL;
}

void am_event_async_subscribe(
    struct am_event_async_hub* hub, int handler_id, uint16_t event_id
) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(event_id >= AM_EVT_USER);
    AM_ASSERT(hub->subscriptions != NULL);

    int si = event_id - AM_EVT_USER;
    AM_ASSERT(si < hub->subscription_count);

    int li = handler_id / 8;

    am_event_crit_enter();

    AM_ASSERT(hub->handlers[handler_id].fn);
    hub->subscriptions[si].list[li] |=
        (uint8_t)(1U << (unsigned)(handler_id % 8));

    am_event_crit_exit();
}

void am_event_async_unsubscribe(
    struct am_event_async_hub* hub, int handler_id, uint16_t event_id
) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(event_id >= AM_EVT_USER);
    AM_ASSERT(hub->subscriptions != NULL);

    int si = event_id - AM_EVT_USER;
    AM_ASSERT(si < hub->subscription_count);

    int li = handler_id / 8;

    am_event_crit_enter();

    AM_ASSERT(hub->handlers[handler_id].fn);

    hub->subscriptions[si].list[li] &=
        (uint8_t)~(1U << (unsigned)(handler_id % 8));

    am_event_crit_exit();
}

void am_event_async_unsubscribe_all(
    struct am_event_async_hub* hub, int handler_id
) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);

    int li = handler_id / 8;
    unsigned clear_mask = ~(1U << (unsigned)(handler_id % 8));

    am_event_crit_enter();

    AM_ASSERT(hub->handlers[handler_id].fn);

    for (int i = 0; i < hub->subscription_count; ++i) {
        hub->subscriptions[i].list[li] &= (uint8_t)clear_mask;
    }

    am_event_crit_exit();
}

void am_event_async_register_with_id(
    struct am_event_async_hub* hub,
    am_event_async_enqueue_fn fn,
    void* ctx,
    int handler_id
) {
    AM_ASSERT(fn);
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);

    am_event_crit_enter();

    AM_ASSERT(NULL == hub->handlers[handler_id].fn);

    hub->handlers[handler_id].fn = fn;
    hub->handlers[handler_id].ctx = ctx;
    hub->handler_generations[handler_id]++;

    am_event_crit_exit();
}

void am_event_async_unregister(struct am_event_async_hub* hub, int handler_id) {
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);

    am_event_crit_enter();

    int h = handler_id / 8;
    unsigned clear_mask = ~(1U << (unsigned)(handler_id % 8));

    for (int i = 0; i < hub->subscription_count; ++i) {
        hub->subscriptions[i].list[h] &= (uint8_t)clear_mask;
    }

    AM_ASSERT(hub->handlers[handler_id].fn);
    hub->handlers[handler_id].fn = NULL;
    hub->handlers[handler_id].ctx = NULL;

    am_event_crit_exit();
}

bool am_event_async_post(
    struct am_event_async_hub* hub,
    int dest_id,
    const struct am_event* event,
    struct am_event_queue_policy policy
) {
    AM_ASSERT(dest_id >= 0);
    AM_ASSERT(dest_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(event);
    AM_ASSERT(event->id >= AM_EVT_USER);

    am_event_crit_enter();

    struct am_event_async_handler* handler = &hub->handlers[dest_id];
    AM_ASSERT(handler->fn);
    bool ok = handler->fn(handler->ctx, event, policy);

    if (!ok && (event->ref_counter == 0)) {
        am_event_free_unsafe(hub->alloc, event);
    }

    am_event_crit_exit();

    return ok;
}

bool am_event_async_publish(
    struct am_event_async_hub* hub,
    const struct am_event* event,
    struct am_event_queue_policy policy
) {
    AM_ASSERT(hub->subscriptions != NULL);
    AM_ASSERT(event);
    AM_ASSERT(event->id >= AM_EVT_USER);

    int si = event->id - AM_EVT_USER;

    AM_ASSERT(si < hub->subscription_count);

    if (!am_event_is_static(event)) {
        AM_ASSERT(hub->alloc);
        /*
         * To avoid a potential race condition, if higher priority
         * event handler (the event handler with larger ID value)
         * preempts the event publishing and frees the event
         * as processed.
         */
        am_event_inc_ref_cnt(event);
    }

    bool all_published = true;

    /*
     * The event publishing is done for event handlers
     * with larger ID value first as they are treated as
     * higher priority event handlers.
     */
    am_event_crit_enter();

    struct am_event_subscription sub = hub->subscriptions[si];
    uint8_t generations[AM_EVT_HANDLERS_NUM_MAX];
    memcpy(generations, hub->handler_generations, sizeof(generations));

    am_event_crit_exit();

    for (int i = AM_COUNTOF(sub.list) - 1; i >= 0; --i) {
        while (sub.list[i]) {
            int msb = am_bit_u8_msb(sub.list[i]);
            sub.list[i] &= (uint8_t)~(1U << (unsigned)msb);

            const int ind = (8 * i) + msb;
            if (policy.exclude && (policy.exclude_id == ind)) {
                continue;
            }

            am_event_crit_enter();

            struct am_event_async_handler* handler = &hub->handlers[ind];

            /*
             * Make sure the handler has not changed while publishing the event.
             */
            if (handler->fn &&
                (hub->handler_generations[ind] == generations[ind])) {
                bool ok = handler->fn(handler->ctx, event, policy);
                if (!ok) {
                    all_published = false;
                }
            }

            am_event_crit_exit();
        }
    }

    /*
     * Tries to free the event.
     * It is needed to balance the ref counter increment at the beginning of
     * the function. Also takes care of the case when no event handlers
     * subscribed to this event.
     */
    am_event_free(hub->alloc, event);

    return all_published;
}
