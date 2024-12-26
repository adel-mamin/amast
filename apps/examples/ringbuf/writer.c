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

struct ringbuf_writer {
    struct am_ao ao;
    struct am_event_timer timer_wait;
    int len;
};

static struct ringbuf_writer m_ringbuf_writer;
struct am_ao *g_ringbuf_writer = &m_ringbuf_writer.ao;

static const struct am_event m_evt_ringbuf_write = {.id = AM_EVT_RINGBUF_WRITE};

static int ringbuf_writer_proc(
    struct ringbuf_writer *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        am_ao_post_fifo(&me->ao, &m_evt_ringbuf_write);
        return AM_HSM_HANDLED();
    }
    case AM_EVT_RINGBUF_WAIT:
    case AM_EVT_RINGBUF_WRITE: {
        uint8_t *ptr = NULL;
        int size = am_ringbuf_get_write_ptr(&g_ringbuf, &ptr, me->len);
        if (size < me->len) {
            am_timer_arm(&me->timer_wait, &me->ao, /*ticks=*/1, /*interval=*/0);
            return AM_HSM_HANDLED();
        }
        AM_ASSERT(ptr);
        memcpy(ptr, g_ringbuf_data, (size_t)me->len);
        am_ringbuf_flush(&g_ringbuf, me->len);
        me->len = (me->len + 1) % g_ringbuf_data_len;
        if (0 == me->len) {
            me->len = 1;
        }
        am_ao_post_fifo(&me->ao, &m_evt_ringbuf_write);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int ringbuf_writer_init(
    struct ringbuf_writer *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(ringbuf_writer_proc);
}

void ringbuf_writer_ctor(void) {
    struct ringbuf_writer *me = &m_ringbuf_writer;
    memset(me, 0, sizeof(*me));
    me->len = 1;
    am_ao_ctor(&me->ao, &AM_HSM_STATE_CTOR(ringbuf_writer_init));
    am_timer_event_ctor(
        &me->timer_wait,
        /*id=*/AM_EVT_RINGBUF_WAIT,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT
    );
}
