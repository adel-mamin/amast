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
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x timeout;
    struct am_async async;
    struct am_async async_blinking_green;
    unsigned i;
};

static const struct am_event am_evt_start = {.id = ASYNC_EVT_START};

static enum am_rc async_top(struct async* me, const struct am_event* event);
static enum am_rc async_regular(struct async* me, const struct am_event* event);
static enum am_rc async_off(struct async* me, const struct am_event* event);
static enum am_rc async_exiting(struct async* me, const struct am_event* event);

static enum am_rc async_top(struct async* me, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_INIT: {
        return AM_HSM_TRAN(async_regular);
    }
    case ASYNC_EVT_SWITCH_MODE: {
        am_printff("\b");
        if (am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(async_regular))) {
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
    struct async* me, const struct am_event* event
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

static enum am_rc async_blinking_green(struct async* me) {
    AM_ASYNC_BEGIN(&me->async_blinking_green);

    /* blinking green */
    for (me->i = 0; me->i < 4; ++me->i) {
        am_printff("\b" AM_COLOR_BLACK AM_SOLID_BLOCK "\b");
        am_timer_arm(me->timer, &me->timeout.event, /*ms=*/700, 0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));

        am_printff(AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ms=*/700, 0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));
    }

    AM_ASYNC_END();

    return AM_RC_ASYNC_DONE;
}

static enum am_rc async_regular_(struct async* me) {
    AM_ASYNC_BEGIN(&me->async);

    for (;;) {
        /* red */
        am_printff(AM_COLOR_RED AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ms=*/2000, 0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));

        /* yellow */
        am_printff("\b" AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ms=*/1000, 0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));

        /* green */
        am_printff("\b" AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ms=*/2000, 0);
        AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));

        /* blinking green */
        AM_ASYNC_CALL(async_blinking_green(me));
        am_printff("\b");
    }

    AM_ASYNC_END();

    return AM_RC_ASYNC_DONE;
}

static enum am_rc async_regular(
    struct async* me, const struct am_event* event
) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_async_ctor(&me->async);
        am_async_ctor(&me->async_blinking_green);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer, &me->timeout.event);
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

static enum am_rc async_off(struct async* me, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_async_ctor(&me->async);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return AM_HSM_HANDLED();
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer, &me->timeout.event);
        return AM_HSM_HANDLED();
    }
    case ASYNC_EVT_START:
    case ASYNC_EVT_TIMER: {
        AM_ASYNC_BEGIN(&me->async);

        for (;;) {
            am_printff("\b");
            am_printff(AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
            am_timer_arm(me->timer, &me->timeout.event, /*ms=*/1000, 0);
            AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));

            am_printff("\b" AM_COLOR_BLACK AM_SOLID_BLOCK "\b");
            am_timer_arm(me->timer, &me->timeout.event, /*ms=*/700, 0);
            AM_ASYNC_AWAIT(!am_timer_is_armed(me->timer, &me->timeout.event));
        }

        AM_ASYNC_END();

        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(async_top);
}

static enum am_rc async_init(struct async* me, const struct am_event* event) {
    (void)event;
    am_ao_subscribe(&me->ao, ASYNC_EVT_SWITCH_MODE);
    am_ao_subscribe(&me->ao, ASYNC_EVT_EXIT);
    return AM_HSM_TRAN(async_top);
}

static void async_ctor(struct async* me, struct am_timer* timer) {
    memset(me, 0, sizeof(*me));

    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(async_init));

    me->timer = timer;
    me->timeout = am_timer_event_ctor_x(ASYNC_EVT_TIMER, &me->ao);
}

static void ticker_task(void* param) {
    struct am_timer* timer = param;

    am_task_wait_all();

    const int domain = AM_TICK_DOMAIN_DEFAULT;
    const uint32_t ticks_per_ms = am_time_get_tick_from_ms(domain, 1);
    uint32_t now_ticks = am_time_get_tick(domain);
    while (am_ao_get_cnt() > 0) {
        am_sleep_till_ticks(domain, now_ticks + ticks_per_ms);
        now_ticks += ticks_per_ms;

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
}

static void input_task(void* param) {
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
    am_pal_ctor(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_ctor(&timer);

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    am_ao_state_ctor(/*cfg=*/NULL);

    struct am_ao_subscribe_list pubsum_list[ASYNC_EVT_PUB_MAX];
    am_ao_init_subscribe_list(pubsum_list, AM_COUNTOF(pubsum_list));

    struct async m;
    async_ctor(&m, &timer);

    const struct am_event* queue[2];

    /* traffic lights controlling active object */
    am_ao_start(
        &m.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue,
        /*nqueue=*/AM_COUNTOF(queue),
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
        /*arg=*/&timer
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

    am_pal_dtor();

    return 0;
}
