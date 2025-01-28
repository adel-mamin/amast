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
 *  +--------------------------+
 *  |       async_top          |
 *  | +---------+  +---------+ |
 *  | |async_ryg|  |async_gyr| |
 *  | +---------+  +---------+ |
 *  +--------------------------+
 *
 * async_ryg substate prints colored rectangles in the order
 * red-yellow-green (ryg)
 * The print delay of each rectangle is 500ms
 *
 * async_ryg substate prints colored rectangles in the order
 * green-yellow-red (gyr)
 * The print delay of each rectangle is 1000ms
 *
 * async_top handles user input. Press ENTER to switch between
 * async_ryg and async_gyr substates.
 *
 * Generally the use of async is warranted if the sequence of
 * steps is known beforehand like in this case.
 * async_ryg calls async_ryg_hnd() to do the printing
 * async_gyr calls async_gyr_hnd() to do the printing
 *
 * The example is built in two flavours:
 * async_preemptive and async_cooperative.
 * Both builds do the same.
 */

#include <stdio.h>
#include <stdint.h>

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

#define ASYNC_EVT_USER_INPUT AM_EVT_USER
#define ASYNC_EVT_TIMER_RYG (AM_EVT_USER + 1)
#define ASYNC_EVT_TIMER_GYR (AM_EVT_USER + 2)

struct async {
    struct am_ao ao;
    int ryg_interval_ticks;
    int gyr_interval_ticks;
    struct am_event_timer timer_ryg;
    struct am_event_timer timer_gyr;
    struct am_async async_ryg;
    struct am_async async_gyr;
};

static enum am_hsm_rc async_top(struct async *me, const struct am_event *event);
static enum am_hsm_rc async_ryg(struct async *me, const struct am_event *event);
static enum am_hsm_rc async_gyr(struct async *me, const struct am_event *event);

static enum am_hsm_rc async_top(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_INIT:
        return AM_HSM_TRAN(async_ryg);

    case ASYNC_EVT_USER_INPUT: {
        am_pal_printf("\r               \r");
        am_pal_flush();
        if (am_hsm_is_in(&me->ao.hsm, AM_HSM_STATE_CTOR(async_ryg))) {
            return AM_HSM_TRAN(async_gyr);
        }
        return AM_HSM_TRAN(async_ryg);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc async_ryg(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        am_async_init(&me->async_ryg);
        am_timer_arm(
            &me->timer_ryg, me->ryg_interval_ticks, me->ryg_interval_ticks
        );
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        am_timer_disarm(&me->timer_ryg);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_TIMER_RYG:
        AM_ASYNC_BEGIN(&me->async_ryg);

        /* red */
        am_pal_printf(AM_COLOR_RED CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        /* yellow */
        am_pal_printf(" " AM_COLOR_YELLOW CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        /* green */
        am_pal_printf(" " AM_COLOR_GREEN CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        am_pal_printf("\r               \r");
        am_pal_flush();
        AM_ASYNC_YIELD();

        AM_ASYNC_END();

        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_hsm_rc async_gyr(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        am_async_init(&me->async_gyr);
        am_timer_arm(
            &me->timer_gyr, me->gyr_interval_ticks, me->gyr_interval_ticks
        );
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        am_timer_disarm(&me->timer_gyr);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_TIMER_GYR:
        AM_ASYNC_BEGIN(&me->async_gyr);

        /* green */
        am_pal_printf(AM_COLOR_GREEN CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        /* yellow */
        am_pal_printf(" " AM_COLOR_YELLOW CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        /* red */
        am_pal_printf(" " AM_COLOR_RED CHAR_SOLID_BLOCK AM_COLOR_RESET);
        am_pal_flush();
        AM_ASYNC_YIELD();

        am_pal_printf("\r               \r");
        am_pal_flush();
        AM_ASYNC_YIELD();

        AM_ASYNC_END();

        return AM_HSM_HANDLED();

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
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(async_init));

    am_timer_event_ctor(
        &me->timer_gyr,
        ASYNC_EVT_TIMER_GYR,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT,
        &me->ao
    );
    am_timer_event_ctor(
        &me->timer_ryg,
        ASYNC_EVT_TIMER_RYG,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT,
        &me->ao
    );

    me->ryg_interval_ticks = (int)am_pal_time_get_tick_from_ms(
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT, /*ms=*/500
    );
    me->gyr_interval_ticks = (int)am_pal_time_get_tick_from_ms(
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT, /*ms=*/1000
    );
}

AM_NORETURN static void ticker_task(void *param) {
    (void)param;

    am_ao_wait_startup();

    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    for (;;) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

static void input_task(void *param) {
    am_ao_wait_startup();

    struct async *m = (struct async *)param;
    int ch;
    while ((ch = getc(stdin)) != EOF) {
        if (ch != '\n') {
            continue;
        }
        am_pal_printf(CHAR_CURSOR_UP);
        am_pal_flush();
        static struct am_event event = {.id = ASYNC_EVT_USER_INPUT};
        am_ao_post_fifo_x(&m->ao, &event, /*margin=*/1);
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
