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
 * @file
 * Timer unit tests.
 */

#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"

#define EVT_TEST AM_EVT_USER
#define EVT_TEST2 (AM_EVT_USER + 1)

static void test_arm(void) {
    struct am_timer timer;

    struct am_timer_event timer_events[2];

    am_timer_ctor(
        &timer,
        timer_events,
        AM_COUNTOF(timer_events),
        sizeof(struct am_timer_event)
    );

    int tix = am_timer_allocate(&timer, EVT_TEST);
    am_timer_arm(&timer, tix, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, tix));

    int tix2 = am_timer_allocate(&timer, EVT_TEST2);
    am_timer_arm(&timer, tix2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, tix2));

    am_timer_disarm(&timer, tix2);
    AM_ASSERT(!am_timer_is_armed(&timer, tix2));
    AM_ASSERT(!am_timer_is_empty_unsafe(&timer));

    AM_ASSERT(am_timer_tick(&timer) & (1UL << (unsigned)tix));

    AM_ASSERT(am_timer_is_empty_unsafe(&timer));

    AM_ASSERT(0 == am_timer_tick(&timer));

    am_timer_arm(&timer, tix, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, tix));

    AM_ASSERT(am_timer_tick(&timer) & (1UL << (unsigned)tix));

    am_timer_arm(&timer, tix2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, tix2));

    AM_ASSERT(am_timer_tick(&timer) & (1UL << (unsigned)tix2));

    AM_ASSERT(am_timer_is_empty_unsafe(&timer));
}

int main(void) {
    test_arm();
    return 0;
}
