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
 * Ring buffer reader.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common/macros.h"
#include "ringbuf/ringbuf.h"
#include "timer/timer.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

#define AM_TEST_RINGBUF_TOTAL 1000

struct ringbuf_reader {
    struct am_ao ao;
    int len;
    int total_len;
    struct am_timer timer_wait;
};

static struct ringbuf_reader m_ringbuf_reader;
struct am_ao *g_ringbuf_reader = &m_ringbuf_reader.ao;

static const struct am_event m_evt_ringbuf_read = {.id = AM_EVT_RINGBUF_READ};

static int ringbuf_reader_proc(
    struct ringbuf_reader *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_RINGBUF_WAIT:
    case AM_EVT_RINGBUF_READ: {
        uint8_t *ptr = NULL;
        int size = am_ringbuf_get_read_ptr(&g_ringbuf, &ptr);
        if (size < me->len) {
            am_timer_arm_ticks(&me->timer_wait, /*ticks=*/1, /*interval=*/0);
            return AM_HSM_HANDLED();
        }
        AM_ASSERT(ptr);
        AM_ASSERT(0 == memcmp(ptr, g_ringbuf_data, (size_t)me->len));
        am_ringbuf_seek(&g_ringbuf, me->len);
        me->total_len += me->len;
        if (me->total_len >= AM_TEST_RINGBUF_TOTAL) {
            exit(0);
        }
        me->len = (me->len + 1) % g_ringbuf_data_len;
        if (0 == me->len) {
            me->len = 1;
        }
        am_ao_post_fifo(&me->ao, &m_evt_ringbuf_read);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int ringbuf_reader_init(
    struct ringbuf_reader *me, const struct am_event *event
) {
    (void)event;
    am_ao_post_fifo(&me->ao, &m_evt_ringbuf_read);
    return AM_HSM_TRAN(ringbuf_reader_proc);
}

void ringbuf_reader_ctor(void) {
    struct ringbuf_reader *me = &m_ringbuf_reader;
    memset(me, 0, sizeof(*me));
    me->len = 1;
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(ringbuf_reader_init));
    am_timer_ctor(
        &me->timer_wait,
        /*id=*/AM_EVT_RINGBUF_WAIT,
        /*domain=*/AM_TICK_DOMAIN_DEFAULT,
        &me->ao
    );
}
