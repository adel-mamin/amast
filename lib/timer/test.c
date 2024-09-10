/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
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
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "timer/timer.h"

#define EVT_TEST EVT_USER

static struct owner {
    int npost;
} m_owner;

static struct timer m_timer;

static void post_cb(void *owner, const struct event *event) {
    (void)event;
    AM_ASSERT(owner == &m_owner);
    AM_ASSERT(EVT_USER == event->id);
    m_owner.npost++;
}

static void test_post(void) {
    memset(&m_owner, 0, sizeof(m_owner));
    struct timer_cfg cfg = {.post = post_cb, .publish = NULL, .update = NULL};
    timer_ctor(&m_timer, &cfg);

    struct event_timer event;
    timer_event_ctor(&event, /*id=*/EVT_TEST, /*domain=*/0);
    timer_post_in_ticks(&m_timer, &event, &m_owner, 1);
    timer_tick(&m_timer, /*domain=*/0);
    AM_ASSERT(1 == m_owner.npost);
}

int main(void) {
    test_post();
    return 0;
}
