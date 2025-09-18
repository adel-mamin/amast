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
 * A task 'watched' is monitored by a watchdog task 'wdt'.
 * The 'watched' tasks works fine for 3 seconds, after which
 * it fails to feed 'wdt' in time.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "timer/timer.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define AM_WDT_FEED_TIMEOUT_MS 1000
#define AM_WDT_BARK_TIMEOUT_MS (AM_WDT_FEED_TIMEOUT_MS + 100)

enum evt {
    EVT_WATCHED_TIMEOUT = AM_EVT_USER,
    EVT_WDT_FEED,
    EVT_WDT_BARK,
};

struct watched {
    struct am_ao ao;
    struct am_timer timer_wdt_feed;
    int feeds_num;
};

struct wdt {
    struct am_ao ao;
    struct am_timer timer_wdt_bark;
};

/* tasks */
static struct watched m_watched;
static struct wdt m_wdt;

/* task event queues */
static const struct am_event *m_queue_watched[1];
static const struct am_event *m_queue_wdt[2];

/* task events */
static const struct am_event m_evt_wdt_feed = {.id = EVT_WDT_FEED};

/* 'watched' task */

static enum am_rc watched_proc(
    struct watched *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm_ms(
            &me->timer_wdt_feed, AM_WDT_FEED_TIMEOUT_MS, AM_WDT_FEED_TIMEOUT_MS
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WATCHED_TIMEOUT: {
        if (me->feeds_num < 3) {
            am_pal_printff("EVT_WDT_FEED sent\n");
            am_ao_post_fifo(&m_wdt.ao, &m_evt_wdt_feed);
            ++me->feeds_num;
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc watched_init(
    struct watched *me, const struct am_event *event
) {
    (void)event;
    am_timer_ctor(
        &me->timer_wdt_feed,
        EVT_WATCHED_TIMEOUT,
        AM_PAL_TICK_DOMAIN_DEFAULT,
        /*owner=*/&me->ao
    );
    return AM_HSM_TRAN(watched_proc);
}

static void watched_ctor(struct watched *me) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(watched_init));
}

/* 'wdt' task */

static enum am_rc wdt_proc(struct wdt *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm_ms(
            &me->timer_wdt_bark, AM_WDT_BARK_TIMEOUT_MS, /*interval=*/0
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WDT_FEED: {
        am_pal_printff("EVT_WDT_FEED received\n");
        /* re-arm bark timer */
        am_timer_arm_ms(
            &me->timer_wdt_bark, AM_WDT_BARK_TIMEOUT_MS, /*interval=*/0
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WDT_BARK: {
        am_pal_printff("WATCHED TASK FAILED!\n");
        exit(-1);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc wdt_init(struct wdt *me, const struct am_event *event) {
    (void)event;
    am_timer_ctor(
        &me->timer_wdt_bark,
        EVT_WDT_BARK,
        AM_PAL_TICK_DOMAIN_DEFAULT,
        /*owner=*/&me->ao
    );
    return AM_HSM_TRAN(wdt_proc);
}

static void wdt_ctor(struct wdt *me) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(wdt_init));
}

/* timer task to drive timers  */

static void ticker_task(void *param) {
    (void)param;

    am_pal_task_wait_all();

    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    while (am_ao_get_cnt() > 0) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

int main(void) {
    am_ao_state_ctor(/*cfg=*/NULL);

    watched_ctor(&m_watched);
    wdt_ctor(&m_wdt);

    am_ao_start(
        &m_watched.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/m_queue_watched,
        /*nqueue=*/AM_COUNTOF(m_queue_watched),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"watched",
        /*init_event=*/NULL
    );

    am_ao_start(
        &m_wdt.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MIN, .task = AM_AO_PRIO_MIN},
        /*queue=*/m_queue_wdt,
        /*nqueue=*/AM_COUNTOF(m_queue_wdt),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"wdt",
        /*init_event=*/NULL
    );

    am_pal_task_create(
        "ticker",
        /*prio=*/AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/NULL
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    return EXIT_SUCCESS;
}
