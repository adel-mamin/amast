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
 * Minimal example of two active objects sending static messages to each other.
 * No event pool allocation is needed.
 * No pub/sub memory allocation is needed.
 */

#include <stddef.h>
#include <stdlib.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"
#include "ao/ao.h"

#define AM_EVT_MIN AM_EVT_USER

static const struct am_event m_min_event = {.id = AM_EVT_MIN};

static const struct am_event *m_queue_loopback[2];
static const struct am_event *m_queue_loopback_test[2];

static struct loopback {
    struct am_ao ao;
} m_loopback;

static struct loopback_test {
    struct am_ao ao;
    int cnt;
} m_loopback_test;

static int loopback_proc(struct loopback *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_MIN:
        am_ao_post_fifo(&m_loopback_test.ao, event);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int loopback_init(struct loopback *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(loopback_proc);
}

static int loopback_test_proc(
    struct loopback_test *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        am_ao_post_fifo(&m_loopback.ao, &m_min_event);
        return AM_HSM_HANDLED();
    case AM_EVT_MIN:
        me->cnt++;
        if (100 == me->cnt) {
            exit(0);
        }
        am_ao_post_fifo(&m_loopback.ao, event);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int loopback_test_init(
    struct loopback_test *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(loopback_test_proc);
}

int main(void) {
    struct am_ao_state_cfg cfg_ao = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg_ao);

    am_ao_ctor(&m_loopback.ao, &AM_HSM_STATE_CTOR(loopback_init));
    am_ao_ctor(&m_loopback_test.ao, &AM_HSM_STATE_CTOR(loopback_test_init));

    am_ao_start(
        &m_loopback.ao,
        /*prio=*/AM_AO_PRIO_MAX - 1,
        /*queue=*/m_queue_loopback,
        /*nqueue=*/AM_COUNTOF(m_queue_loopback),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"loopback",
        /*init_event=*/NULL
    );

    am_ao_start(
        &m_loopback_test.ao,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue_loopback_test,
        /*nqueue=*/AM_COUNTOF(m_queue_loopback_test),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"loopback",
        /*init_event=*/NULL
    );

    am_ao_run_all(/*loop=*/1);

    return 0;
}
