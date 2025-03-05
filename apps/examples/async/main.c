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
 *  +----------------------------------+
 *  |            async_top             |
 *  | +-------------+  +-------------+ |
 *  | |async_regular|  |  async_off  | |
 *  | +-------------+  +-------------+ |
 *  +----------------------------------+
 *
 * It mimics the behavior of traffic lights.
 *
 * The async_regular substate prints colored rectangles in the order
 * red - yellow - green - blinking green
 * The print delay of each rectangle is different.
 *
 * The async_off substate shown blinking yellow.
 * The blink delay is 700 ms.
 *
 * async_top handles user input. Press ENTER to switch between
 * async_regular and async_off substates.
 *
 * Generally the use of async is warranted if the sequence of
 * steps is known beforehand like in this case.
 *
 * async_regular calls async_regular_() to do the printing
 * async_off calls async_off_() to do the printing
 *
 * The example is built in two flavours:
 * async_preemptive and async_cooperative.
 * Both builds do the same.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common/compiler.h"
#include "common/constants.h"
#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"
#include "async/async.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define CHAR_SOLID_BLOCK "\xE2\x96\x88"
#define CHAR_CURSOR_UP "\033[A"

enum {
    ASYNC_EVT_USER_INPUT = AM_EVT_USER,
    ASYNC_EVT_TIMER,
    ASYNC_EVT_START,
};

struct async {
    struct am_ao ao;
    struct am_timer timer;
    struct am_async async;
    int i;
};

static const struct am_event am_evt_start = {.id = ASYNC_EVT_START};

static enum am_hsm_rc async_top(struct async *me, const struct am_event *event);
static enum am_hsm_rc async_regular(
    struct async *me, const struct am_event *event
);
static enum am_hsm_rc async_off(struct async *me, const struct am_event *event);

static enum am_hsm_rc async_top(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_INIT:
        return AM_HSM_TRAN(async_regular);

    case ASYNC_EVT_USER_INPUT: {
        am_pal_printf("\b");
        am_pal_flush();
        if (am_hsm_is_in(&me->ao.hsm, AM_HSM_STATE_CTOR(async_regular))) {
            return AM_HSM_TRAN(async_off);
        }
        return AM_HSM_TRAN(async_regular);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_async_rc async_regular_(struct async *me) {
    AM_ASYNC_BEGIN(&me->async);

    /* red */
    am_pal_printf(AM_COLOR_RED CHAR_SOLID_BLOCK AM_COLOR_RESET);
    am_pal_flush();
    am_timer_arm_ms(&me->timer, /*ms=*/2000, /*interval=*/0);
    AM_ASYNC_YIELD();

    /* yellow */
    am_pal_printf("\b" AM_COLOR_YELLOW CHAR_SOLID_BLOCK AM_COLOR_RESET);
    am_pal_flush();
    am_timer_arm_ms(&me->timer, /*ms=*/1000, /*interval=*/0);
    AM_ASYNC_YIELD();

    /* green */
    am_pal_printf("\b" AM_COLOR_GREEN CHAR_SOLID_BLOCK AM_COLOR_RESET);
    am_pal_flush();
    am_timer_arm_ms(&me->timer, /*ms=*/2000, /*interval=*/0);
    AM_ASYNC_YIELD();

    for (me->i = 0; me->i < 4; ++me->i) {
        am_pal_printf("\b");
        am_pal_flush();
        am_timer_arm_ms(&me->timer, /*ms=*/700, /*interval=*/0);
        AM_ASYNC_YIELD();

        am_pal_printf(AM_COLOR_GREEN CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        am_timer_arm_ms(&me->timer, /*ms=*/700, /*interval=*/0);
        AM_ASYNC_YIELD();
    }
    am_pal_printf("\b");
    am_pal_flush();

    am_ao_post_fifo(&me->ao, &am_evt_start);

    AM_ASYNC_END();

    return AM_ASYNC_RC(&me->async);
}

static enum am_hsm_rc async_regular(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_async_ctor(&me->async);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();

    case AM_EVT_HSM_EXIT:
        am_timer_disarm(&me->timer);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_START:
    case ASYNC_EVT_TIMER: {
        (void)async_regular_(me);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_async_rc async_off_(struct async *me) {
    AM_ASYNC_BEGIN(&me->async);

    am_timer_arm_ms(&me->timer, /*ms=*/1000, /*interval=*/0);
    am_pal_printf("\b" AM_COLOR_YELLOW CHAR_SOLID_BLOCK AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    am_timer_arm_ms(&me->timer, /*ms=*/700, /*interval=*/0);
    am_pal_printf("\b");
    am_pal_flush();
    AM_ASYNC_YIELD();

    am_ao_post_fifo(&me->ao, &am_evt_start);

    AM_ASYNC_END();

    return AM_ASYNC_RC(&me->async);
}

static enum am_hsm_rc async_off(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_async_ctor(&me->async);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();

    case AM_EVT_HSM_EXIT:
        am_timer_disarm(&me->timer);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_START:
    case ASYNC_EVT_TIMER: {
        (void)async_off_(me);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_hsm_rc async_init(
    struct async *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(async_top);
}

static void async_ctor(struct async *me) {
    memset(me, 0, sizeof(*me));

    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(async_init));

    am_timer_ctor(
        &me->timer,
        ASYNC_EVT_TIMER,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT,
        &me->ao
    );
}

AM_NORETURN static void ticker_task(void *param) {
    (void)param;

    am_ao_wait_start_all();

    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    for (;;) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

static void input_task(void *param) {
    am_ao_wait_start_all();

    struct async *m = (struct async *)param;
    int ch;
    while ((ch = getc(stdin)) != EOF) {
        if (ch != '\n') {
            continue;
        }
        am_pal_printf(CHAR_CURSOR_UP);
        am_pal_flush();
        static struct am_event event = {.id = ASYNC_EVT_USER_INPUT};
        am_ao_post_fifo_x(&m->ao, &event, /*margin=*/0);
    }
}

int main(void) {
    struct am_ao_state_cfg cfg = {
        .on_idle = am_pal_on_idle,
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg);

    struct async m;
    async_ctor(&m);

    static const struct am_event *m_queue[1];

    am_ao_start(
        &m.ao,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue,
        /*nqueue=*/AM_COUNTOF(m_queue),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"async",
        /*init_event=*/NULL
    );

    am_pal_task_create(
        "ticker",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/NULL
    );

    am_pal_task_create(
        "input",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/input_task,
        /*arg=*/&m
    );

    for (;;) {
        am_ao_run_all();
    }

    return 0;
}
