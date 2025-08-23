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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "event/event.h"
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

static union events m_event_pool[10];

static struct am_ao_subscribe_list m_pubsub_list[EVT_PUB_MAX];

static const struct am_event *m_queue_agent[2 * AM_SMOKERS_NUM_MAX];

static const struct am_event *m_queue_smoker[AM_SMOKERS_NUM_MAX][5];

struct smoker {
    struct am_ao ao;
    struct am_timer timer_done_smoking;
    int id;
    unsigned resource_own;
    unsigned resource_acquired;
    unsigned resource_id;
};

static struct smoker m_smokers[AM_SMOKERS_NUM_MAX];

static int rand_012(void) {
    static unsigned long next = 1;

    next = next * 1103515245 + 12345;
    return (int)((unsigned)(next / 65536) % 3);
}

static int smoker_top(struct smoker *me, const struct am_event *event);
static int smoker_idle(struct smoker *me, const struct am_event *event);
static int smoker_smoking(struct smoker *me, const struct am_event *event);
static int smoker_stopping(struct smoker *me, const struct am_event *event);

static int smoker_stopping(struct smoker *me, const struct am_event *event) {
    switch (event->id) {
    case EVT_STOP: {
        am_ao_publish(&m_evt_stopped);
        am_ao_stop(&me->ao);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int smoker_top(struct smoker *me, const struct am_event *event) {
    switch (event->id) {
    case EVT_STOP: {
        return AM_HSM_TRAN_REDISPATCH(smoker_stopping);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int smoker_idle(struct smoker *me, const struct am_event *event) {
    switch (event->id) {
    case EVT_RESOURCE: {
        const struct resource *e = (const struct resource *)event;
        if (e->resource_id != me->resource_id) {
            me->resource_acquired = me->resource_own;
            me->resource_id = e->resource_id;
        }
        me->resource_acquired |= e->resource;
        if (me->resource_acquired == (PAPER | TOBACCO | FIRE)) {
            return AM_HSM_TRAN(smoker_smoking);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(smoker_top);
}

static int smoker_smoking(struct smoker *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_timer_arm_ms(&me->timer_done_smoking, /*ms=*/20, /*interval=*/0);
        return AM_HSM_HANDLED();

    case AM_EVT_EXIT:
        am_timer_disarm(&me->timer_done_smoking);
        return AM_HSM_HANDLED();

    case EVT_RESOURCE:
        AM_ASSERT(0);
        return AM_HSM_HANDLED();

    case EVT_DONE_SMOKING_TIMER: {
        struct done_smoking *e = (struct done_smoking *)am_event_allocate(
            EVT_DONE_SMOKING, sizeof(*e)
        );
        e->smoker_id = me->id;
        am_ao_publish(&e->event);
        return AM_HSM_TRAN(smoker_idle);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(smoker_top);
}

static int smoker_init(struct smoker *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_RESOURCE);
    am_ao_subscribe(&me->ao, EVT_STOP);
    am_timer_ctor(
        &me->timer_done_smoking,
        EVT_DONE_SMOKING_TIMER,
        AM_PAL_TICK_DOMAIN_DEFAULT,
        /*owner=*/&me->ao
    );
    return AM_HSM_TRAN(smoker_idle);
}

static void smoker_ctor(struct smoker *me, int id, unsigned resource) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(smoker_init));
    me->id = id;
    me->resource_own = me->resource_acquired = resource;
}

struct agent {
    struct am_ao ao;
    struct am_timer timeout;
    int stats[AM_SMOKERS_NUM_MAX];
    int nstops;
    unsigned resource_id;
};

static struct agent m_agent;

static void agent_check_stats(const struct agent *me) {
    int baseline = me->stats[0];
    AM_ASSERT(baseline > 0);
    for (int i = 1; i < AM_COUNTOF(me->stats); ++i) {
        int percent =
            AM_ABS((int)(100LL * (baseline - me->stats[i]) / baseline));
        AM_ASSERT(percent < 40);
    }
}

static int agent_stopping(struct agent *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_ao_publish_exclude(&m_evt_stop, &me->ao);
        return AM_HSM_HANDLED();

    case EVT_STOPPED: {
        ++me->nstops;
        if (me->nstops == AM_SMOKERS_NUM_MAX) {
            for (int i = 0; i < AM_SMOKERS_NUM_MAX; ++i) {
                am_pal_printf("smoker: %d smokes done: %d\n", i, me->stats[i]);
            }
            agent_check_stats(me);
            am_ao_stop(&me->ao);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static void publish_resource(const struct agent *me, unsigned resource) {
    struct resource *e =
        (struct resource *)am_event_allocate(EVT_RESOURCE, sizeof(*e));
    e->resource = resource;
    e->resource_id = me->resource_id;
    am_ao_publish(&e->event);
}

static void publish_resources(struct agent *me) {
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

static int agent_proc(struct agent *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm_ms(&me->timeout, AM_TIMEOUT_MS, /*interval=*/0);
        am_ao_post_fifo(&me->ao, &m_evt_start);
        return AM_HSM_HANDLED();
    }
    case EVT_DONE_SMOKING: {
        const struct done_smoking *done = (const struct done_smoking *)event;
        AM_ASSERT(done->smoker_id >= 0);
        AM_ASSERT(done->smoker_id < AM_COUNTOF(me->stats));
        ++me->stats[done->smoker_id];
        publish_resources(me);
        return AM_HSM_HANDLED();
    }
    case EVT_START: {
        publish_resources(me);
        return AM_HSM_HANDLED();
    }
    case EVT_TIMEOUT:
        return AM_HSM_TRAN(agent_stopping);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int agent_init(struct agent *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_DONE_SMOKING);
    am_ao_subscribe(&me->ao, EVT_STOPPED);
    am_timer_ctor(&me->timeout, EVT_TIMEOUT, AM_PAL_TICK_DOMAIN_DEFAULT, me);
    return AM_HSM_TRAN(agent_proc);
}

static void agent_ctor(void) {
    struct agent *me = &m_agent;
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(agent_init));
}

static void ticker_task(void *param) {
    (void)param;

    am_pal_wait_all_tasks();

    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    while (am_ao_get_cnt() > 0) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

AM_ALIGNOF_DEFINE(events_t);

int main(void) {
    am_ao_state_ctor(/*cfg=*/NULL);

    am_event_pool_add(
        m_event_pool,
        sizeof(m_event_pool),
        sizeof(m_event_pool[0]),
        AM_ALIGNOF(events_t)
    );

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    agent_ctor();
    static const unsigned resource[] = {PAPER, TOBACCO, FIRE};
    for (int i = 0; i < AM_COUNTOF(resource); ++i) {
        smoker_ctor(&m_smokers[i], i, resource[i]);
    }

    am_ao_start(
        &m_agent.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/m_queue_agent,
        /*nqueue=*/AM_COUNTOF(m_queue_agent),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"agent",
        /*init_event=*/NULL
    );

    for (int i = 0; i < AM_COUNTOF(m_smokers); ++i) {
        unsigned char prio = (unsigned char)(AM_AO_PRIO_MIN + i);
        am_ao_start(
            &m_smokers[i].ao,
            (struct am_ao_prio){.ao = (unsigned char)(prio + i),
                                .task = AM_AO_PRIO_LOW},
            /*queue=*/m_queue_smoker[i],
            /*nqueue=*/AM_COUNTOF(m_queue_smoker[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/"smoker",
            /*init_event=*/NULL
        );
    }

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
