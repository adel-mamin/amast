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

#define SOLID_BLOCK_CHAR "\xE2\x96\x88"
#define CURSOR_UP_CHAR "\033[A"

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

static enum am_async_rc async_ryg_hnd(struct am_async *me) {
    AM_ASYNC_BEGIN(me);

    /* red */
    am_pal_printf(AM_COLOR_RED SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    /* yellow */
    am_pal_printf(" " AM_COLOR_YELLOW SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    /* green */
    am_pal_printf(" " AM_COLOR_GREEN SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    am_pal_printf("\r               \r");
    am_pal_flush();
    AM_ASYNC_YIELD();

    AM_ASYNC_END();
}

static enum am_hsm_rc async_ryg(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        am_async_init(&me->async_ryg);
        am_timer_arm(
            &me->timer_ryg, me, me->ryg_interval_ticks, me->ryg_interval_ticks
        );
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        am_timer_disarm(&me->timer_ryg);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_TIMER_RYG:
        async_ryg_hnd(&me->async_ryg);
        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_async_rc async_gyr_hnd(struct am_async *me) {
    AM_ASYNC_BEGIN(me);

    /* green */
    am_pal_printf(AM_COLOR_GREEN SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    /* yellow */
    am_pal_printf(" " AM_COLOR_YELLOW SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    /* red */
    am_pal_printf(" " AM_COLOR_RED SOLID_BLOCK_CHAR AM_COLOR_RESET);
    am_pal_flush();
    AM_ASYNC_YIELD();

    am_pal_printf("\r               \r");
    am_pal_flush();
    AM_ASYNC_YIELD();

    AM_ASYNC_END();
}

static enum am_hsm_rc async_gyr(
    struct async *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        am_async_init(&me->async_gyr);
        am_timer_arm(
            &me->timer_gyr, me, me->gyr_interval_ticks, me->gyr_interval_ticks
        );
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        am_timer_disarm(&me->timer_gyr);
        return AM_HSM_HANDLED();

    case ASYNC_EVT_TIMER_GYR:
        async_gyr_hnd(&me->async_gyr);
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
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT
    );
    am_timer_event_ctor(
        &me->timer_ryg,
        ASYNC_EVT_TIMER_RYG,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT
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
    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    for (;;) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

static void input_task(void *param) {
    struct async *m = (struct async *)param;
    int ch;
    while ((ch = getc(stdin)) != EOF) {
        am_pal_printf(CURSOR_UP_CHAR);
        am_pal_flush();
        static struct am_event event = {.id = ASYNC_EVT_USER_INPUT};
        am_ao_post_fifo(&m->ao, &event);
    }
}

int main(void) {
    struct am_ao_state_cfg cfg = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg);

    struct async m;
    async_ctor(&m);

    static const struct am_event *m_queue[2];

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
        if (!am_ao_run_all()) {
            am_pal_crit_exit();
        }
    }

    return 0;
}
