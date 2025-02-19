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

/**
 * Platform abstraction layer (PAL) API libuv implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include <uv.h>

#include "pal/pal.h"

/** PAL task descriptor */
struct am_pal_task {
    /** libuv thread */
    uv_thread_t thread;
    /** libuv semaphore */
    uv_sem_t semaphore;
    /** libuv thread entry */
    void (*entry)(void *);
    /** libuv thread argument */
    void *arg;
    /** libuv thread ID */
    int id;
};

static uv_loop_t *loop_;
static uv_mutex_t crit_section_;
static uv_mutex_t mutexes_[AM_PAL_TASK_NUM_MAX];
static struct am_pal_task tasks_[AM_PAL_TASK_NUM_MAX];
static int ntasks_ = 0;

void am_pal_ctor(void) {
    loop_ = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop_);
    uv_mutex_init(&crit_section_);
}

void am_pal_dtor(void) {
    uv_loop_close(loop_);
    free(loop_);
    uv_mutex_destroy(&crit_section_);
}

void am_pal_crit_enter(void) { uv_mutex_lock(&crit_section_); }

void am_pal_crit_exit(void) { uv_mutex_unlock(&crit_section_); }

int am_pal_mutex_create(void) {
    static int mutex_id = 0;
    AM_ASSERT(mutex_id < AM_PAL_TASK_NUM_MAX);
    uv_mutex_init(&mutexes_[mutex_id]);
    return mutex_id++;
}

void am_pal_mutex_lock(int mutex) { uv_mutex_lock(&mutexes_[mutex]); }

void am_pal_mutex_unlock(int mutex) { uv_mutex_unlock(&mutexes_[mutex]); }

void am_pal_mutex_destroy(int mutex) { uv_mutex_destroy(&mutexes_[mutex]); }

static void task_entry_wrapper(void *arg) {
    struct am_pal_task *task = (struct am_pal_task *)arg;
    task->entry(task->arg);
}

int am_pal_task_create(
    const char *name,
    int priority,
    void *stack,
    int stack_size,
    void (*entry)(void *),
    void *arg
) {
    AM_ASSERT(ntasks_ < AM_PAL_TASK_NUM_MAX);

    tasks_[ntasks_].entry = entry;
    tasks_[ntasks_].arg = arg;
    tasks_[ntasks_].id = ntasks_;
    uv_sem_init(&tasks_[ntasks_].semaphore, 0);
    uv_thread_create(
        &tasks_[ntasks_].thread, task_entry_wrapper, &tasks_[ntasks_]
    );

    return ntasks_++;
}

void am_pal_task_notify(int task) { uv_sem_post(&tasks_[task].semaphore); }

void am_pal_task_wait(int task) { uv_sem_wait(&tasks_[task].semaphore); }

int am_pal_task_get_own_id(void) {
    uv_thread_t self = uv_thread_self();
    for (int i = 0; i < ntasks_; i++) {
        if (uv_thread_equal(&tasks_[i].thread, &self)) {
            return tasks_[i].id;
        }
    }
    return -1;
}

uint32_t am_pal_time_get_ms(void) { return (uint32_t)uv_hrtime() / 1000000; }

uint32_t am_pal_time_get_tick(int domain) { return am_pal_time_get_ms(); }

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) { return ms; }

uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick) {
    return tick;
}

void am_pal_sleep_ticks(int domain, int ticks) { uv_sleep(ticks); }

void am_pal_sleep_till_ticks(int domain, uint32_t ticks) {
    uint32_t now = am_pal_time_get_tick(domain);
    if (ticks > now) {
        am_pal_sleep_ticks(domain, ticks - now);
    }
}

void am_pal_sleep_ms(int ms) { uv_sleep(ms); }

void am_pal_sleep_till_ms(uint32_t ms) {
    uint32_t now = am_pal_time_get_ms();
    if (ms > now) {
        am_pal_sleep_ms(ms - now);
    }
}

int am_pal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

void am_pal_flush(void) { fflush(stdout); }

void am_pal_on_idle(void) {
    am_pal_crit_exit();
    am_pal_task_wait(am_pal_task_get_own_id());
    am_pal_crit_enter();
}
