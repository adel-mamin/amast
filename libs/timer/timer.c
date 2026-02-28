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

#include "common/compiler.h"
#include "common/macros.h"
#include "timer/timer.h"

#define AM_TIX_IS_VALID(x) ((0 <= (x)) && ((x) < timer->events_num))

static void timer_crit_stub(void) {}

void am_timer_ctor(
    struct am_timer *timer, void *events, int events_num, int event_size
) {
    AM_ASSERT(events);
    AM_ASSERT(AM_ALIGNOF_PTR(events) >= AM_ALIGNOF(am_timer_event_t));
    AM_ASSERT(events_num > 0);
    AM_ASSERT(
        AM_ALIGN_SIZE(event_size, AM_ALIGNOF(am_timer_event_t)) == event_size
    );

    memset(timer, 0, sizeof(*timer));
    timer->crit_enter = timer->crit_exit = timer_crit_stub;
    timer->events = events;
    timer->events_num = events_num;
    timer->event_size = event_size;
    if (timer->events_num < 32) {
        timer->timers_allocated = (uint32_t)-1 << (uint32_t)timer->events_num;
    }
}

void am_timer_register_cbs(
    struct am_timer *timer, void (*crit_enter)(void), void (*crit_exit)(void)
) {
    AM_ASSERT(crit_enter);
    AM_ASSERT(crit_exit);
    timer->crit_enter = crit_enter;
    timer->crit_exit = crit_exit;
}

int am_timer_allocate(struct am_timer *timer, int event_id) {
    AM_ASSERT(timer);
    AM_ASSERT(event_id >= AM_EVT_USER);
    AM_ASSERT(event_id <= UINT16_MAX);

    uint32_t free_slots = ~timer->timers_allocated;
    AM_ASSERT(free_slots); /* no free timer slots */
    int tix = AM_CTZL(free_slots);
    AM_ASSERT(tix < timer->events_num);
    struct am_timer_event *event = am_timer_from_tix(timer, tix);

    memset(event, 0, (size_t)timer->event_size);
    event->base.id = (uint16_t)event_id;

    timer->timers_allocated |= (uint32_t)(1UL << (unsigned)tix);

    return tix;
}

int am_timer_allocate_x(struct am_timer *timer, int event_id, void *ctx) {
    int tix = am_timer_allocate(timer, event_id);
    AM_ASSERT(timer->event_size >= (int)sizeof(struct am_timer_event_x));
    const struct am_timer_event *event = am_timer_from_tix(timer, tix);
    AM_CAST(struct am_timer_event_x *, event)->ctx = ctx;
    return tix;
}

void am_timer_arm(
    struct am_timer *timer, int tix, uint32_t ticks, uint32_t interval
) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_TIX_IS_VALID(tix));
    AM_ASSERT(ticks > 0);

    struct am_timer_event *event = am_timer_from_tix(timer, tix);

    timer->crit_enter();

    event->oneshot_ticks = AM_MAX(ticks, 1);
    event->interval_ticks = interval;

    timer->timers_running |= (uint32_t)(1UL << (unsigned)tix);

    timer->crit_exit();
}

bool am_timer_disarm(struct am_timer *timer, int tix) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_TIX_IS_VALID(tix));

    timer->crit_enter();

    struct am_timer_event *event = am_timer_from_tix(timer, tix);
    bool was_armed = event->oneshot_ticks || event->interval_ticks;
    event->oneshot_ticks = event->interval_ticks = 0;
    timer->timers_running &= ~(uint32_t)(1UL << (unsigned)tix);

    timer->crit_exit();

    return was_armed;
}

bool am_timer_is_armed(const struct am_timer *timer, int tix) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_TIX_IS_VALID(tix));

    timer->crit_enter();

    bool armed = (timer->timers_running & (1UL << tix)) != 0;

    timer->crit_exit();

    return armed;
}

uint32_t am_timer_tick(struct am_timer *timer) {
    AM_ASSERT(timer);

    uint32_t fired = 0;

    timer->crit_enter();

    uint32_t running = timer->timers_running;
    while (running) {
        int tix = AM_CTZL(running);
        struct am_timer_event *event = am_timer_from_tix(timer, tix);
        AM_ASSERT(event->oneshot_ticks);
        --event->oneshot_ticks;
        uint32_t mask = (uint32_t)1 << (uint32_t)tix;
        running &= ~mask;
        if (event->oneshot_ticks) {
            continue;
        }
        fired |= mask;
        if (event->interval_ticks) {
            event->oneshot_ticks = event->interval_ticks;
            continue;
        }
        timer->timers_running &= ~mask;
    }

    timer->crit_exit();

    return fired;
}

struct am_timer_event *am_timer_from_tix(
    const struct am_timer *timer, int tix
) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_TIX_IS_VALID(tix));

    const char *event = (char *)timer->events + timer->event_size * tix;
    return AM_CAST(struct am_timer_event *, event);
}

bool am_timer_is_empty_unsafe(const struct am_timer *timer) {
    AM_ASSERT(timer);
    return !timer->timers_running;
}

uint32_t am_timer_get_ticks(const struct am_timer *timer, int tix) {
    AM_ASSERT(timer);

    timer->crit_enter();
    uint32_t ticks = am_timer_from_tix(timer, tix)->oneshot_ticks;
    timer->crit_exit();

    return ticks;
}
