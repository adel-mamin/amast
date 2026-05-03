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
#include "common/types.h"
#include "event/event_common.h"
#include "event/event_sync.h"

#include "common/macros.h"
#include "bit/bit.h"

void am_event_sync_init(
    struct am_event_sync_hub* hub,
    struct am_event_subscribe_list* sub,
    int nsub,
    struct am_event_alloc* alloc
) {
    AM_ASSERT(hub);

    memset(hub, 0, sizeof(*hub));

    if (sub) {
        AM_ASSERT(nsub > 0);
        memset(sub, 0, sizeof(*sub) * (size_t)nsub);
    }

    hub->sub = sub;
    hub->nsub = nsub;
    hub->alloc = alloc;
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

enum am_rc am_event_sync_post(
    struct am_event_sync_hub* hub,
    int dest_id,
    const struct am_event* event,
    struct am_event* out,
    int out_size
) {
    AM_ASSERT(hub);

    AM_ASSERT(dest_id >= 0);
    AM_ASSERT(dest_id < AM_EVT_HANDLERS_NUM_MAX);
    am_event_sync_fn fn = hub->handlers[dest_id].fn;
    AM_ASSERT(fn);
    AM_ASSERT(event->id >= AM_EVT_USER);

    void* ctx = hub->handlers[dest_id].ctx;

    return fn(ctx, event, out, out_size);
}

enum am_rc am_event_sync_publish(
    struct am_event_sync_hub* hub,
    int publisher_id,
    const struct am_event* event,
    struct am_event* out,
    int out_size,
    unsigned policy
) {
    AM_ASSERT(hub);
    AM_ASSERT(hub->sub);

    AM_ASSERT(publisher_id >= 0);
    AM_ASSERT(publisher_id < AM_EVT_HANDLERS_NUM_MAX);
    AM_ASSERT(event->id >= AM_EVT_USER);
    AM_ASSERT(event->id < hub->nsub);

    if (!am_event_is_static(event)) {
        /* to avoid freeing the event during the processing */
        am_event_inc_ref_cnt(event);
    }

    bool all_published = true;

    struct am_event_subscribe_list* sub = &hub->sub[event->id];
    for (int i = 0; i < AM_COUNTOF(sub->list); ++i) {
        unsigned done = 0;
        int cnt = 0;
        while (true) {
            AM_ASSERT(cnt++ <= 8);

            unsigned list = sub->list[i];
            list &= ~done;
            if (0 == list) {
                break;
            }

            int msb = am_bit_u8_msb((uint8_t)list);
            done |= 1U << (unsigned)msb;

            int ind = (8 * i) + msb;
            if (0 == (policy & AM_EVENT_SYNC_RECURSION)) {
                if (ind == publisher_id) {
                    continue;
                }
            }

            struct am_event_sync_handler* handler = &hub->handlers[ind];
            AM_ASSERT(handler->fn);
            enum am_rc rc = handler->fn(handler->ctx, event, out, out_size);

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
    am_event_free(hub->alloc, event);

    return all_published;
}
