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
 * Timer API implementation.
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "common/macros.h"
#include "slist/slist.h"
#include "timer/timer.h"

static void timer_crit_stub(void) {}

void am_timer_ctor(struct am_timer *timer) {
    memset(timer, 0, sizeof(*timer));
    timer->crit_enter = timer->crit_exit = timer_crit_stub;

    am_slist_ctor(&timer->events);
    am_slist_ctor(&timer->events_pend);
}

void am_timer_register_cbs(
    struct am_timer *timer, void (*crit_enter)(void), void (*crit_exit)(void)
) {
    AM_ASSERT(crit_enter);
    AM_ASSERT(crit_exit);
    timer->crit_enter = crit_enter;
    timer->crit_exit = crit_exit;
}

void am_timer_arm(
    struct am_timer *timer,
    struct am_timer_event *event,
    uint32_t ticks,
    uint32_t interval
) {
    AM_ASSERT(timer);
    AM_ASSERT(event);
    AM_ASSERT(ticks > 0);
    AM_ASSERT(interval < UINT32_MAX / 2);

    timer->crit_enter();

    event->oneshot_ticks = AM_MAX(ticks, 1);
    event->interval_ticks = interval & 0x7FFFFFFF;
    event->disarm_pending = 0;

    if (!am_slist_item_is_linked(&event->item)) {
        am_slist_push_back(&timer->events_pend, &event->item);
        ++timer->nevents.pend;
    }

    timer->crit_exit();
}

bool am_timer_disarm(struct am_timer *timer, struct am_timer_event *event) {
    AM_ASSERT(timer);
    AM_ASSERT(event);

    timer->crit_enter();

    bool was_armed = am_slist_item_is_linked(&event->item);
    event->oneshot_ticks = event->interval_ticks = 0;
    event->disarm_pending = 1;

    timer->crit_exit();

    return was_armed;
}

bool am_timer_is_armed(
    const struct am_timer *timer, const struct am_timer_event *event
) {
    AM_ASSERT(timer);
    AM_ASSERT(event);

    timer->crit_enter();

    bool armed = am_slist_item_is_linked(&event->item);
    bool disarm_pending = event->disarm_pending;

    timer->crit_exit();

    return armed && !disarm_pending;
}

void am_timer_tick_iterator_init(struct am_timer *timer) {
    am_slist_iterator_ctor(&timer->events, &timer->it);

    timer->crit_enter();

    if (!am_slist_is_empty(&timer->events_pend)) {
        am_slist_append(&timer->events, &timer->events_pend);
        timer->nevents.running += timer->nevents.pend;
        timer->nevents.pend = 0;
    }

    timer->crit_exit();
}

struct am_timer_event *am_timer_tick_iterator_next(struct am_timer *timer) {
    AM_ASSERT(timer);

    timer->crit_enter();

    int nevents = timer->nevents.running;

    struct am_slist_item *p = NULL;
    struct am_timer_event *event = NULL;
    while ((p = am_slist_iterator_next(&timer->it)) != NULL) {
        event = AM_CONTAINER_OF(p, struct am_timer_event, item);

        AM_ASSERT(nevents > 0);
        --nevents;

        if (event->disarm_pending) {
            am_slist_iterator_pop(&timer->it);
            event->disarm_pending = 0;
            AM_ASSERT(timer->nevents.running > 0);
            --timer->nevents.running;
            event = NULL;
            continue;
        }

        AM_ASSERT(event->oneshot_ticks);
        --event->oneshot_ticks;
        if (event->oneshot_ticks) {
            event = NULL;
            continue;
        }
        if (event->interval_ticks) {
            event->oneshot_ticks = event->interval_ticks;
        } else {
            am_slist_iterator_pop(&timer->it);
            AM_ASSERT(timer->nevents.running > 0);
            --timer->nevents.running;
        }
        break;
    }

    timer->crit_exit();

    return event;
}

bool am_timer_is_empty_unsafe(const struct am_timer *timer) {
    AM_ASSERT(timer);

    bool empty = am_slist_is_empty(&timer->events);
    bool empty_pend = am_slist_is_empty(&timer->events_pend);

    return empty && empty_pend;
}

uint32_t am_timer_get_ticks(
    const struct am_timer *timer, const struct am_timer_event *event
) {
    AM_ASSERT(timer);
    AM_ASSERT(event);

    timer->crit_enter();
    uint32_t ticks = event->oneshot_ticks;
    timer->crit_exit();

    return ticks;
}
