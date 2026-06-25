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

/* Platform abstraction layer (PAL) API posix implementation */

#include <stdbool.h>

/* amast-pragma: verbatim-include-std-on */

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <bits/local_lim.h>
#include <sched.h>
#include <features.h>
#include <unistd.h>
#include <errno.h>

/* amast-pragma: verbatim-include-std-off */

#include "common/compiler.h"
#include "common/macros.h"
#include "pal/pal.h"

/* to silence unused macro warnings */
AM_ASSERT_STATIC(_POSIX_C_SOURCE);

/** PAL task descriptor */
struct am_task {
    /** pthread data */
    pthread_t thread;
    /** mutex to protect condition variable */
    pthread_mutex_t mutex;
    /** condition variable for signaling */
    pthread_cond_t cond;
    /** flag to track notification state */
    bool notified;
    /** the task is created */
    bool created;
    /** the task is running */
    bool running;
    /** the task is joinable */
    bool joinable;
    /** the task waits for the startup gate to open */
    bool wait_init;
    /** the task waits for the startup gate to open */
    bool init_complete;
    /** init function */
    void (*init)(void* arg);
    /** entry function */
    void (*entry)(void* arg);
    /** entry function argument */
    void* arg;
    /** flags */
    unsigned flags;
    /** task name */
    const char* name;
};

static struct am_task task_main_ = {0};
static struct am_task am_tasks_[AM_TASK_NUM_MAX] = {0};
static int init_complete_mutex_;
static int init_complete_mutex_acquired_;

/** PAL mutex descriptor */
struct am_mutex {
    /** pthread mutex */
    pthread_mutex_t mutex;
    /** the mutex is valid */
    bool valid;
};

/** Maximum number of mutexes */
#ifndef AM_PAL_MUTEX_NUM_MAX
#define AM_PAL_MUTEX_NUM_MAX 2
#endif

static struct am_mutex am_mutexes_[AM_PAL_MUTEX_NUM_MAX] = {0};

static int am_pal_index_from_id(int id) {
    AM_ASSERT(id > 0);
    return id - 1;
}

static int am_pal_id_from_index(int index) {
    AM_ASSERT(index >= 0);
    return index + 1;
}

static bool am_task_id_is_valid(int task_id) {
    if (AM_TASK_ID_MAIN == task_id) {
        return true;
    }
    if ((AM_TASK_ID_NONE == task_id) || (task_id < 0)) {
        return false;
    }
    return task_id <= AM_COUNTOF(am_tasks_);
}

static struct am_task* am_task_get_hnd(int task_id) {
    AM_ASSERT(am_task_id_is_valid(task_id));
    if (AM_TASK_ID_MAIN == task_id) {
        return &task_main_;
    }
    return &am_tasks_[am_pal_index_from_id(task_id)];
}

static void* thread_entry_wrapper(void* arg) {
    AM_ASSERT(arg);
    struct am_task* task = (struct am_task*)arg;
    AM_ASSERT(task->entry);

    AM_ATOMIC_STORE_N(&task->running, true);

    if (task->init) {
        task->init(task->arg);
    }
    if (task->flags & AM_TASK_FLAG_WAIT_INIT) {
        am_task_init_wait();
    }

    task->entry(task->arg);

    int rc = pthread_mutex_destroy(&task->mutex);
    AM_ASSERT(0 == rc);

    if (!AM_ATOMIC_LOAD_N(&task->joinable)) {
        rc = pthread_cond_destroy(&task->cond);
        AM_ASSERT(0 == rc);
    }
    AM_ATOMIC_STORE_N(&task->created, false);

    return NULL;
}

static pthread_mutex_t am_crit_mutex_ = PTHREAD_MUTEX_INITIALIZER;
static bool am_crit_entered_ = false;

void am_crit_enter(void) {
    int rc = pthread_mutex_lock(&am_crit_mutex_);
    AM_ASSERT(!am_crit_entered_);
    am_crit_entered_ = true;
    AM_ASSERT(0 == rc);
}

void am_crit_exit(void) {
    AM_ASSERT(am_crit_entered_);
    am_crit_entered_ = false;
    int rc = pthread_mutex_unlock(&am_crit_mutex_);
    AM_ASSERT(0 == rc);
}

int am_task_get_own_id(void) {
    pthread_t thread = pthread_self();
    if (task_main_.thread == thread) {
        return AM_TASK_ID_MAIN;
    }
    for (int i = 0; i < AM_COUNTOF(am_tasks_); ++i) {
        struct am_task* me = &am_tasks_[i];
        if (AM_ATOMIC_LOAD_N(&me->running) && (me->thread == thread)) {
            return am_pal_id_from_index(i);
        }
    }
    AM_ASSERT(0);
    return AM_TASK_ID_NONE;
}

static void am_mutex_init(pthread_mutex_t* me);

int am_task_create(
    const char* name,
    int prio,
    void* stack,
    const int stack_size,
    void (*init)(void* arg),
    void (*entry)(void* arg),
    unsigned flags,
    void* arg
) {
    (void)stack;
    AM_ASSERT(entry);
    AM_ASSERT(prio >= 0);
    AM_ASSERT(prio < AM_TASK_NUM_MAX);

    int index = -1;
    struct am_task* task = NULL;
    for (int i = 0; i < AM_COUNTOF(am_tasks_); ++i) {
        task = &am_tasks_[i];
        bool was_created = AM_ATOMIC_EXCHANGE_N(&task->created, true);
        if (!was_created) {
            index = i;
            break;
        }
    }
    AM_ASSERT(index >= 0);

    am_mutex_init(&task->mutex);

    int ret = pthread_cond_init(&task->cond, /*attr=*/NULL);
    AM_ASSERT(0 == ret);

    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    AM_ASSERT(0 == ret);

    ret = pthread_attr_setstacksize(
        &attr, (size_t)AM_MAX(stack_size, PTHREAD_STACK_MIN)
    );
    AM_ASSERT(0 == ret);

    int policy = SCHED_OTHER;
    ret = pthread_attr_setschedpolicy(&attr, policy);
    AM_ASSERT(0 == ret);

    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);

    int prio_scaled = prio * (max_prio - min_prio) / (AM_TASK_NUM_MAX - 1);
    prio = min_prio + prio_scaled;

    struct sched_param param = {.sched_priority = prio};
    ret = pthread_attr_setschedparam(&attr, &param);
    AM_ASSERT(0 == ret);
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    AM_ASSERT(0 == ret);

    task->init = init;
    task->entry = entry;
    task->arg = arg;
    task->flags = flags;

    ret = pthread_create(&task->thread, &attr, thread_entry_wrapper, task);
    AM_ASSERT(0 == ret);
    pthread_attr_destroy(&attr);

    task->name = name;
    pthread_setname_np(task->thread, name);

    if (AM_TASK_FLAG_DETACH == (flags & AM_TASK_FLAG_DETACH)) {
        ret = pthread_detach(task->thread);
        AM_ASSERT(0 == ret);
        AM_ATOMIC_STORE_N(&task->joinable, false);
    } else {
        AM_ATOMIC_STORE_N(&task->joinable, true);
    }

    return am_pal_id_from_index(index);
}

void am_task_notify(int task_id) {
    AM_ASSERT(task_id != AM_TASK_ID_NONE);

    struct am_task* t = am_task_get_hnd(task_id);
    pthread_mutex_lock(&t->mutex);
    AM_ATOMIC_STORE_N(&t->notified, true);
    pthread_cond_signal(&t->cond);
    pthread_mutex_unlock(&t->mutex);
}

void am_task_wait(int task_id) {
    if (AM_TASK_ID_NONE == task_id) {
        task_id = am_task_get_own_id();
    }
    AM_ASSERT(task_id != AM_TASK_ID_NONE);

    struct am_task* t = am_task_get_hnd(task_id);
    pthread_mutex_lock(&t->mutex);
    while (!AM_ATOMIC_LOAD_N(&t->notified)) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    AM_ATOMIC_STORE_N(&t->notified, false);
    pthread_mutex_unlock(&t->mutex);
}

static void am_mutex_init(pthread_mutex_t* me) {
    AM_ASSERT(me);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int rc = pthread_mutex_init(me, &attr);
    AM_ASSERT(0 == rc);
}

int am_mutex_create(void) {
    int mutex = -1;
    for (int i = 0; i < AM_COUNTOF(am_mutexes_); ++i) {
        struct am_mutex* me = &am_mutexes_[i];
        if (!me->valid) {
            mutex = i;
            me->valid = true;
            am_mutex_init(&me->mutex);
            break;
        }
    }
    AM_ASSERT(mutex >= 0);

    return am_pal_id_from_index(mutex);
}

void am_mutex_lock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_mutexes_));
    AM_ASSERT(am_mutexes_[index].valid);

    int rc = pthread_mutex_lock(&am_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
}

void am_mutex_unlock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_mutexes_));
    AM_ASSERT(am_mutexes_[index].valid);

    int rc = pthread_mutex_unlock(&am_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
}

void am_mutex_destroy(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_mutexes_));
    AM_ASSERT(am_mutexes_[index].valid);

    int rc = pthread_mutex_destroy(&am_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
    am_mutexes_[mutex].valid = false;
}

uint32_t am_time_get_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)ms;
}

uint32_t am_time_get_ticks(int timebase) {
    AM_ASSERT(AM_TIMEBASE_DEFAULT == timebase);
    return am_time_get_ms();
}

uint32_t am_time_get_ticks_from_ms(int timebase, uint32_t ms) {
    AM_ASSERT(AM_TIMEBASE_DEFAULT == timebase);
    return ms;
}

uint32_t am_time_get_ms_from_ticks(int timebase, uint32_t ticks) {
    AM_ASSERT(AM_TIMEBASE_DEFAULT == timebase);
    return ticks;
}

void am_sleep_ticks(int timebase, uint32_t ticks) {
    AM_ASSERT(AM_TIMEBASE_DEFAULT == timebase);
    if (0 == ticks) {
        return;
    }
    am_sleep_ms(ticks);
}

void am_sleep_ms(uint32_t ms) {
    if (0 == ms) {
        return;
    }
    struct timespec ts = {
        .tv_sec = ms / 1000,             /**< convert ms -> s */
        .tv_nsec = (ms % 1000) * 1000000 /**< remaining ms -> ns */
    };
    nanosleep(&ts, /*rmtp=*/NULL);
}

void am_sleep_till_ms(uint32_t ms) {
    uint32_t now_ms = am_time_get_ms();
    uint32_t sleep_ms = ms - now_ms;
    if (sleep_ms > UINT32_MAX) {
        return;
    }
    usleep(sleep_ms * 1000);
}

void am_sleep_till_ticks(int timebase, uint32_t ticks) {
    uint32_t now_ticks = am_time_get_ticks(timebase);
    uint32_t sleep_ticks = ticks - now_ticks;
    if ((0 == sleep_ticks) || (sleep_ticks > UINT32_MAX / 2)) {
        return;
    }
    useconds_t us = (useconds_t)sleep_ticks * 1000UL;
    usleep(us);
}

int am_vprintf(const char* fmt, va_list args) {
    am_crit_enter();
    int rc = vprintf(fmt, args);
    am_crit_exit();
    return rc;
}

int am_vprintff(const char* fmt, va_list args) {
    am_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_flush();
    am_crit_exit();
    return rc;
}

int am_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    am_crit_enter();
    int rc = vprintf(fmt, args);
    am_crit_exit();
    va_end(args);
    return rc;
}

int am_printf_unsafe(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_printff(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    am_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_flush();
    am_crit_exit();
    va_end(args);
    return rc;
}

void am_pal_flush(void) { fflush(stdout); }

void* am_pal_init(void* arg) {
    (void)arg;
    struct am_task* task = &task_main_;

    memset(task, 0, sizeof(*task));

    task->thread = pthread_self();
    am_mutex_init(&task->mutex);

    int ret = pthread_cond_init(&task->cond, /*attr=*/NULL);
    AM_ASSERT(0 == ret);
    AM_ATOMIC_STORE_N(&task->running, true);

    init_complete_mutex_ = am_mutex_create();
    am_mutex_lock(init_complete_mutex_);
    init_complete_mutex_acquired_ = true;

    return NULL;
}

void am_pal_deinit(void) {
    if (init_complete_mutex_acquired_) {
        am_mutex_unlock(init_complete_mutex_);
        init_complete_mutex_acquired_ = false;
    }
    for (int i = 0; i < AM_COUNTOF(am_tasks_); ++i) {
        struct am_task* me = &am_tasks_[i];
        if (AM_ATOMIC_LOAD_N(&me->joinable)) {
            pthread_join(me->thread, /*__thread_return=*/NULL);
            AM_ATOMIC_STORE_N(&me->joinable, false);

            int rc = pthread_mutex_destroy(&me->mutex);
            AM_ASSERT(0 == rc);
            rc = pthread_cond_destroy(&me->cond);
            AM_ASSERT(0 == rc);
        }
    }
    for (int i = 0; i < AM_COUNTOF(am_mutexes_); ++i) {
        struct am_mutex* mutex = &am_mutexes_[i];
        if (mutex->valid) {
            int rc = pthread_mutex_destroy(&mutex->mutex);
            AM_ASSERT(0 == rc);
            mutex->valid = false;
        }
    }
}

void am_on_idle(void) {
    am_crit_exit();
    int task_id = am_task_get_own_id();
    am_task_wait(task_id);
    am_crit_enter();
}

int am_get_cpu_count(void) {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nprocs < 1) ? 1 : (int)nprocs;
}

void am_task_run_all(void) {}

void am_task_init_wait(void) {
    int task_id = am_task_get_own_id();

    if (task_id == AM_TASK_ID_MAIN) {
        bool init_complete = false;
        while (!init_complete) {
            init_complete = true;
            for (int i = 0; i < AM_COUNTOF(am_tasks_); ++i) {
                struct am_task* task = &am_tasks_[i];

                if (!AM_ATOMIC_LOAD_N(&task->created)) {
                    continue;
                }
                if ((task->flags & AM_TASK_FLAG_WAIT_INIT) !=
                    AM_TASK_FLAG_WAIT_INIT) {
                    continue;
                }
                if (!AM_ATOMIC_LOAD_N(&task->init_complete)) {
                    init_complete = false;
                    am_task_wait(AM_TASK_ID_MAIN);
                    break;
                }
            }
        }
        init_complete_mutex_acquired_ = false;
    } else {
        struct am_task* this_task = am_task_get_hnd(task_id);

        AM_ATOMIC_STORE_N(&this_task->init_complete, true);

        am_task_notify(AM_TASK_ID_MAIN);

        am_mutex_lock(init_complete_mutex_);
    }

    am_mutex_unlock(init_complete_mutex_);
}

#define NSEC_PER_SEC 1000000000L

/** Ticker handler */
struct am_ticker {
    /** ticker identifier */
    int task_id;
    /** ticker configuration */
    struct am_ticker_cfg cfg;
    /** ticker thread is running */
    bool running;
    /** ticker is busy */
    bool busy;
    /** ticker period [ns] */
    long period_ns;
};

static struct am_ticker tickers[1];

static void timespec_add_ns(struct timespec* ts, long ns) {
    ts->tv_nsec += ns;

    while (ts->tv_nsec >= NSEC_PER_SEC) {
        ts->tv_nsec -= NSEC_PER_SEC;
        ts->tv_sec += 1;
    }
}

static void am_ticker_task(void* arg) {
    struct am_ticker* ticker = arg;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (AM_ATOMIC_LOAD_N(&ticker->running)) {
        timespec_add_ns(&next, ticker->period_ns);

        int rc;
        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        } while ((rc == EINTR) && AM_ATOMIC_LOAD_N(&ticker->running));

        if (!AM_ATOMIC_LOAD_N(&ticker->running)) {
            break;
        }

        if (ticker->cfg.ticker_cb != NULL) {
            ticker->cfg.ticker_cb(ticker->cfg.ctx);
        }
    }
}

int am_ticker_create(const struct am_ticker_cfg* cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->ticker_cb);

    struct am_ticker* ticker = &tickers[0];

    AM_ASSERT(!ticker->busy);

    memset(ticker, 0, sizeof(*ticker));
    ticker->busy = true;
    ticker->cfg = *cfg;
    ticker->period_ns =
        1000000 * am_time_get_ms_from_ticks(cfg->timebase, /*ticks=*/1);

    return am_pal_id_from_index(0);
}

void am_ticker_start(int ticker_id) {
    struct am_ticker* ticker = &tickers[am_pal_index_from_id(ticker_id)];
    AM_ASSERT(ticker->busy);

    bool was_running = AM_ATOMIC_EXCHANGE_N(&ticker->running, true);
    AM_ASSERT(!was_running);

    /* ticker thread to feed timers */
    ticker->task_id = am_task_create(
        "ticker",
        ticker->cfg.priority_hint,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*init=*/NULL,
        /*entry=*/am_ticker_task,
        /*flags=*/AM_TASK_FLAG_WAIT_INIT,
        /*arg=*/ticker
    );
}

void am_ticker_stop(int ticker_id) {
    struct am_ticker* ticker = &tickers[am_pal_index_from_id(ticker_id)];
    AM_ASSERT(ticker->busy);

    bool was_running = AM_ATOMIC_EXCHANGE_N(&ticker->running, false);
    AM_ASSERT(was_running);

    AM_ASSERT(am_task_id_is_valid(ticker->task_id));
    const int task_index = am_pal_index_from_id(ticker->task_id);
    struct am_task* task = &am_tasks_[task_index];
    AM_ASSERT(task);
    if (AM_ATOMIC_LOAD_N(&task->joinable)) {
        pthread_join(task->thread, /*__thread_return=*/NULL);
        AM_ATOMIC_STORE_N(&task->joinable, false);

        int rc = pthread_mutex_destroy(&task->mutex);
        AM_ASSERT(0 == rc);
        rc = pthread_cond_destroy(&task->cond);
        AM_ASSERT(0 == rc);
    }
}
