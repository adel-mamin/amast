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

/* amast-pragma: verbatim-include-std-off */

#include "common/compiler.h"
#include "common/macros.h"
#include "pal/pal.h"

/* to silence unused macro warnings */
AM_ASSERT_STATIC(_POSIX_C_SOURCE);

/** Default tick rate [ms] */
#define AM_PAL_TICK_DOMAIN_DEFAULT_MS 10

/** PAL task descriptor */
struct am_pal_task {
    /** pthread data */
    pthread_t thread;
    /** mutex to protect condition variable */
    pthread_mutex_t mutex;
    /** condition variable for signaling */
    pthread_cond_t cond;
    /** flag to track notification state */
    bool notified;
    /** the task is valid */
    bool valid;
    /** entry function */
    void (*entry)(void *arg);
    /** entry function argument */
    void *arg;
};

static struct am_pal_task task_main_ = {0};
static struct am_pal_task am_pal_tasks_[AM_PAL_TASK_NUM_MAX] = {0};
static int startup_complete_mutex_;

/** PAL mutex descriptor */
struct am_pal_mutex {
    /** pthread mutex */
    pthread_mutex_t mutex;
    /** the mutex is valid */
    bool valid;
};

/** Maximum number of mutexes */
#ifndef AM_PAL_MUTEX_NUM_MAX
#define AM_PAL_MUTEX_NUM_MAX 2
#endif

static struct am_pal_mutex am_pal_mutexes_[AM_PAL_MUTEX_NUM_MAX] = {0};

static int am_pal_index_from_id(int id) {
    AM_ASSERT(id > 0);
    return id - 1;
}

static int am_pal_id_from_index(int index) {
    AM_ASSERT(index >= 0);
    return index + 1;
}

static void *thread_entry_wrapper(void *arg) {
    AM_ASSERT(arg);
    struct am_pal_task *ctx = (struct am_pal_task *)arg;
    AM_ASSERT(ctx->entry);

    ctx->entry(ctx->arg);
    ctx->valid = false;
    int rc = pthread_mutex_destroy(&ctx->mutex);
    AM_ASSERT(0 == rc);

    return NULL;
}

static pthread_mutex_t am_pal_crit_mutex_ = PTHREAD_MUTEX_INITIALIZER;
static bool am_pal_crit_entered_ = false;

void am_pal_crit_enter(void) {
    int rc = pthread_mutex_lock(&am_pal_crit_mutex_);
    AM_ASSERT(!am_pal_crit_entered_);
    am_pal_crit_entered_ = true;
    AM_ASSERT(0 == rc);
}

void am_pal_crit_exit(void) {
    AM_ASSERT(am_pal_crit_entered_);
    am_pal_crit_entered_ = false;
    int rc = pthread_mutex_unlock(&am_pal_crit_mutex_);
    AM_ASSERT(0 == rc);
}

int am_pal_task_get_own_id(void) {
    pthread_t thread = pthread_self();
    if (task_main_.thread == thread) {
        return AM_PAL_TASK_ID_MAIN;
    }
    for (int i = 0; i < AM_COUNTOF(am_pal_tasks_); ++i) {
        struct am_pal_task *me = &am_pal_tasks_[i];
        if (me->valid && (me->thread == thread)) {
            return am_pal_id_from_index(i);
        }
    }
    AM_ASSERT(0);
    return AM_PAL_TASK_ID_NONE;
}

static void am_pal_mutex_init(pthread_mutex_t *me);

int am_pal_task_create(
    const char *name,
    int prio,
    void *stack,
    const int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    (void)stack;
    AM_ASSERT(entry);
    AM_ASSERT(prio >= 0);
    AM_ASSERT(prio < AM_PAL_TASK_NUM_MAX);

    int index = -1;
    struct am_pal_task *me = NULL;
    for (int i = 0; i < AM_COUNTOF(am_pal_tasks_); ++i) {
        me = &am_pal_tasks_[i];
        if (!me->valid) {
            index = i;
            me->valid = true;
            break;
        }
    }
    AM_ASSERT(index >= 0);

    am_pal_mutex_init(&me->mutex);

    int ret = pthread_cond_init(&me->cond, /*attr=*/NULL);
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

    int prio_scaled = prio * (max_prio - min_prio) / (AM_PAL_TASK_NUM_MAX - 1);
    prio = min_prio + prio_scaled;

    struct sched_param param = {.sched_priority = prio};
    ret = pthread_attr_setschedparam(&attr, &param);
    AM_ASSERT(0 == ret);
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    AM_ASSERT(0 == ret);

    me->entry = entry;
    me->arg = arg;

    ret = pthread_create(&me->thread, &attr, thread_entry_wrapper, me);
    AM_ASSERT(0 == ret);
    pthread_attr_destroy(&attr);

    pthread_setname_np(me->thread, name);

    return am_pal_id_from_index(index);
}

void am_pal_task_notify(int task) {
    AM_ASSERT(task != AM_PAL_TASK_ID_NONE);

    struct am_pal_task *t = NULL;
    if (AM_PAL_TASK_ID_MAIN == task) {
        t = &task_main_;
    } else {
        t = &am_pal_tasks_[am_pal_index_from_id(task)];
    }
    pthread_mutex_lock(&t->mutex);
    AM_ATOMIC_STORE_N(&t->notified, true);
    pthread_cond_signal(&t->cond);
    pthread_mutex_unlock(&t->mutex);
}

void am_pal_task_wait(int task) {
    if (AM_PAL_TASK_ID_NONE == task) {
        task = am_pal_task_get_own_id();
    }
    AM_ASSERT(task != AM_PAL_TASK_ID_NONE);

    struct am_pal_task *t = NULL;
    if (AM_PAL_TASK_ID_MAIN == task) {
        t = &task_main_;
    } else {
        t = &am_pal_tasks_[am_pal_index_from_id(task)];
    }
    pthread_mutex_lock(&t->mutex);
    while (!AM_ATOMIC_LOAD_N(&t->notified)) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    AM_ATOMIC_STORE_N(&t->notified, false);
    pthread_mutex_unlock(&t->mutex);
}

static void am_pal_mutex_init(pthread_mutex_t *me) {
    AM_ASSERT(me);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int rc = pthread_mutex_init(me, &attr);
    AM_ASSERT(0 == rc);
}

int am_pal_mutex_create(void) {
    int mutex = -1;
    for (int i = 0; i < AM_COUNTOF(am_pal_mutexes_); ++i) {
        struct am_pal_mutex *me = &am_pal_mutexes_[i];
        if (!me->valid) {
            mutex = i;
            me->valid = true;
            am_pal_mutex_init(&me->mutex);
            break;
        }
    }
    AM_ASSERT(mutex >= 0);

    return am_pal_id_from_index(mutex);
}

void am_pal_mutex_lock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));
    AM_ASSERT(am_pal_mutexes_[index].valid);

    int rc = pthread_mutex_lock(&am_pal_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_unlock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));
    AM_ASSERT(am_pal_mutexes_[index].valid);

    int rc = pthread_mutex_unlock(&am_pal_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_destroy(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));
    AM_ASSERT(am_pal_mutexes_[index].valid);

    int rc = pthread_mutex_destroy(&am_pal_mutexes_[index].mutex);
    AM_ASSERT(0 == rc);
    am_pal_mutexes_[mutex].valid = false;
}

uint32_t am_pal_time_get_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)ms;
}

uint32_t am_pal_time_get_tick(int domain) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);

    uint32_t ms = am_pal_time_get_ms();
    return (uint32_t)AM_DIV_CEIL(ms, AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);

    return (uint32_t)AM_DIV_CEIL(ms, AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);

    return (uint32_t)(tick * AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

void am_pal_sleep_ticks(int domain, int ticks) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);
    if (0 == ticks) {
        return;
    }
    if (ticks < 0) {
        am_pal_sleep_ms(-1);
        return;
    }
    am_pal_sleep_ms(ticks * AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

void am_pal_sleep_ms(int ms) {
    if (0 == ms) {
        return;
    }
    if (ms < 0) {
        struct timespec req = {
            .tv_sec = 86400, .tv_nsec = 0
        }; /* 1 day in seconds */
        while (1) {
            nanosleep(&req, NULL); /* Sleep for 1 day at a time */
        }
    }
    struct timespec ts = {
        .tv_sec = ms / 1000,             /**< convert ms -> s */
        .tv_nsec = (ms % 1000) * 1000000 /**< remaining ms -> ns */
    };
    nanosleep(&ts, /*rmtp=*/NULL);
}

void am_pal_sleep_till_ms(uint32_t ms) {
    uint32_t now_ms = am_pal_time_get_ms();
    uint32_t sleep_ms = ms - now_ms;
    if (sleep_ms > UINT32_MAX) {
        return;
    }
    usleep(sleep_ms * 1000);
}

void am_pal_sleep_till_ticks(int domain, uint32_t ticks) {
    uint32_t now_ticks = am_pal_time_get_tick(domain);
    uint32_t sleep_ticks = ticks - now_ticks;
    if ((0 == sleep_ticks) || (sleep_ticks > UINT32_MAX / 2)) {
        return;
    }
    useconds_t us =
        (useconds_t)sleep_ticks * AM_PAL_TICK_DOMAIN_DEFAULT_MS * 1000UL;
    usleep(us);
}

int am_pal_vprintf(const char *fmt, va_list args) {
    am_pal_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_crit_exit();
    return rc;
}

int am_pal_vprintff(const char *fmt, va_list args) {
    am_pal_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_flush();
    am_pal_crit_exit();
    return rc;
}

int am_pal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    am_pal_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_crit_exit();
    va_end(args);
    return rc;
}

int am_pal_printf_unsafe(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_pal_printff(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    am_pal_crit_enter();
    int rc = vprintf(fmt, args);
    am_pal_flush();
    am_pal_crit_exit();
    va_end(args);
    return rc;
}

void am_pal_flush(void) { fflush(stdout); }

void *am_pal_ctor(void *arg) {
    (void)arg;
    struct am_pal_task *task = &task_main_;

    memset(task, 0, sizeof(*task));

    task->thread = pthread_self();
    am_pal_mutex_init(&task->mutex);

    int ret = pthread_cond_init(&task->cond, /*attr=*/NULL);
    AM_ASSERT(0 == ret);
    task->valid = true;

    startup_complete_mutex_ = am_pal_mutex_create();

    return NULL;
}

void am_pal_dtor(void) {
    for (int i = 0; i < AM_COUNTOF(am_pal_tasks_); ++i) {
        struct am_pal_task *me = &am_pal_tasks_[i];
        if (me->valid) {
            pthread_join(me->thread, /*__thread_return=*/NULL);
            me->valid = false;
        }
    }
    for (int i = 0; i < AM_COUNTOF(am_pal_mutexes_); ++i) {
        struct am_pal_mutex *mutex = &am_pal_mutexes_[i];
        if (mutex->valid) {
            int rc = pthread_mutex_destroy(&mutex->mutex);
            AM_ASSERT(0 == rc);
            mutex->valid = false;
        }
    }
    for (int i = 0; i < AM_COUNTOF(am_pal_tasks_); ++i) {
        struct am_pal_task *task = &am_pal_tasks_[i];
        if (task->valid) {
            int rc = pthread_mutex_destroy(&task->mutex);
            AM_ASSERT(0 == rc);
            rc = pthread_cond_destroy(&task->cond);
            AM_ASSERT(0 == rc);
            task->valid = false;
        }
    }
}

void am_pal_on_idle(void) {
    am_pal_crit_exit();
    int task = am_pal_task_get_own_id();
    am_pal_task_wait(task);
    am_pal_crit_enter();
}

int am_pal_get_cpu_count(void) {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nprocs < 1) ? 1 : (int)nprocs;
}

void am_pal_run_all_tasks(void) {}

void am_pal_lock_all_tasks(void) { am_pal_mutex_lock(startup_complete_mutex_); }

void am_pal_unlock_all_tasks(void) {
    am_pal_mutex_unlock(startup_complete_mutex_);
}

void am_pal_wait_all_tasks(void) {
    am_pal_mutex_lock(startup_complete_mutex_);
    am_pal_mutex_unlock(startup_complete_mutex_);
}
