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

#include "common/macros.h"
#include "timer/timer.h"
#include "ringbuf/ringbuf.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "state.h"

static void test_ringbuf_threading(void) {
    am_pal_global_init(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_init(
        &timer,
        &(struct am_timer_cfg){
            .crit_enter = am_crit_enter, .crit_exit = am_crit_exit
        }
    );

    uint8_t buf[9];

    struct am_ringbuf ringbuf;

    const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7};

    am_ringbuf_init(&ringbuf, buf, AM_COUNTOF(buf));

    am_ao_global_init(&(struct am_ao_global_cfg){
        .crit_enter = am_crit_enter,
        .crit_exit = am_crit_exit,
    });

    ringbuf_reader_init(&ringbuf, &timer, data, (int)sizeof(data));
    ringbuf_writer_init(&ringbuf, &timer, data, (int)sizeof(data));

    const struct am_event* queue_reader[1];
    am_ao_start(
        ringbuf_reader_get_ao(),
        &(struct am_ao_cfg){
            .name = "ringbuf_reader",
            .prio = am_ao_prio_make(AM_AO_PRIO_MID),
            .queue = queue_reader,
            .queue_size = AM_COUNTOF(queue_reader),
        }
    );

    const struct am_event* queue_writer[1];
    am_ao_start(
        ringbuf_writer_get_ao(),
        &(struct am_ao_cfg){
            .name = "ringbuf_writer",
            .prio = am_ao_prio_make(AM_AO_PRIO_MAX),
            .queue = queue_writer,
            .queue_size = AM_COUNTOF(queue_writer),
        }
    );

    int ticker = am_ticker_create(&(struct am_ticker_cfg){
        .name = "timebase_ms",
        .timebase = AM_TIMEBASE_MS,
        .tick_cb = am_ao_timer_tick_cb,
        .ctx = &timer,
        .priority_hint = AM_AO_PRIO_MIN
    });
    am_ticker_start(ticker);

    am_ao_run();

    am_ticker_stop(ticker);

    am_ao_global_deinit();

    am_pal_global_deinit();
}

int main(void) {
    test_ringbuf_threading();
    return 0;
}
