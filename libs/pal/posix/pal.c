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

#include <stdbool.h>

#define _GNU_SOURCE

#include <stdint.h>
#include <stdlib.h>

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
    void *arg;
    /* entry point */
    void (*entry)(void *arg);
};

static pthread_mutex_t critical_section_mutex_ = PTHREAD_MUTEX_INITIALIZER;

void am_pal_crit_enter(void) {
    int result = pthread_mutex_lock(&critical_section_mutex_);
    AM_ASSERT(0 == result);
}

void am_pal_crit_exit(void) {
    int result = pthread_mutex_unlock(&critical_section_mutex_);
    AM_ASSERT(0 == result);
}

void *am_pal_task_create(
    const char *name,
    const int priority,
    void *stack,
    const int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    AM_ASSERT(stack);
    AM_ASSERT(stack_size > 0);
    AM_ASSERT(entry);
    AM_ASSERT(priority >= 0);
    (void)arg;

    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    AM_ASSERT(0 == ret);

    ret = pthread_attr_setstacksize(
        &attr, (size_t)AM_MAX(stack_size, PTHREAD_STACK_MIN)
    );
    AM_ASSERT(0 == ret);

    ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    AM_ASSERT(0 == ret);

    int prio = priority + sched_get_priority_min(SCHED_RR);
    struct sched_param param = {.sched_priority = prio};
    ret = pthread_attr_setschedparam(&attr, &param);
    AM_ASSERT(0 == ret);
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    AM_ASSERT(0 == ret);

    struct am_pal_task *task = malloc(sizeof(struct am_pal_task));
    AM_ASSERT(0 == task);

    pthread_setname_np(task->thread, name);

    return task;
}

void am_pal_task_notify(void *task) {
    AM_ASSERT(task);

    struct am_pal_task *t = (struct am_pal_task *)task;
    pthread_mutex_lock(&t->mutex);
    t->notified = true;
    pthread_cond_signal(&t->cond);
    pthread_mutex_unlock(&t->mutex);
}

void am_pal_task_wait(void *task) {
    AM_ASSERT(task);

    struct am_pal_task *t = (struct am_pal_task *)task;
    pthread_mutex_lock(&t->mutex);
    while (!t->notified) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    t->notified = false;
    pthread_mutex_unlock(&t->mutex);
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

#endif /* AMAST_PAL_POSIX */
