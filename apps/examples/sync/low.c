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

#include <string.h>

#include "event/event_sync.h"
#include "timer/timer.h"
#include "pal/pal.h"

#include "low.h"
#include "events.h"
#include "common/macros.h"

static bool low_event_handler(
    struct low* low,
    const struct am_event* event,
    struct am_event* out,
    int out_size
) {
    (void)out;
    (void)out_size;

    switch (event->id) {
    case EVT_JOB_REQ:
        AM_ASSERT(!am_timer_is_armed(low->timer, &low->timer_event.event));

        am_timer_arm(
            low->timer, &low->timer_event.event, low->timeout, /*interval=*/0
        );
        break;

    case EVT_TIMEOUT:
        am_event_sync_publish(low->hub, &(struct am_event){.id = EVT_JOB_DONE});
        break;

    default:
        break;
    }
    return true;
}

bool low_event_post(struct low* low, const struct am_event* event) {
    return low_event_handler(
        low,
        event,
        /*out=*/NULL,
        /*out_size=*/0
    );
}

void low_init(
    struct low* low, struct am_event_sync_hub* hub, struct am_timer* timer
) {
    memset(low, 0, sizeof(*low));

    low->hub = hub;
    low->timer = timer;
    low->handler_id =
        am_event_sync_register(hub, (am_event_sync_fn)low_event_handler, low);
    low->timer_event = am_timer_event_create_x(EVT_TIMEOUT, &low->handler_id);
    low->timeout = am_time_get_ticks_from_ms(AM_TIMEBASE_DEFAULT, 1000);

    am_event_sync_subscribe(hub, low->handler_id, EVT_JOB_REQ);
}
