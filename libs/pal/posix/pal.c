/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

/**
 * @file
 *
 * Platform abstraction layer (PAL) API posix implementation
 */

#ifdef AMAST_PAL_POSIX

#define _GNU_SOURCE

#include <stdarg.h> /* IWYU pragma: keep */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <time.h>
#include <bits/local_lim.h>
#include <sched.h>
#include <features.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "pal/pal.h"

/* to silence unused macro warnings */
AM_ASSERT_STATIC(_POSIX_C_SOURCE);

#define AM_PAL_TICK_DOMAIN_DEFAULT_MS 10

struct am_pal_task {
    /* pthread data */
    pthread_t thread;
    /* mutex to protect condition variable */
    pthread_mutex_t mutex;
    /* condition variable for signaling */
    pthread_cond_t cond;
    /* flag to track notification state */
    bool notified;
    /* the task is valid */
    bool valid;
    /* entry function */
    void (*entry)(void *arg);
    /* entry function argumement */
    void *arg;
};

static struct am_pal_task task_arr_[64] = {0};

struct am_pal_mutex {
    pthread_mutex_t mutex;
    bool valid;
};

static struct am_pal_mutex mutex_arr_[2] = {0};

static int am_pal_index_from_id(int task_id) {
    AM_ASSERT(task_id > 0);
    return task_id - 1;
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

static pthread_mutex_t critical_section_mutex_ = PTHREAD_MUTEX_INITIALIZER;

void am_pal_crit_enter(void) {
    int rc = pthread_mutex_lock(&critical_section_mutex_);
    AM_ASSERT(0 == rc);
}

void am_pal_crit_exit(void) {
    int rc = pthread_mutex_unlock(&critical_section_mutex_);
    AM_ASSERT(0 == rc);
}

int am_pal_task_own_id(void) {
    pthread_t thread = pthread_self();
    for (int i = 0; i < AM_COUNTOF(task_arr_); ++i) {
        if (task_arr_[i].thread == thread) {
            return am_pal_id_from_index(i);
        }
    }
    AM_ASSERT(0);
    return 0;
}

static void am_pal_mutex_init(pthread_mutex_t *hnd);

int am_pal_task_create(
    const char *name,
    const int priority,
    void *stack,
    const int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    (void)stack;
    AM_ASSERT(entry);
    AM_ASSERT(priority >= 0);

    int index = -1;
    struct am_pal_task *task = NULL;
    for (int i = 0; i < AM_COUNTOF(task_arr_); ++i) {
        if (!task_arr_[i].valid) {
            index = i;
            task = &task_arr_[i];
            task_arr_[i].valid = true;
            break;
        }
    }
    AM_ASSERT(task && (index >= 0));

    am_pal_mutex_init(&task->mutex);

    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    AM_ASSERT(0 == ret);

    ret = pthread_attr_setstacksize(
        &attr, (size_t)AM_MAX(stack_size, PTHREAD_STACK_MIN)
    );
    AM_ASSERT(0 == ret);

    ret = pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    AM_ASSERT(0 == ret);

    /* int prio = priority + sched_get_priority_min(SCHED_OTHER); */
    /* struct sched_param param = {.sched_priority = prio}; */
    /* ret = pthread_attr_setschedparam(&attr, &param); */
    /* AM_ASSERT(0 == ret); */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    AM_ASSERT(0 == ret);

    task->entry = entry;
    task->arg = arg;

    ret = pthread_create(&task->thread, &attr, thread_entry_wrapper, task);
    AM_ASSERT(0 == ret);
    pthread_attr_destroy(&attr);

    /* detach the thread so it cleans up after finishing */
    ret = pthread_detach(task->thread);
    AM_ASSERT(0 == ret);

    pthread_setname_np(task->thread, name);

    return am_pal_id_from_index(index);
}

void am_pal_task_notify(int task_id) {
    AM_ASSERT(task_id != AM_PAL_TASK_ID_NONE);

    int index = am_pal_index_from_id(task_id);
    struct am_pal_task *t = &task_arr_[index];
    AM_ASSERT(t);
    pthread_mutex_lock(&t->mutex);
    t->notified = true;
    pthread_cond_signal(&t->cond);
    pthread_mutex_unlock(&t->mutex);
}

void am_pal_task_wait(int task_id) {
    if (AM_PAL_TASK_ID_NONE == task_id) {
        task_id = am_pal_task_own_id();
    }
    int index = am_pal_index_from_id(task_id);
    struct am_pal_task *t = &task_arr_[index];
    AM_ASSERT(t);
    pthread_mutex_lock(&t->mutex);
    while (!t->notified) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    t->notified = false;
    pthread_mutex_unlock(&t->mutex);
}

static void am_pal_mutex_init(pthread_mutex_t *hnd) {
    AM_ASSERT(hnd);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int rc = pthread_mutex_init(hnd, &attr);
    AM_ASSERT(0 == rc);
}

int am_pal_mutex_create(void) {
    int mutex = -1;
    pthread_mutex_t *hnd = NULL;
    for (int i = 0; i < AM_COUNTOF(mutex_arr_); ++i) {
        if (!mutex_arr_[i].valid) {
            mutex = i;
            hnd = &mutex_arr_[i].mutex;
            mutex_arr_[i].valid = true;
            break;
        }
    }
    AM_ASSERT(hnd && (mutex >= 0));

    am_pal_mutex_init(hnd);

    return mutex;
}

void am_pal_mutex_lock(int mutex) {
    AM_ASSERT(mutex < AM_COUNTOF(mutex_arr_));
    AM_ASSERT(mutex_arr_[mutex].valid);
    int rc = pthread_mutex_lock(&mutex_arr_[mutex].mutex);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_unlock(int mutex) {
    AM_ASSERT(mutex < AM_COUNTOF(mutex_arr_));
    AM_ASSERT(mutex_arr_[mutex].valid);
    int rc = pthread_mutex_lock(&mutex_arr_[mutex].mutex);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_destroy(int mutex) {
    AM_ASSERT(mutex < AM_COUNTOF(mutex_arr_));
    AM_ASSERT(mutex_arr_[mutex].valid);
    int rc = pthread_mutex_destroy(&mutex_arr_[mutex].mutex);
    AM_ASSERT(0 == rc);
    mutex_arr_[mutex].valid = false;
}

uint32_t am_pal_time_get_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)ms;
}

uint32_t am_pal_time_get_tick(int domain) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);

    uint32_t ms = am_pal_time_get_ms();
    return (uint32_t)AM_DIVIDE_ROUND_UP(ms, AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);
    return (uint32_t)AM_DIVIDE_ROUND_UP(ms, AM_PAL_TICK_DOMAIN_DEFAULT_MS);
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
    AM_ASSERT(ticks > 0);
    am_pal_sleep_ms(ticks * AM_PAL_TICK_DOMAIN_DEFAULT_MS);
}

void am_pal_sleep_ms(int ms) {
    if (0 == ms) {
        return;
    }
    AM_ASSERT(ms > 0);
    struct timespec ts = {/* convert milliseconds to seconds */
                          .tv_sec = ms / 1000,
                          /* remaining milliseconds to nanoseconds */
                          .tv_nsec = (ms % 1000) * 1000000
    };
    nanosleep(&ts, /*rmtp=*/NULL);
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

void am_pal_ctor(void) {}

void am_pal_dtor(void) {
    for (int i = 0; i < AM_COUNTOF(mutex_arr_); ++i) {
        if (mutex_arr_[i].valid) {
            am_pal_mutex_destroy(i);
        }
    }
}

#endif /* AMAST_PAL_POSIX */
