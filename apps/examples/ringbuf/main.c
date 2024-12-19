/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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
#include "ringbuf/ringbuf.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "state.h"

struct am_ringbuf_desc g_ringbuf;

const uint8_t g_ringbuf_data[] = {0, 1, 2, 3, 4, 5, 6, 7};
int g_ringbuf_data_len = AM_COUNTOF(g_ringbuf_data);

static const struct am_event *m_queue_ringbuf_reader[2];
static const struct am_event *m_queue_ringbuf_writer[2];

static void test_ringbuf_threading(void) {
    uint8_t buf[32];

    am_ringbuf_ctor(&g_ringbuf, buf, AM_COUNTOF(buf));

    struct am_ao_state_cfg cfg_ao = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg_ao);

    ringbuf_reader_ctor();
    ringbuf_writer_ctor();

    am_ao_start(
        g_ringbuf_reader,
        /*prio=*/AM_AO_PRIO_MAX - 1,
        /*queue=*/m_queue_ringbuf_reader,
        /*nqueue=*/AM_COUNTOF(m_queue_ringbuf_reader),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"ringbuf_reader",
        /*init_event=*/NULL
    );

    am_ao_start(
        g_ringbuf_writer,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue_ringbuf_writer,
        /*nqueue=*/AM_COUNTOF(m_queue_ringbuf_writer),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"ringbuf_writer",
        /*init_event=*/NULL
    );

    am_ao_run_all(/*loop=*/1);
}

int main(void) {
    test_ringbuf_threading();
    return 0;
}
