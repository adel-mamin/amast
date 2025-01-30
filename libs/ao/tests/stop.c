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
 * Unit test am_ao_stop() API.
 * Creates a single AO and waits till it stops itself.
 */

#include <stddef.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"
#include "ao/ao.h"

#define AM_EVT_SELF_STOP AM_EVT_USER

static const struct am_event *m_queue_test[1];

static struct test {
    struct am_ao ao;
} m_test;

static int test_proc(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: {
        static const struct am_event stop = {.id = AM_EVT_SELF_STOP};
        am_ao_post_fifo(&me->ao, &stop);
        break;
    }
    case AM_EVT_SELF_STOP:
        am_ao_stop(&me->ao);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int test_init(struct test *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(test_proc);
}

int main(void) {
    struct am_ao_state_cfg cfg_ao = {
        .on_idle = am_pal_on_idle,
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg_ao);

    am_ao_ctor(&m_test.ao, AM_HSM_STATE_CTOR(test_init));

    am_ao_start(
        &m_test.ao,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue_test,
        /*nqueue=*/AM_COUNTOF(m_queue_test),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"test",
        /*init_event=*/NULL
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    return 0;
}
