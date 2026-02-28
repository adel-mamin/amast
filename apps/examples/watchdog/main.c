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
 * The 'watched' task works fine for 3 seconds, after which
 * it fails to feed 'wdt' in time.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "timer/timer.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define AM_FEED_TIMEOUT_MS 1000
#define AM_BARK_TIMEOUT_MS (AM_FEED_TIMEOUT_MS + 100)

enum evt {
    EVT_WATCHED_TIMEOUT = AM_EVT_USER,
    EVT_WDT_FEED,
    EVT_WDT_BARK,
};

struct watched {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer *timer;
    int tix_feed;
    int feeds_num;
};

struct wdt {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer *timer;
    int tix_bark;
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
        am_timer_arm(
            me->timer, me->tix_feed, AM_FEED_TIMEOUT_MS, AM_FEED_TIMEOUT_MS
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WATCHED_TIMEOUT: {
        if (me->feeds_num < 3) {
            am_printff("EVT_WDT_FEED sent\n");
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
    return AM_HSM_TRAN(watched_proc);
}

static void watched_ctor(struct watched *me, struct am_timer *timer) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(watched_init));
    me->timer = timer;
    me->tix_feed = am_timer_allocate_x(
        timer,
        EVT_WATCHED_TIMEOUT,
        /*owner=*/&me->ao
    );
}

/* 'wdt' task */

static enum am_rc wdt_proc(struct wdt *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm(
            me->timer, me->tix_bark, AM_BARK_TIMEOUT_MS, /*interval=*/0
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WDT_FEED: {
        am_printff("EVT_WDT_FEED received\n");
        /* re-arm bark timer */
        am_timer_arm(
            me->timer, me->tix_bark, AM_BARK_TIMEOUT_MS, /*interval=*/0
        );
        return AM_HSM_HANDLED();
    }
    case EVT_WDT_BARK: {
        am_printff("WATCHED TASK FAILED!\n");
        exit(EXIT_FAILURE);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc wdt_init(struct wdt *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(wdt_proc);
}

static void wdt_ctor(struct wdt *me, struct am_timer *timer) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(wdt_init));
    me->timer = timer;
    me->tix_bark = am_timer_allocate_x(me->timer, EVT_WDT_BARK, &me->ao);
}

/* timer task to drive timers  */

static void ticker_task(void *param) {
    struct am_timer *timer = param;

    am_task_wait_all();

    const int domain = AM_TICK_DOMAIN_DEFAULT;
    const uint32_t ticks_per_ms = am_time_get_tick_from_ms(domain, 1);
    uint32_t now_ticks = am_time_get_tick(domain);
    while (am_ao_get_cnt() > 0) {
        am_sleep_till_ticks(domain, now_ticks + ticks_per_ms);
        now_ticks += 1;
        uint32_t fired = am_timer_tick(timer);
        while (fired) {
            int tix = AM_CTZL(fired);
            struct am_timer_event *event = am_timer_from_tix(timer, tix);
            fired &= (uint32_t)~(1UL << (unsigned)tix);
            void *owner = AM_CAST(struct am_timer_event_x *, event)->ctx;
            if (owner) {
                am_ao_post_fifo(owner, &event->base);
            } else {
                am_ao_publish(&event->base);
            }
        }
    }
}

int main(void) {
    struct am_timer timer;
    struct am_timer_event_x timer_events[4];

    am_timer_ctor(
        &timer,
        timer_events,
        AM_COUNTOF(timer_events),
        sizeof(struct am_timer_event_x)
    );

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    am_ao_state_ctor(/*cfg=*/NULL);

    watched_ctor(&m_watched, &timer);
    wdt_ctor(&m_wdt, &timer);

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

    am_task_create(
        "ticker",
        /*prio=*/AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/&timer
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    return EXIT_SUCCESS;
}
