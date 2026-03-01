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

#include <string.h>
#include <stdint.h>

#include "common/macros.h"
#include "ringbuf/ringbuf.h"
#include "timer/timer.h"
#include "event/event.h"
#include "ao/ao.h"
#include "state.h"

struct ringbuf_writer {
    struct am_ao ao;
    struct am_ringbuf *ringbuf;
    struct am_timer *timer;
    struct am_timer_event_x wait;
    const uint8_t *data;
    int datalen;
    int len;
};

static struct ringbuf_writer m_ringbuf_writer;

static const struct am_event m_evt_ringbuf_write = {.id = AM_EVT_RINGBUF_WRITE};

static void ringbuf_writer_init_handler(
    struct ringbuf_writer *me, const struct am_event *event
) {
    (void)event;
    am_ao_post_fifo(&me->ao, &m_evt_ringbuf_write);
}

static void ringbuf_writer_event_handler(
    struct ringbuf_writer *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_RINGBUF_WAIT:
    case AM_EVT_RINGBUF_WRITE: {
        uint8_t *ptr = NULL;
        int size = me->len;
        (void)am_ringbuf_get_write_ptr(me->ringbuf, &ptr, &size);
        if (size < me->len) {
            am_timer_arm(
                me->timer, &me->wait.event, /*ticks=*/1, /*interval=*/0
            );
            return;
        }
        AM_ASSERT(ptr);
        memcpy(ptr, me->data, (size_t)me->len);
        am_ringbuf_flush(me->ringbuf, me->len);
        me->len = (me->len + 1) % me->datalen;
        if (0 == me->len) {
            me->len = 1;
        }
        am_ao_post_fifo(&me->ao, &m_evt_ringbuf_write);
        return;
    }
    default:
        break;
    }
}

void ringbuf_writer_ctor(
    struct am_ringbuf *ringbuf,
    struct am_timer *timer,
    const uint8_t *data,
    int len
) {
    struct ringbuf_writer *me = &m_ringbuf_writer;
    memset(me, 0, sizeof(*me));
    me->len = 1;
    am_ao_ctor(
        &me->ao,
        (am_ao_fn)ringbuf_writer_init_handler,
        (am_ao_fn)ringbuf_writer_event_handler,
        me
    );
    me->ringbuf = ringbuf;
    me->data = data;
    me->datalen = len;
    me->timer = timer;
    me->wait = am_timer_event_ctor_x(AM_EVT_RINGBUF_WAIT, &me->ao);
}

struct am_ao *ringbuf_writer_get_obj(void) { return &m_ringbuf_writer.ao; }
