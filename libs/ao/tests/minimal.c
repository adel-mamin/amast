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
 * No timers.
 */

#include <stddef.h>
#include <stdlib.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"
#include "ao/ao.h"

#define AM_EVT_MIN AM_EVT_USER
#define AM_EVT_SHUTDOWN (AM_EVT_USER + 1)
#define AM_EVT_START_TEST (AM_EVT_USER + 2)

static const struct am_event m_min_event = {.id = AM_EVT_MIN};
static const struct am_event m_start_test_event = {.id = AM_EVT_START_TEST};

static const struct am_event *m_queue_loopback[1];
static const struct am_event *m_queue_loopback_test[1];

static struct loopback {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
} m_loopback;

static struct loopback_test {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    int cnt;
} m_loopback_test;

static struct am_event event_shutdown_ = {.id = AM_EVT_SHUTDOWN};

static int loopback_proc(struct loopback *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_MIN:
        AM_ASSERT(am_ao_get_own_prio() == AM_AO_PRIO_HIGH);
        am_ao_post_fifo(&m_loopback_test.ao, event);
        return AM_HSM_HANDLED();

    case AM_EVT_SHUTDOWN:
        am_ao_stop(&me->ao);
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
    case AM_EVT_START_TEST:
        am_ao_post_fifo(&m_loopback.ao, &m_min_event);
        return AM_HSM_HANDLED();

    case AM_EVT_MIN:
        AM_ASSERT(am_ao_get_own_prio() == AM_AO_PRIO_MAX);
        ++me->cnt;
        if (100 == me->cnt) {
            am_ao_post_fifo(&m_loopback.ao, &event_shutdown_);
            am_ao_stop(&me->ao);
            return AM_HSM_HANDLED();
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
    am_ao_post_fifo(&me->ao, &m_start_test_event);
    return AM_HSM_TRAN(loopback_test_proc);
}

int main(void) {
    am_ao_state_ctor(/*cfg=*/NULL);

    am_ao_ctor(
        &m_loopback.ao,
        (am_ao_fn)am_hsm_init,
        (am_ao_fn)am_hsm_dispatch,
        &m_loopback
    );
    am_hsm_ctor(&m_loopback.hsm, AM_HSM_STATE_CTOR(loopback_init));

    am_ao_ctor(
        &m_loopback_test.ao,
        (am_ao_fn)am_hsm_init,
        (am_ao_fn)am_hsm_dispatch,
        &m_loopback_test
    );
    am_hsm_ctor(&m_loopback_test.hsm, AM_HSM_STATE_CTOR(loopback_test_init));

    am_ao_start(
        &m_loopback.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_HIGH, .task = AM_AO_PRIO_HIGH},
        /*queue=*/m_queue_loopback,
        /*nqueue=*/AM_COUNTOF(m_queue_loopback),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"loopback",
        /*init_event=*/NULL
    );

    am_ao_start(
        &m_loopback_test.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/m_queue_loopback_test,
        /*nqueue=*/AM_COUNTOF(m_queue_loopback_test),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"loopback_test",
        /*init_event=*/NULL
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    return 0;
}
