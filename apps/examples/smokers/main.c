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
 * Cigarette smokers problem solution.
 */

#include <stdlib.h>
#include <string.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event_async.h"
#include "event/event_common.h"
#include "event/event_pool.h"
#include "timer/timer.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define AM_TIMEOUT_MS (1 * 1000)

#define PAPER (1U << 0U)
#define TOBACCO (1U << 1U)
#define FIRE (1U << 2U)

#define AM_SMOKERS_NUM_MAX 3

enum evt {
    EVT_RESOURCE = AM_EVT_USER,
    EVT_DONE_SMOKING,
    EVT_DONE_SMOKING_TIMER,
    EVT_STOP,
    EVT_STOPPED,
    EVT_PUB_MAX,

    /* non pub/sub events */
    EVT_TIMEOUT,
    EVT_START
};

struct resource {
    struct am_event event;
    unsigned resource;
    unsigned resource_id;
};

struct done_smoking {
    struct am_event event;
    int smoker_id;
};

typedef union events {
    struct resource resource;         /* cppcheck-suppress unusedStructMember */
    struct done_smoking done_smoking; /* cppcheck-suppress unusedStructMember */
} events_t;

static const struct am_event m_evt_start = {.id = EVT_START};
static const struct am_event m_evt_stop = {.id = EVT_STOP};
static const struct am_event m_evt_stopped = {.id = EVT_STOPPED};

struct smoker {
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x done;
    int id;
    unsigned resource_own;
    unsigned resource_acquired;
    unsigned resource_id;

    struct am_event_alloc* alloc;
};

static int rand_012(void) {
    static unsigned long next = 1;

    next = (next * 1103515245) + 12345;
    return (int)((unsigned)(next / 65536) % 3);
}

static enum am_rc smoker_top(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc smoker_idle(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc smoker_smoking(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc smoker_stopping(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc smoker_stopping(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct smoker* me = AM_CONTAINER_OF(hsm, struct smoker, hsm);
    switch (event->id) {
    case EVT_STOP: {
        am_ao_publish(&m_evt_stopped);
        am_ao_stop(&me->ao);
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc smoker_top(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case EVT_STOP: {
        return am_hsm_tran_redispatch(hsm, smoker_stopping);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc smoker_idle(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct smoker* me = AM_CONTAINER_OF(hsm, struct smoker, hsm);
    switch (event->id) {
    case EVT_RESOURCE: {
        const struct resource* e = (const struct resource*)event;
        if (e->resource_id != me->resource_id) {
            me->resource_acquired = me->resource_own;
            me->resource_id = e->resource_id;
        }
        me->resource_acquired |= e->resource;
        if (me->resource_acquired == (PAPER | TOBACCO | FIRE)) {
            return am_hsm_tran(hsm, smoker_smoking);
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, smoker_top);
}

static enum am_rc smoker_smoking(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct smoker* me = AM_CONTAINER_OF(hsm, struct smoker, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_timer_arm(me->timer, &me->done.event, /*ticks=*/20, /*interval=*/0);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        am_timer_disarm(me->timer, &me->done.event);
        return am_hsm_handled(hsm);

    case EVT_RESOURCE:
        AM_ASSERT(0);
        return am_hsm_handled(hsm);

    case EVT_DONE_SMOKING_TIMER: {
        struct done_smoking* e = (struct done_smoking*)am_event_allocate(
            me->alloc, EVT_DONE_SMOKING, sizeof(*e)
        );
        e->smoker_id = me->id;
        am_ao_publish(&e->event);
        return am_hsm_tran(hsm, smoker_idle);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, smoker_top);
}

static enum am_rc smoker_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;
    struct smoker* me = AM_CONTAINER_OF(hsm, struct smoker, hsm);
    am_ao_subscribe(&me->ao, EVT_RESOURCE);
    am_ao_subscribe(&me->ao, EVT_STOP);
    return am_hsm_tran(hsm, smoker_idle);
}

static void smoker_init(
    struct smoker* me,
    int id,
    unsigned resource,
    struct am_timer* timer,
    struct am_event_alloc* alloc
) {
    memset(me, 0, sizeof(*me));
    am_hsm_init(&me->hsm, am_hsm_state_make(smoker_initial));
    am_ao_init(&me->ao, (am_ao_fn)am_hsm_start, (am_ao_fn)am_hsm_dispatch, me);
    me->id = id;
    me->resource_own = me->resource_acquired = resource;

    me->timer = timer;
    me->done = am_timer_event_create_x(EVT_DONE_SMOKING_TIMER, &me->ao);

    me->alloc = alloc;
}

struct agent {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x timeout;
    int stats[AM_SMOKERS_NUM_MAX];
    int nstops;
    unsigned resource_id;

    struct am_event_alloc* alloc;
};

static void agent_check_stats(const struct agent* me) {
    int baseline = me->stats[0];
    AM_ASSERT(baseline > 0);
    for (int i = 1; i < AM_COUNTOF(me->stats); ++i) {
        int percent =
            AM_ABS((int)(100LL * (baseline - me->stats[i]) / baseline));
        AM_ASSERT(percent < 40);
    }
}

static enum am_rc agent_stopping(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct agent* me = AM_CONTAINER_OF(hsm, struct agent, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_ao_publish_exclude(&m_evt_stop, &me->ao);
        return am_hsm_handled(hsm);

    case EVT_STOPPED: {
        ++me->nstops;
        if (me->nstops == AM_SMOKERS_NUM_MAX) {
            for (int i = 0; i < AM_SMOKERS_NUM_MAX; ++i) {
                am_printf("smoker: %d smokes done: %d\n", i, me->stats[i]);
            }
            agent_check_stats(me);
            am_ao_stop(&me->ao);
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static void publish_resource(const struct agent* me, unsigned resource) {
    struct resource* e = (struct resource*)am_event_allocate(
        me->alloc, EVT_RESOURCE, sizeof(*e)
    );
    e->resource = resource;
    e->resource_id = me->resource_id;
    am_ao_publish(&e->event);
}

static void publish_resources(struct agent* me) {
    int r = rand_012();
    switch (r) {
    case 0:
        publish_resource(me, PAPER);
        publish_resource(me, TOBACCO);
        break;
    case 1:
        publish_resource(me, PAPER);
        publish_resource(me, FIRE);
        break;
    case 2:
        publish_resource(me, TOBACCO);
        publish_resource(me, FIRE);
        break;
    default:
        AM_ASSERT(0);
        break;
    }
    me->resource_id++;
}

static enum am_rc agent_proc(struct am_hsm* hsm, const struct am_event* event) {
    struct agent* me = AM_CONTAINER_OF(hsm, struct agent, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm(me->timer, &me->timeout.event, AM_TIMEOUT_MS, 0);
        am_ao_post_fifo(&me->ao, &m_evt_start);
        return am_hsm_handled(hsm);
    }
    case EVT_DONE_SMOKING: {
        const struct done_smoking* done = (const struct done_smoking*)event;
        AM_ASSERT(done->smoker_id >= 0);
        AM_ASSERT(done->smoker_id < AM_COUNTOF(me->stats));
        ++me->stats[done->smoker_id];
        publish_resources(me);
        return am_hsm_handled(hsm);
    }
    case EVT_START: {
        publish_resources(me);
        return am_hsm_handled(hsm);
    }
    case EVT_TIMEOUT:
        return am_hsm_tran(hsm, agent_stopping);

    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc agent_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;

    struct agent* me = AM_CONTAINER_OF(hsm, struct agent, hsm);
    am_ao_subscribe(&me->ao, EVT_DONE_SMOKING);
    am_ao_subscribe(&me->ao, EVT_STOPPED);
    return am_hsm_tran(hsm, agent_proc);
}

static void agent_init(
    struct agent* me, struct am_timer* timer, struct am_event_alloc* alloc
) {
    memset(me, 0, sizeof(*me));
    am_hsm_init(&me->hsm, am_hsm_state_make(agent_initial));
    am_ao_init(&me->ao, (am_ao_fn)am_hsm_start, (am_ao_fn)am_hsm_dispatch, me);

    me->timer = timer;
    me->timeout = am_timer_event_create_x(EVT_TIMEOUT, &me->ao);

    me->alloc = alloc;
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

AM_ALIGNOF_DEFINE(events_t);

int main(void) {
    am_pal_init(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_init(&timer);

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);

    union events event_pool[10];
    am_event_alloc_add_pool(
        &alloc,
        event_pool,
        sizeof(event_pool),
        sizeof(event_pool[0]),
        AM_ALIGNOF(events_t)
    );

    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX];
    am_event_async_init(pubsub_list, AM_COUNTOF(pubsub_list), &alloc);

    struct am_ao_state_cfg cfg = {
        .crit_enter = am_crit_enter, .crit_exit = am_crit_exit, .alloc = &alloc
    };
    am_ao_state_init(&cfg);

    struct smoker smokers[AM_SMOKERS_NUM_MAX];
    struct agent agent;

    agent_init(&agent, &timer, &alloc);
    static const unsigned resource[] = {PAPER, TOBACCO, FIRE};
    for (int i = 0; i < AM_COUNTOF(resource); ++i) {
        smoker_init(&smokers[i], i, resource[i], &timer, &alloc);
    }

    const struct am_event* queue_agent[2 * AM_SMOKERS_NUM_MAX];

    am_ao_start(
        &agent.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue_agent,
        /*queue_size=*/AM_COUNTOF(queue_agent),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"agent",
        /*init_event=*/NULL
    );

    const struct am_event* queue_smoker[AM_SMOKERS_NUM_MAX][5];

    for (int i = 0; i < AM_COUNTOF(smokers); ++i) {
        unsigned char prio = (unsigned char)(AM_AO_PRIO_MIN + i);
        am_ao_start(
            &smokers[i].ao,
            (struct am_ao_prio){
                .ao = (unsigned char)(prio + i), .task = AM_AO_PRIO_LOW
            },
            /*queue=*/queue_smoker[i],
            /*queue_size=*/AM_COUNTOF(queue_smoker[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/"smoker",
            /*init_event=*/NULL
        );
    }

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
