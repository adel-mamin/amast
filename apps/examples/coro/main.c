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
 * Demonstrate integration of coroutine(s) with HSM.
 * HSM topology:
 *
 *  +-----------------------------------------------------+
 *  |                    am_hsm_top                       |
 *  | +--------------------------------+                  |
 *  | |            coro_top            |                  |
 *  | | +------------+  +------------+ | +--------------+ |
 *  | | |coro_regular|  |  coro_off  | | | coro_exiting | |
 *  | | +------------+  +------------+ | +--------------+ |
 *  | +--------------------------------+                  |
 *  +-----------------------------------------------------+
 *
 * It mimics the behavior of traffic lights.
 *
 * The coro_regular substate prints colored rectangles in the order
 * red - yellow - green - blinking green
 * The print delay of each rectangle varies.
 *
 * The coro_off substate shows blinking yellow mimicking unregulated
 * road intersection.
 * The blink delay is 700 ms.
 *
 * coro_exiting substate calls am_ao_stop().
 *
 * coro_top handles user input:
 * - press ENTER to switch between coro_regular and coro_off substates
 * - press ENTER two times in a row within 500ms to switch to coro_exiting
 *   and exit the app
 *
 * Generally the use of coroutine is warranted, if the sequence of
 * steps can be represented as a flowchart like in this case.
 *
 * coro_regular calls coro_regular_() to do the printing
 * coro_off calls coro_off_() to do the printing.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common/constants.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "timer/timer.h"
#include "coro/coro.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define CORO_TWO_NEWLINES_TIMEOUT_MS 500

enum {
    CORO_EVT_SWITCH_MODE = AM_EVT_USER,
    CORO_EVT_TIMER,
    CORO_EVT_EXIT,
    CORO_EVT_PUB_MAX,
    CORO_EVT_START,
};

struct coro {
    struct am_timer* timer;
    struct am_timer_event_x timeout;
    struct am_coro coro;
    struct am_coro coro_blinking_green;
    struct am_ao ao;
    struct am_hsm hsm;
    unsigned i;
};

static const struct am_event am_evt_start = {.id = CORO_EVT_START};

static enum am_rc coro_top(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc coro_regular(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc coro_off(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc coro_exiting(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc coro_top(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_INIT: {
        return am_hsm_tran(hsm, coro_regular);
    }
    case CORO_EVT_SWITCH_MODE: {
        am_printff("\b");
        if (am_hsm_is_in(hsm, am_hsm_state_make(coro_regular))) {
            return am_hsm_tran(hsm, coro_off);
        }
        return am_hsm_tran(hsm, coro_regular);
    }
    case CORO_EVT_EXIT: {
        return am_hsm_tran_redispatch(hsm, coro_exiting);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc coro_exiting(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct coro* me = AM_CONTAINER_OF(hsm, struct coro, hsm);
    switch (event->id) {
    case CORO_EVT_EXIT: {
        am_ao_stop(&me->ao);
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc coro_blinking_green(struct coro* me) {
    struct am_coro* coro = &me->coro_blinking_green;
    AM_CORO_BEGIN(coro);

    /* blinking green */
    for (me->i = 0; me->i < 4; ++me->i) {
        am_printff("\b" AM_COLOR_BLACK AM_SOLID_BLOCK "\b");
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/700, 0);
        AM_CORO_AWAIT(coro, !am_timer_is_armed(me->timer, &me->timeout.event));

        am_printff(AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/700, 0);
        AM_CORO_AWAIT(coro, !am_timer_is_armed(me->timer, &me->timeout.event));
    }

    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static enum am_rc coro_regular_(struct coro* me) {
    struct am_coro* coro = &me->coro_blinking_green;
    AM_CORO_BEGIN(coro);

    for (;;) {
        /* red */
        am_printff(AM_COLOR_RED AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/2000, 0);
        AM_CORO_AWAIT(coro, !am_timer_is_armed(me->timer, &me->timeout.event));

        /* yellow */
        am_printff("\b" AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/1000, 0);
        AM_CORO_AWAIT(coro, !am_timer_is_armed(me->timer, &me->timeout.event));

        /* green */
        am_printff("\b" AM_COLOR_GREEN AM_SOLID_BLOCK AM_COLOR_RESET);
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/2000, 0);
        AM_CORO_AWAIT(coro, !am_timer_is_armed(me->timer, &me->timeout.event));

        /* blinking green */
        AM_CORO_CALL(coro, coro_blinking_green(me));
        am_printff("\b");
    }

    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static enum am_rc coro_regular(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct coro* me = AM_CONTAINER_OF(hsm, struct coro, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_coro_init(&me->coro);
        am_coro_init(&me->coro_blinking_green);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return am_hsm_handled(hsm);
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer, &me->timeout.event);
        return am_hsm_handled(hsm);
    }
    case CORO_EVT_START:
    case CORO_EVT_TIMER: {
        return coro_regular_(me);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, coro_top);
}

static enum am_rc coro_off(struct am_hsm* hsm, const struct am_event* event) {
    struct coro* me = AM_CONTAINER_OF(hsm, struct coro, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_coro_init(&me->coro);
        am_ao_post_fifo(&me->ao, &am_evt_start);
        return am_hsm_handled(hsm);
    }
    case AM_EVT_EXIT: {
        am_timer_disarm(me->timer, &me->timeout.event);
        return am_hsm_handled(hsm);
    }
    case CORO_EVT_START:
    case CORO_EVT_TIMER: {
        struct am_coro* coro = &me->coro;
        AM_CORO_BEGIN(coro);

        for (;;) {
            am_printff("\b");
            am_printff(AM_COLOR_YELLOW AM_SOLID_BLOCK AM_COLOR_RESET);
            am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/1000, 0);
            AM_CORO_AWAIT(
                coro, !am_timer_is_armed(me->timer, &me->timeout.event)
            );

            am_printff("\b" AM_COLOR_BLACK AM_SOLID_BLOCK "\b");
            am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/700, 0);
            AM_CORO_AWAIT(
                coro, !am_timer_is_armed(me->timer, &me->timeout.event)
            );
        }

        AM_CORO_END();

        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, coro_top);
}

static enum am_rc coro_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;
    struct coro* me = AM_CONTAINER_OF(hsm, struct coro, hsm);
    am_ao_subscribe(&me->ao, CORO_EVT_SWITCH_MODE);
    am_ao_subscribe(&me->ao, CORO_EVT_EXIT);
    return am_hsm_tran(hsm, coro_top);
}

static void coro_init(struct coro* me, struct am_timer* timer) {
    memset(me, 0, sizeof(*me));

    am_ao_init(&me->ao, am_hsm_start_cb, am_hsm_dispatch_cb, &me->hsm);
    am_hsm_init(&me->hsm, am_hsm_state_make(coro_initial));

    me->timer = timer;
    me->timeout = am_timer_event_create_x(CORO_EVT_TIMER, &me->ao);
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

static void input_task(void* param) {
    (void)param;

    int ch = 0;
    uint32_t prev_ms = am_time_get_ms() - (2 * CORO_TWO_NEWLINES_TIMEOUT_MS);
    while ((ch = getc(stdin)) != EOF) {
        if ('\n' != ch) {
            continue;
        }
        am_printff(AM_CURSOR_UP);
        uint32_t now_ms = am_time_get_ms();
        uint32_t diff_ms = now_ms - prev_ms;
        prev_ms = now_ms;
        if (diff_ms > CORO_TWO_NEWLINES_TIMEOUT_MS) {
            static struct am_event event = {.id = CORO_EVT_SWITCH_MODE};
            am_ao_publish(&event);
            continue;
        }
        static struct am_event event = {.id = CORO_EVT_EXIT};
        am_ao_publish(&event);
        break;
    }
}

int main(void) {
    am_pal_global_init(/*arg=*/NULL);

    struct am_event_subscribe_list pubsub_list[CORO_EVT_PUB_MAX];
    am_ao_global_init(/*cfg=*/NULL, pubsub_list, AM_COUNTOF(pubsub_list));

    struct am_timer timer;
    am_timer_init(&timer);
    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    struct coro coro;
    coro_init(&coro, &timer);

    const struct am_event* event_queue[2];

    /* traffic lights controlling active object */
    am_ao_start(
        &coro.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/event_queue,
        /*queue_size=*/AM_COUNTOF(event_queue),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"coro",
        /*init_event=*/NULL
    );

    /* user input controlling thread */
    am_task_create(
        "input",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*init=*/NULL,
        /*entry=*/input_task,
        /*flags=*/AM_TASK_FLAG_WAIT_INIT,
        /*arg=*/&coro
    );

    int ticker = am_ticker_create(&(struct am_ticker_cfg){
        .timebase = AM_TIMEBASE_DEFAULT,
        .ticker_cb = ticker_cb,
        .ctx = &timer,
        .priority_hint = AM_AO_PRIO_MIN,
    });
    am_ticker_start(ticker);

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ticker_stop(ticker);

    am_ao_global_deinit();

    am_pal_global_deinit();

    return 0;
}
