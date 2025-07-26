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
 * Demonstrate workers implementation.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define AM_WORKERS_NUM_MAX 64
#define AM_WORKER_LOAD_CYCLES 50000
#define AM_TIMEOUT_MS (1 * 1000)

enum evt {
    EVT_JOB_DONE = AM_EVT_USER,
    EVT_JOB_REQ,
    EVT_STOP,
    EVT_STOPPED,
    EVT_PUB_MAX,

    /* non pub/sub events */
    EVT_TIMEOUT,
    EVT_START
};

struct job_req {
    struct am_event event;
    void (*work)(int cycles);
    int cycles;
};

struct job_done {
    struct am_event event;
    int worker;
};

typedef union events {
    struct job_req req;   /* cppcheck-suppress unusedStructMember */
    struct job_done done; /* cppcheck-suppress unusedStructMember */
} events_t;

static struct am_ao_subscribe_list m_pubsub_list[EVT_PUB_MAX];
static union events m_event_pool[AM_WORKERS_NUM_MAX];

static const struct am_event *m_queue_balancer[AM_WORKERS_NUM_MAX];

static const struct am_event *m_queue_worker[AM_WORKERS_NUM_MAX][2];

struct worker {
    struct am_ao ao;
    int id;
};

static struct worker m_workers[AM_WORKERS_NUM_MAX];

static const struct am_event m_evt_stop = {.id = EVT_STOP};
static const struct am_event m_evt_stopped = {.id = EVT_STOPPED};
static const struct am_event m_evt_start = {.id = EVT_START};

static void work(int cycles) {
    for (volatile int i = 0; i < cycles; ++i) {
        ;
    }
}

static int worker_proc(struct worker *me, const struct am_event *event) {
    switch (event->id) {
    case EVT_JOB_REQ: {
        const struct job_req *req = AM_CAST(const struct job_req *, event);
        AM_ASSERT(req->work);
        req->work(req->cycles);
        struct job_done *done = (struct job_done *)am_event_allocate(
            EVT_JOB_DONE, sizeof(struct job_done)
        );
        done->worker = me->id;
        am_ao_publish(&done->event);
        return AM_HSM_HANDLED();
    }
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

static int worker_init(struct worker *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_JOB_REQ);
    am_ao_subscribe(&me->ao, EVT_STOP);
    return AM_HSM_TRAN(worker_proc);
}

static void worker_ctor(struct worker *me, int id) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(worker_init));
    me->id = id;
}

struct balancer {
    struct am_ao ao;
    struct am_timer timeout;
    int nworkers;
    int nstops;
    int stats[AM_WORKERS_NUM_MAX];
};

static struct balancer m_balancer;

static void balancer_check_stats(const struct balancer *me) {
    int baseline = me->stats[0];
    AM_ASSERT(baseline > 0);
    for (int i = 1; i < me->nworkers; ++i) {
        int percent =
            AM_ABS((int)(100LL * (baseline - me->stats[i]) / baseline));
        AM_ASSERT(percent < 40);
    }
}

static int balancer_stopping(
    struct balancer *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_ao_publish_exclude(&m_evt_stop, &me->ao);
        return AM_HSM_HANDLED();

    case EVT_STOPPED: {
        ++me->nstops;
        if (me->nstops == me->nworkers) {
            for (int i = 0; i < me->nworkers; ++i) {
                am_pal_printf("worker: %d jobs done: %d\n", i, me->stats[i]);
            }
            balancer_check_stats(me);
            am_ao_stop(&me->ao);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int balancer_proc(struct balancer *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: {
        am_timer_arm_ms(&me->timeout, AM_TIMEOUT_MS, /*interval=*/0);
        am_ao_post_fifo(&me->ao, &m_evt_start);
        return AM_HSM_HANDLED();
    }
    case EVT_START: {
        struct job_req *req = AM_CAST(
            struct job_req *,
            am_event_allocate(EVT_JOB_REQ, sizeof(struct job_req))
        );
        req->work = work;
        req->cycles = AM_WORKER_LOAD_CYCLES;
        am_ao_publish_exclude(&req->event, &me->ao);
        return AM_HSM_HANDLED();
    }
    case EVT_TIMEOUT:
        return AM_HSM_TRAN(balancer_stopping);

    case EVT_JOB_DONE: {
        const struct job_done *done = (const struct job_done *)event;
        AM_ASSERT(done->worker >= 0);
        AM_ASSERT(done->worker < AM_COUNTOF(m_workers));
        struct job_req *req = AM_CAST(
            struct job_req *,
            am_event_allocate(EVT_JOB_REQ, sizeof(struct job_req))
        );
        req->work = work;
        req->cycles = AM_WORKER_LOAD_CYCLES;
        am_ao_post_fifo(&m_workers[done->worker].ao, &req->event);
        ++me->stats[done->worker];
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int balancer_init(struct balancer *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_JOB_DONE);
    am_ao_subscribe(&me->ao, EVT_STOPPED);
    am_timer_ctor(&me->timeout, EVT_TIMEOUT, AM_PAL_TICK_DOMAIN_DEFAULT, me);
    return AM_HSM_TRAN(balancer_proc);
}

static void balancer_ctor(int nworkers) {
    struct balancer *me = &m_balancer;
    memset(me, 0, sizeof(*me));
    me->nworkers = nworkers;
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(balancer_init));
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
    struct am_ao_state_cfg cfg = {
        .on_idle = am_pal_on_idle,
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg);

    am_event_add_pool(
        m_event_pool,
        sizeof(m_event_pool),
        sizeof(m_event_pool[0]),
        AM_ALIGNOF(events_t)
    );

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    int ncpus = am_pal_get_cpu_count();
    am_pal_printf("Number of CPUs: %d\n", ncpus);

    ncpus = AM_MIN(ncpus, AM_WORKERS_NUM_MAX);
    ncpus = AM_MIN(ncpus, AM_AO_NUM_MAX);

    balancer_ctor(/*nworkers=*/ncpus);
    for (int i = 0; i < ncpus; ++i) {
        worker_ctor(&m_workers[i], /*id=*/i);
    }

    am_ao_start(
        &m_balancer.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/m_queue_balancer,
        /*nqueue=*/AM_COUNTOF(m_queue_balancer),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"balancer",
        /*init_event=*/NULL
    );

    for (int i = 0; i < ncpus; ++i) {
        unsigned char prio = (unsigned char)(AM_AO_PRIO_MIN + i);
        am_ao_start(
            &m_workers[i].ao,
            (struct am_ao_prio){.ao = prio, .task = AM_AO_PRIO_LOW},
            /*queue=*/m_queue_worker[i],
            /*nqueue=*/AM_COUNTOF(m_queue_worker[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/"worker",
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
