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

struct worker {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    int id;
    struct am_event_alloc* alloc;
};

static const struct am_event m_evt_stop = {.id = EVT_STOP};
static const struct am_event m_evt_stopped = {.id = EVT_STOPPED};
static const struct am_event m_evt_start = {.id = EVT_START};

static void work(int cycles) {
    for (volatile int i = 0; i < cycles; ++i) {
        ;
    }
}

static enum am_rc worker_proc(struct worker* me, const struct am_event* event) {
    switch (event->id) {
    case EVT_JOB_REQ: {
        const struct job_req* req = AM_CAST(const struct job_req*, event);
        AM_ASSERT(req->work);
        req->work(req->cycles);
        struct job_done* done = (struct job_done*)am_event_allocate(
            me->alloc, EVT_JOB_DONE, sizeof(struct job_done)
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

static enum am_rc worker_init(struct worker* me, const struct am_event* event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_JOB_REQ);
    am_ao_subscribe(&me->ao, EVT_STOP);
    return AM_HSM_TRAN(worker_proc);
}

static void worker_ctor(
    struct worker* me, int id, struct am_event_alloc* alloc
) {
    memset(me, 0, sizeof(*me));
    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(worker_init));
    me->id = id;
    me->alloc = alloc;
}

struct balancer {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer* timer;
    struct am_timer_event_x timeout;
    int ncpus;
    int nstops;
    int stats[AM_WORKERS_NUM_MAX];
    struct worker* workers;
    int nworkers;

    struct am_event_alloc* alloc;
};

static void balancer_check_stats(const struct balancer* me) {
    int baseline = me->stats[0];
    AM_ASSERT(baseline > 0);
    for (int i = 1; i < me->ncpus; ++i) {
        int percent =
            AM_ABS((int)(100LL * (baseline - me->stats[i]) / baseline));
        AM_ASSERT(percent < 40);
    }
}

static enum am_rc balancer_stopping(
    struct balancer* me, const struct am_event* event
) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_ao_publish_exclude(&m_evt_stop, &me->ao);
        return AM_HSM_HANDLED();

    case EVT_STOPPED: {
        ++me->nstops;
        if (me->nstops == me->ncpus) {
            for (int i = 0; i < me->ncpus; ++i) {
                am_printf("worker: %d jobs done: %d\n", i, me->stats[i]);
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

static enum am_rc balancer_proc(
    struct balancer* me, const struct am_event* event
) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        am_timer_arm(me->timer, &me->timeout.event, AM_TIMEOUT_MS, 0);
        am_ao_post_fifo(&me->ao, &m_evt_start);
        return AM_HSM_HANDLED();
    }
    case EVT_START: {
        struct job_req* req = AM_CAST(
            struct job_req*,
            am_event_allocate(me->alloc, EVT_JOB_REQ, sizeof(struct job_req))
        );
        req->work = work;
        req->cycles = AM_WORKER_LOAD_CYCLES;
        am_ao_publish_exclude(&req->event, &me->ao);
        return AM_HSM_HANDLED();
    }
    case EVT_TIMEOUT:
        return AM_HSM_TRAN(balancer_stopping);

    case EVT_JOB_DONE: {
        const struct job_done* done = (const struct job_done*)event;
        AM_ASSERT(done->worker >= 0);
        AM_ASSERT(done->worker < me->nworkers);
        struct job_req* req = AM_CAST(
            struct job_req*,
            am_event_allocate(me->alloc, EVT_JOB_REQ, sizeof(struct job_req))
        );
        req->work = work;
        req->cycles = AM_WORKER_LOAD_CYCLES;
        am_ao_post_fifo(&me->workers[done->worker].ao, &req->event);
        ++me->stats[done->worker];
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc balancer_init(
    struct balancer* me, const struct am_event* event
) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_JOB_DONE);
    am_ao_subscribe(&me->ao, EVT_STOPPED);
    return AM_HSM_TRAN(balancer_proc);
}

static void balancer_ctor(
    struct balancer* me,
    int ncpus,
    struct am_timer* timer,
    struct worker* workers,
    int nworkers,
    struct am_event_alloc* alloc
) {
    memset(me, 0, sizeof(*me));
    me->ncpus = ncpus;
    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(balancer_init));

    me->workers = workers;
    me->nworkers = nworkers;

    me->timer = timer;
    me->timeout = am_timer_event_ctor_x(EVT_TIMEOUT, &me->ao);

    me->alloc = alloc;
}

static void ticker_task(void* param) {
    struct am_timer* timer = param;

    am_task_startup_gate_wait();

    while (am_ao_get_cnt() > 0) {
        am_sleep_ticks(AM_TICK_DOMAIN_DEFAULT, /*ticks=*/1);

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

AM_ALIGNOF_DEFINE(events_t);

int main(void) {
    am_pal_ctor(/*arg=*/NULL);

    struct am_timer timer;
    am_timer_ctor(&timer);

    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);

    union events event_pool[AM_WORKERS_NUM_MAX];
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
    am_ao_state_ctor(&cfg);

    int ncpus = am_get_cpu_count();
    am_printf("Number of CPUs: %d\n", ncpus);

    ncpus = AM_MIN(ncpus, AM_WORKERS_NUM_MAX);
    ncpus = AM_MIN(ncpus, AM_AO_NUM_MAX);

    struct balancer balancer;
    struct worker workers[AM_WORKERS_NUM_MAX];

    balancer_ctor(
        &balancer, ncpus, &timer, workers, AM_COUNTOF(workers), &alloc
    );
    for (int i = 0; i < ncpus; ++i) {
        worker_ctor(&workers[i], /*id=*/i, &alloc);
    }

    const struct am_event* queue_balancer[AM_WORKERS_NUM_MAX];
    am_ao_start(
        &balancer.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue_balancer,
        /*queue_size=*/AM_COUNTOF(queue_balancer),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"balancer",
        /*init_event=*/NULL
    );

    const struct am_event* queue_worker[AM_WORKERS_NUM_MAX][2];
    for (int i = 0; i < ncpus; ++i) {
        unsigned char prio = (unsigned char)(AM_AO_PRIO_MIN + i);
        am_ao_start(
            &workers[i].ao,
            (struct am_ao_prio){.ao = prio, .task = AM_AO_PRIO_LOW},
            /*queue=*/queue_worker[i],
            /*queue_size=*/AM_COUNTOF(queue_worker[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/"worker",
            /*init_event=*/NULL
        );
    }

    am_task_create(
        "ticker",
        /*prio=*/AM_AO_PRIO_MAX,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*flags=*/0,
        /*arg=*/&timer
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    am_pal_dtor();

    return EXIT_SUCCESS;
}
