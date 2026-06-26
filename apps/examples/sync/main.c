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

#include <stdio.h>
#include <inttypes.h>

#include "common/macros.h"
#include "event/event_sync.h"
#include "event/event_common.h"
#include "timer/timer.h"
#include "pal/pal.h"
#include "events.h"
#include "low.h"
#include "top.h"

static void timer_proc(struct am_timer* timer, struct am_event_sync_hub* hub) {
    struct am_timer_event* fired = NULL;

    am_timer_tick_iterator_init(timer);

    while ((fired = am_timer_tick_iterator_next(timer)) != NULL) {
        const int* dest_id = AM_CAST(struct am_timer_event_x*, fired)->ctx;
        AM_ASSERT(dest_id);
        am_event_sync_post(hub, *dest_id, &fired->event);
    }
}

static const char* event_to_str(int event_id) {
    switch (event_id) {
    case EVT_JOB_REQ:
        return "JOB_REQ";
    case EVT_JOB_DONE:
        return "JOB_DONE";
    case EVT_COMMIT:
        return "COMMIT";
    case EVT_TIMEOUT:
        return "TIMEOUT";
    default:
        break;
    }
    return "UNKN";
}

static void event_sync_observe(int handler_id, const struct am_event* event) {
    am_printf(
        "%" PRIu32 " handler_id: %d event: %s\n",
        am_time_get_ms(),
        handler_id,
        event_to_str(event->id)
    );
}

int main(void) {
    am_pal_global_init(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_init(&timer);

    struct am_event_sync_hub hub;
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX - AM_EVT_USER];

    am_event_sync_init(&hub, &pubsub_list[0], AM_COUNTOF(pubsub_list));
    am_event_sync_observe(&hub, event_sync_observe);

    struct top top;
    struct low low;

    top_init(&top, &hub, /*rounds=*/2);
    low_init(&low, &hub, &timer);

    uint32_t now_ticks = am_time_get_ticks(AM_TIMEBASE_DEFAULT);
    while (top_is_active(&top)) {
        const struct am_event commit = {.id = EVT_COMMIT};
        top_event_post(&top, &commit);
        low_event_post(&low, &commit);

        am_sleep_till_ticks(AM_TIMEBASE_DEFAULT, now_ticks + 1);
        now_ticks += 1;

        timer_proc(&timer, &hub);
    }

    am_pal_global_deinit();

    return 0;
}
