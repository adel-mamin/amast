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

#include <stddef.h>

#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"

#define EVT_TEST AM_EVT_USER
#define EVT_TEST2 (AM_EVT_USER + 1)

static void test_arm(void) {
    struct am_timer timer;
    struct am_timer_event test = am_timer_event_ctor(EVT_TEST);
    struct am_timer_event test2 = am_timer_event_ctor(EVT_TEST2);

    am_timer_ctor(&timer);

    am_timer_arm(&timer, &test, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, &test));

    am_timer_arm(&timer, &test2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, &test2));

    am_timer_disarm(&timer, &test2);
    AM_ASSERT(!am_timer_is_armed(&timer, &test2));
    AM_ASSERT(!am_timer_is_empty_unsafe(&timer));

    am_timer_tick_iterator_init(&timer);
    struct am_timer_event* e = am_timer_tick_iterator_next(&timer);
    AM_ASSERT(e && (test.event.id == e->event.id));

    AM_ASSERT(NULL == am_timer_tick_iterator_next(&timer));

    AM_ASSERT(am_timer_is_empty_unsafe(&timer));

    am_timer_arm(&timer, &test, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, &test));

    am_timer_tick_iterator_init(&timer);
    e = am_timer_tick_iterator_next(&timer);
    AM_ASSERT(e && (test.event.id == e->event.id));

    am_timer_arm(&timer, &test2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&timer, &test2));

    am_timer_tick_iterator_init(&timer);
    e = am_timer_tick_iterator_next(&timer);
    AM_ASSERT(e && (test2.event.id == e->event.id));

    AM_ASSERT(am_timer_is_empty_unsafe(&timer));
}

int main(void) {
    test_arm();
    return 0;
}
