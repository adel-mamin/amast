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
#include "event/event_common.h"
#include "event/event_sync.h"

#include "common/macros.h"
#include "bit/bit.h"

/** Maximum recursion count allowed. */
#ifndef AM_SYNC_RECURSION_MAX
#define AM_SYNC_RECURSION_MAX 16
#endif

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
    AM_ASSERT(event_id < hub->nsub);

    int i = handler_id / 8;
    hub->sub[event_id].list[i] |= (uint8_t)(1U << (unsigned)(handler_id % 8));
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
    AM_ASSERT(event_id < hub->nsub);

    int h = handler_id / 8;
    int i = event_id - AM_EVT_USER;
    hub->sub[i].list[h] &= (uint8_t)~(1U << (unsigned)(handler_id % 8));
}

void am_event_sync_unsubscribe_all(
    struct am_event_sync_hub* hub, int handler_id
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);

    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);

    int h = handler_id / 8;
    unsigned clear_mask = ~(1U << (unsigned)(handler_id % 8));

    for (int i = 0; i < hub->nsub; ++i) {
        hub->sub[i].list[h] &= (uint8_t)clear_mask;
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
}

void am_event_sync_unregister(struct am_event_sync_hub* hub, int handler_id) {
    AM_ASSERT(hub);
    AM_ASSERT(handler_id >= 0);
    AM_ASSERT(handler_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(hub->handlers[handler_id].fn);

    hub->handlers[handler_id].fn = NULL;
    hub->handlers[handler_id].ctx = NULL;
}

bool am_event_sync_post_request(
    struct am_event_sync_hub* hub,
    int dest_id,
    const struct am_event* event,
    struct am_event* out,
    int out_size
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->recursion_count < AM_SYNC_RECURSION_MAX);

    AM_ASSERT(dest_id >= 0);
    AM_ASSERT(dest_id < AM_EVT_HANDLERS_NUM_MAX);
    am_event_sync_fn fn = hub->handlers[dest_id].fn;
    AM_ASSERT(fn);
    AM_ASSERT(event->id >= AM_EVT_USER);

    void* ctx = hub->handlers[dest_id].ctx;

    ++hub->recursion_count;

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
    struct am_event* out,
    int out_size
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);
    AM_ASSERT(hub->recursion_count < AM_SYNC_RECURSION_MAX);

    AM_ASSERT(event->id >= AM_EVT_USER);
    AM_ASSERT(event->id < hub->nsub);

    ++hub->recursion_count;

    bool all_published = true;

    struct am_event_subscribe_list* sub = &hub->sub[event->id];
    for (int i = 0; i < AM_COUNTOF(sub->list); ++i) {
        unsigned list = sub->list[i];
        while (list) {
            int msb = am_bit_u8_msb((uint8_t)list);
            list &= ~(1U << (unsigned)msb);

            int ind = (8 * i) + msb;
            struct am_event_sync_handler* handler = &hub->handlers[ind];
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
