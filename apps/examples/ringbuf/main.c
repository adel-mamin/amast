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
 *
 * Ring buffer unit tests.
 */

#include <stddef.h>
#include <stdint.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "timer/timer.h"
#include "ringbuf/ringbuf.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "state.h"

AM_NORETURN static void ticker_task(void *param) {
    struct am_timer *timer = param;

    am_task_wait_all();

    const int domain = AM_TICK_DOMAIN_DEFAULT;
    const uint32_t ticks_per_ms = am_time_get_tick_from_ms(domain, 1);
    uint32_t now_ticks = am_time_get_tick(domain);
    for (;;) {
        am_sleep_till_ticks(domain, now_ticks + ticks_per_ms);
        now_ticks += ticks_per_ms;

        am_timer_tick_iterator_init(timer);
        struct am_timer_event *fired = NULL;
        while ((fired = am_timer_tick_iterator_next(timer)) != NULL) {
            void *owner = AM_CAST(struct am_timer_event_x *, fired)->ctx;
            if (owner) {
                am_ao_post_fifo(owner, &fired->event);
            } else {
                am_ao_publish(&fired->event);
            }
        }
    }
}

static void test_ringbuf_threading(void) {
    am_pal_ctor(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_ctor(&timer);

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    uint8_t buf[9];

    struct am_ringbuf ringbuf;

    const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7};

    am_ringbuf_ctor(&ringbuf, buf, AM_COUNTOF(buf));

    am_ao_state_ctor(/*cfg=*/NULL);

    ringbuf_reader_ctor(&ringbuf, &timer, data, (int)sizeof(data));
    ringbuf_writer_ctor(&ringbuf, &timer, data, (int)sizeof(data));

    const struct am_event *queue_reader[1];
    am_ao_start(
        ringbuf_reader_get_obj(),
        (struct am_ao_prio){.ao = AM_AO_PRIO_MID, .task = AM_AO_PRIO_MID},
        /*queue=*/queue_reader,
        /*nqueue=*/AM_COUNTOF(queue_reader),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"ringbuf_reader",
        /*init_event=*/NULL
    );

    const struct am_event *queue_writer[1];
    am_ao_start(
        ringbuf_writer_get_obj(),
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue_writer,
        /*nqueue=*/AM_COUNTOF(queue_writer),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"ringbuf_writer",
        /*init_event=*/NULL
    );

    am_task_create(
        "ticker",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/&timer
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    am_pal_dtor();
}

int main(void) {
    test_ringbuf_threading();
    return 0;
}
