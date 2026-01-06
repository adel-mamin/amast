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

/*
 * Demonstrate integration of async with HSM.
 * HSM topology:
 *
 *  +--------------------------------------------------------+
 *  |                    am_hsm_top                          |
 *  | +----------------------------------+                   |
 *  | |            async_top             |                   |
 *  | | +-------------+  +-------------+ | +---------------+ |
 *  | | |async_regular|  |  async_off  | | | async_exiting | |
 *  | | +-------------+  +-------------+ | +---------------+ |
 *  | +----------------------------------+                   |
 *  +--------------------------------------------------------+
 *
 * It mimics the behavior of traffic lights.
 *
 * The async_regular substate prints colored rectangles in the order
 * red - yellow - green - blinking green
 * The print delay of each rectangle varies.
 *
 * The async_off substate shows blinking yellow mimicking unregulated
 * road intersection.
 * The blink delay is 700 ms.
 *
 * async_exiting substate calls am_ao_stop().
 *
 * async_top handles user input:
 * - press ENTER to switch between async_regular and async_off substates
 * - press ENTER two times in a row within 500ms to switch to async_exiting
 *   and exit the app
 *
 * Generally the use of async is warranted, if the sequence of
 * steps can be represented as a flowchart like in this case.
 *
 * async_regular calls async_regular_() to do the printing
 * async_off calls async_off_() to do the printing.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common/alignment.h"
#include "common/constants.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "timer/timer.h"
#include "async/async.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define ASYNC_TWO_NEWLINES_TIMEOUT_MS 500

enum {
    ASYNC_EVT_SWITCH_MODE = AM_EVT_USER,
    ASYNC_EVT_TIMER,
    ASYNC_EVT_EXIT,
    ASYNC_EVT_PUB_MAX,
    ASYNC_EVT_START,
};

struct async {
    struct am_ao ao;
    struct am_timer *timer;
    struct am_async async;
    struct am_async async_blinking_green;
    unsigned i;
};

static const struct am_event am_evt_start = {.id = ASYNC_EVT_START};

static struct am_timer m_event_pool[1];
static struct am_ao_subscribe_list m_pubsub_list[ASYNC_EVT_PUB_MAX];
static const struct am_event *m_queue[2];

static enum am_rc async_top(struct async *me, const struct am_event *event);
static enum am_rc async_regular(struct async *me, const struct am_event *event);
static enum am_rc async_off(struct async *me, const struct am_event *event);
static enum am_rc async_exiting(struct async *me, const struct am_event *event);

static enum am_rc async_top(struct async *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_INIT: {
        return AM_HSM_TRAN(async_regular);
    }
    case ASYNC_EVT_SWITCH_MODE: {
        am_printff("\b");
        if (am_hsm_is_in(&me->ao.hsm, AM_HSM_STATE_CTOR(async_regular))) {
            return AM_HSM_TRAN(async_off);
        }
        return AM_HSM_TRAN(async_regular);
    }
    case ASYNC_EVT_EXIT: {
        return AM_HSM_TRAN_REDISPATCH(async_exiting);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc async_exiting(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case ASYNC_EVT_EXIT: {
        am_ao_stop(&me->ao);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc async_blinking_green(struct async *me) {
    AM_ASYNC_BEGIN(&me->async_blinking_green);

    /* blinking green */
    for (me->i = 0; me->i < 4; ++me->i) {
        am_printff("\b");
        am_timer_arm_ms(me->timer, /*ms=*/700, /*interval=*/0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));

        am_printff(AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm_ms(me->timer, /*ms=*/700, /*interval=*/0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));
    }

    AM_ASYNC_END();

    return AM_RC_DONE;
}

static enum am_rc async_regular_(struct async *me) {
    AM_ASYNC_BEGIN(&me->async);

    for (;;) {
        /* red */
        am_printff(AM_COLOR_RED AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm_ms(me->timer, /*ms=*/2000, /*interval=*/0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));

        /* yellow */
        am_printff("\b" AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm_ms(me->timer, /*ms=*/1000, /*interval=*/0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));

        /* green */
        am_printff("\b" AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm_ms(me->timer, /*ms=*/2000, /*interval=*/0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));

        /* blinking green */
        AM_ASYNC_CHAIN(async_blinking_green(me));
        am_printff("\b");
    }

    AM_ASYNC_END();

    return AM_RC_DONE;
}

static enum am_rc async_regular(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_async_ctor(&me->async);
        am_async_ctor(&me->async_blinking_green);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer);
        return AM_HSM_HANDLED();
    }
    case ASYNC_EVT_START:
    case ASYNC_EVT_TIMER: {
        return async_regular_(me);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_rc async_off(struct async *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_async_ctor(&me->async);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer);
        return AM_HSM_HANDLED();
    }
    case ASYNC_EVT_START:
    case ASYNC_EVT_TIMER: {
        AM_ASYNC_BEGIN(&me->async);

        for (;;) {
            am_printff("\b");
            am_printff(AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
            am_timer_arm_ms(me->timer, /*ms=*/1000, /*interval=*/0);
            AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));

            am_printff("\b");
            am_timer_arm_ms(me->timer, /*ms=*/700, /*interval=*/0);
            AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer));
        }

        AM_ASYNC_END();

        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_rc async_init(struct async *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, ASYNC_EVT_SWITCH_MODE);
    am_ao_subscribe(&me->ao, ASYNC_EVT_EXIT);
    return AM_HSM_TRAN(async_top);
}

static void async_ctor(struct async *me) {
    memset(me, 0, sizeof(*me));

    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(async_init));

    me->timer = am_timer_allocate(
        ASYNC_EVT_TIMER, sizeof(*me->timer), AM_TICK_DOMAIN_DEFAULT, &me->ao
    );
}

static void ticker_task(void *param) {
    (void)param;

    am_task_wait_all();

    uint32_t now_ticks = am_time_get_tick(AM_TICK_DOMAIN_DEFAULT);
    while (am_ao_get_cnt() > 0) {
        am_sleep_till_ticks(AM_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_TICK_DOMAIN_DEFAULT);
    }
}

static void input_task(void *param) {
    (void)param;

    am_task_wait_all();

    int ch;
    uint32_t prev_ms = am_time_get_ms() - 2 * ASYNC_TWO_NEWLINES_TIMEOUT_MS;
    while ((ch = getc(stdin)) != EOF) {
        if ('\n' != ch) {
            continue;
        }
        am_printff(AM_CURSOR_UP);
        uint32_t now_ms = am_time_get_ms();
        uint32_t diff_ms = now_ms - prev_ms;
        prev_ms = now_ms;
        if (diff_ms > ASYNC_TWO_NEWLINES_TIMEOUT_MS) {
            static struct am_event event = {.id = ASYNC_EVT_SWITCH_MODE};
            am_ao_publish(&event);
            continue;
        }
        static struct am_event event = {.id = ASYNC_EVT_EXIT};
        am_ao_publish(&event);
        break;
    }
}

int main(void) {
    am_ao_state_ctor(/*cfg=*/NULL);

    am_event_pool_add(
        m_event_pool,
        sizeof(m_event_pool),
        sizeof(m_event_pool[0]),
        AM_ALIGNOF(am_timer_t)
    );

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    struct async m;
    async_ctor(&m);

    /* traffic lights controlling active object */
    am_ao_start(
        &m.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/m_queue,
        /*nqueue=*/AM_COUNTOF(m_queue),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"async",
        /*init_event=*/NULL
    );

    /* ticker thread to feed timers */
    am_task_create(
        "ticker",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/NULL
    );

    /* user input controlling thread */
    am_task_create(
        "input",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/input_task,
        /*arg=*/&m
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    return 0;
}
