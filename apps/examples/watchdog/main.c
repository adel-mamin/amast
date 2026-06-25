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

#include <stdlib.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
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
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x feed;
    int feeds_num;
    struct am_ao* wdt;
};

struct wdt {
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x bark;
};

/* task events */
static const struct am_event m_evt_wdt_feed = {.id = EVT_WDT_FEED};

/* 'watched' task */

static enum am_rc watched_proc(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct watched* me = AM_CONTAINER_OF(hsm, struct watched, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm(
            me->timer, &me->feed.event, AM_FEED_TIMEOUT_MS, AM_FEED_TIMEOUT_MS
        );
        return am_hsm_handled(hsm);
    }
    case EVT_WATCHED_TIMEOUT: {
        if (me->feeds_num < 3) {
            am_printff("EVT_WDT_FEED sent\n");
            am_ao_post_fifo(me->wdt, &m_evt_wdt_feed);
            ++me->feeds_num;
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc watched_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;
    return am_hsm_tran(hsm, watched_proc);
}

static void watched_init(
    struct watched* me, struct am_timer* timer, struct am_ao* wdt
) {
    memset(me, 0, sizeof(*me));
    am_ao_init(&me->ao, (am_ao_fn)am_hsm_start, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_init(&me->hsm, am_hsm_state_make(watched_initial));
    me->timer = timer;
    me->feed = am_timer_event_create_x(EVT_WATCHED_TIMEOUT, &me->ao);
    me->wdt = wdt;
}

/* 'wdt' task */

static enum am_rc wdt_proc(struct am_hsm* hsm, const struct am_event* event) {
    struct wdt* me = AM_CONTAINER_OF(hsm, struct wdt, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm(me->timer, &me->bark.event, AM_BARK_TIMEOUT_MS, 0);
        return am_hsm_handled(hsm);
    }
    case EVT_WDT_FEED: {
        am_printff("EVT_WDT_FEED received\n");
        /* re-arm bark timer */
        am_timer_arm(me->timer, &me->bark.event, AM_BARK_TIMEOUT_MS, 0);
        return am_hsm_handled(hsm);
    }
    case EVT_WDT_BARK: {
        am_printff("WATCHED TASK FAILED!\n");
        exit(EXIT_FAILURE);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc wdt_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;
    return am_hsm_tran(hsm, wdt_proc);
}

static void wdt_init(struct wdt* me, struct am_timer* timer) {
    memset(me, 0, sizeof(*me));
    am_ao_init(&me->ao, (am_ao_fn)am_hsm_start, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_init(&me->hsm, am_hsm_state_make(wdt_initial));
    me->timer = timer;
    me->bark = am_timer_event_create_x(EVT_WDT_BARK, &me->ao);
}

static void ticker_cb(void* param) {
    struct am_timer* timer = param;

    am_timer_tick_iterator_init(timer);
    struct am_timer_event* fired = NULL;
    while ((fired = am_timer_tick_iterator_next(timer)) != NULL) {
        void* owner = AM_CAST(struct am_timer_event_x*, fired)->ctx;
        if (owner) {
            am_ao_post_fifo(owner, &fired->event);
        } else {
            am_ao_publish(&fired->event);
        }
    }
}

int main(void) {
    am_pal_init(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_init(&timer);

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    am_ao_state_init(/*cfg=*/NULL);

    struct wdt wdt;
    wdt_init(&wdt, &timer);

    struct watched watched;
    watched_init(&watched, &timer, &wdt.ao);

    const struct am_event* queue_watched[1];
    am_ao_start(
        &watched.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue_watched,
        /*queue_size=*/AM_COUNTOF(queue_watched),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"watched",
        /*init_event=*/NULL
    );

    const struct am_event* queue_wdt[2];
    am_ao_start(
        &wdt.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MIN, .task = AM_AO_PRIO_MIN},
        /*queue=*/queue_wdt,
        /*queue_size=*/AM_COUNTOF(queue_wdt),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"wdt",
        /*init_event=*/NULL
    );

    int ticker = am_ticker_create(&(struct am_ticker_cfg){
        .timebase = AM_TIMEBASE_DEFAULT,
        .ticker_cb = ticker_cb,
        .ctx = &timer,
        .priority_hint = AM_AO_PRIO_MIN
    });
    am_ticker_start(ticker);

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ticker_stop(ticker);

    am_ao_state_deinit();

    am_pal_deinit();

    return EXIT_SUCCESS;
}
