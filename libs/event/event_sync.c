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
 * Synchronous event API implementation.
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "event_common.h"
#include "event_sync.h"

#include "common/macros.h"
#include "bit/bit.h"

/** Maximum recursion count allowed. */
#ifndef AM_SYNC_RECURSION_MAX
#define AM_SYNC_RECURSION_MAX 16
#endif

static void am_event_sync_observer_nil(
    int handler_id, const struct am_event* event
) {
    (void)handler_id;
    (void)event;
}

void am_event_sync_init(
    struct am_event_sync_hub* hub, struct am_event_subscribe_list* sub, int nsub
) {
    AM_ASSERT(hub);

    memset(hub, 0, sizeof(*hub));

    if (sub) {
        AM_ASSERT(nsub > 0);
        memset(sub, 0, sizeof(*sub) * (size_t)nsub);
    }

    hub->sub = sub;
    hub->nsub = nsub;
    hub->observer_cb = am_event_sync_observer_nil;
}

bool am_event_sync_is_pubsub_enabled(const struct am_event_sync_hub* hub) {
    AM_ASSERT(hub);
    return (hub->sub != NULL);
}

void am_event_sync_subscribe(
    struct am_event_sync_hub* hub, int handler_id, int event_id
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);

    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);
    AM_ASSERT(event_id >= AM_EVT_USER);

    int si = event_id - AM_EVT_USER;
    AM_ASSERT(si < hub->nsub);

    int li = handler_id / 8;
    hub->sub[si].list[li] |= (uint8_t)(1U << (unsigned)(handler_id % 8));
}

void am_event_sync_unsubscribe(
    struct am_event_sync_hub* hub, int handler_id, int event_id
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);

    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);
    AM_ASSERT(event_id >= AM_EVT_USER);

    int si = event_id - AM_EVT_USER;
    AM_ASSERT(si < hub->nsub);

    int li = handler_id / 8;
    hub->sub[si].list[li] &= (uint8_t)~(1U << (unsigned)(handler_id % 8));
}

void am_event_sync_unsubscribe_all(
    struct am_event_sync_hub* hub, int handler_id
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);

    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);

    int li = handler_id / 8;
    unsigned clear_mask = ~(1U << (unsigned)(handler_id % 8));

    for (int i = 0; i < hub->nsub; ++i) {
        hub->sub[i].list[li] &= (uint8_t)clear_mask;
    }
}

int am_event_sync_register(
    struct am_event_sync_hub* hub, am_event_sync_fn fn, void* ctx
) {
    AM_ASSERT(hub);
    AM_ASSERT(fn);

    for (int i = 0; i < AM_COUNTOF(hub->handlers); ++i) {
        if (NULL == hub->handlers[i].fn) {
            hub->handlers[i].fn = fn;
            hub->handlers[i].ctx = ctx;
            return i;
        }
    }
    AM_ASSERT(0);

    return -1;
}

void am_event_sync_unregister(struct am_event_sync_hub* hub, int handler_id) {
    AM_ASSERT(hub);
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);

    if (hub->sub) {
        am_event_sync_unsubscribe_all(hub, handler_id);
    }

    hub->handlers[handler_id].fn = NULL;
    hub->handlers[handler_id].ctx = NULL;
}

void am_event_sync_observe(
    struct am_event_sync_hub* hub, am_event_sync_observer_fn fn
) {
    AM_ASSERT(hub);
    hub->observer_cb = fn ? fn : am_event_sync_observer_nil;
}

bool am_event_sync_post_request(
    struct am_event_sync_hub* hub,
    int dest_id,
    const struct am_event* event,
    void* out,
    int out_size
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->recursion_count < AM_SYNC_RECURSION_MAX);

    AM_ASSERT(dest_id >= 0);
    AM_ASSERT(dest_id < AM_EVT_HANDLERS_NUM_MAX);
    am_event_sync_fn fn = hub->handlers[dest_id].fn;
    AM_ASSERT(fn);
    AM_ASSERT(event);

    void* ctx = hub->handlers[dest_id].ctx;

    ++hub->recursion_count;

    hub->observer_cb(dest_id, event);

    bool ret = fn(ctx, event, out, out_size);

    --hub->recursion_count;

    return ret;
}

bool am_event_sync_post(
    struct am_event_sync_hub* hub, int dest_id, const struct am_event* event
) {
    return am_event_sync_post_request(
        hub,
        dest_id,
        event,
        /*out=*/NULL,
        /*out_size=*/0
    );
}

bool am_event_sync_publish_request(
    struct am_event_sync_hub* hub,
    const struct am_event* event,
    void* out,
    int out_size
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);
    AM_ASSERT(hub->recursion_count < AM_SYNC_RECURSION_MAX);

    AM_ASSERT(event);
    AM_ASSERT(event->id >= AM_EVT_USER);

    ++hub->recursion_count;

    bool all_published = true;

    int si = event->id - AM_EVT_USER;
    AM_ASSERT(si < hub->nsub);

    struct am_event_subscribe_list sub = hub->sub[si];
    for (int i = 0; i < AM_COUNTOF(sub.list); ++i) {
        while (sub.list[i]) {
            int msb = am_bit_u8_msb(sub.list[i]);
            sub.list[i] &= (uint8_t)~(1U << (unsigned)msb);

            int handler_id = (8 * i) + msb;

            hub->observer_cb(handler_id, event);

            struct am_event_sync_handler* handler = &hub->handlers[handler_id];

            AM_ASSERT(handler->fn);
            if (!handler->fn(handler->ctx, event, out, out_size)) {
                all_published = false;
            }
        }
    }

    --hub->recursion_count;

    return all_published;
}

bool am_event_sync_publish(
    struct am_event_sync_hub* hub, const struct am_event* event
) {
    return am_event_sync_publish_request(
        hub,
        event,
        /*out=*/NULL,
        /*out_size=*/0
    );
}
