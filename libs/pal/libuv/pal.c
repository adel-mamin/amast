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
 * Platform abstraction layer (PAL) API libuv implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include <uv.h>

#include "common/compiler.h"
#include "common/macros.h"
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
    /** priority */
    int prio;
    /** mak task as valid */
    bool valid;
};

/** PAL mutex descriptor */
struct am_pal_mutex {
    /** libuv mutex */
    uv_mutex_t mutex;
    /** the mutex is valid */
    bool valid;
};

static uv_loop_t *loop_;
static uv_mutex_t crit_section_;

/** Maximum number of mutexes */
#ifndef AM_PAL_MUTEX_NUM_MAX
#define AM_PAL_MUTEX_NUM_MAX 2
#endif

static struct am_pal_mutex mutexes_[AM_PAL_MUTEX_NUM_MAX];

static struct am_pal_task task_main_ = {0};
static struct am_pal_task tasks_[AM_PAL_TASK_NUM_MAX];
static int ntasks_ = 0;

static int startup_complete_mutex_;

static int am_pal_index_from_id(int id) {
    AM_ASSERT(id > 0);
    return id - 1;
}

static int am_pal_id_from_index(int index) {
    AM_ASSERT(index >= 0);
    return index + 1;
}

void *am_pal_ctor(void *arg) {
    if (arg) {
        loop_ = (uv_loop_t *)arg;
    } else {
        loop_ = malloc(sizeof(uv_loop_t));
        uv_loop_init(loop_);
    }
    uv_mutex_init(&crit_section_);

    struct am_pal_task *task = &task_main_;

    memset(task, 0, sizeof(*task));

    task->valid = true;
    task->thread = uv_thread_self();
    uv_sem_init(&task->semaphore, 0);

    startup_complete_mutex_ = am_pal_mutex_create();

    return loop_;
}

/* callback to close handles */
static void close_cb(uv_handle_t *handle, void *arg) {
    (void)arg;
    uv_close(handle, NULL);
}

void am_pal_dtor(void) {
    for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
        if (tasks_[i].valid) {
            uv_thread_join(&tasks_[i].thread);
            tasks_[i].valid = false;
        }
    }
    for (int i = 0; i < AM_COUNTOF(mutexes_); ++i) {
        struct am_pal_mutex *mutex = &mutexes_[i];
        if (mutex->valid) {
            uv_mutex_destroy(&mutex->mutex);
            mutex->valid = false;
        }
    }
    for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
        struct am_pal_task *task = &tasks_[i];
        if (task->valid) {
            uv_sem_destroy(&task->semaphore);
            task->valid = false;
        }
    }
    uv_mutex_destroy(&crit_section_);

    /* close all handles */
    uv_walk(loop_, close_cb, NULL);

    /* run the loop to process closing handles */
    uv_run(loop_, UV_RUN_DEFAULT);
    uv_run(loop_, UV_RUN_NOWAIT);

    int rc = uv_loop_close(loop_);
    AM_ASSERT(0 == rc);
    free(loop_);
}

void am_pal_crit_enter(void) { uv_mutex_lock(&crit_section_); }

void am_pal_crit_exit(void) { uv_mutex_unlock(&crit_section_); }

int am_pal_mutex_create(void) {
    int mutex = -1;
    uv_mutex_t *me = NULL;
    for (int i = 0; i < AM_COUNTOF(mutexes_); ++i) {
        if (!mutexes_[i].valid) {
            mutex = i;
            me = &mutexes_[i].mutex;
            mutexes_[i].valid = true;
            break;
        }
    }
    AM_ASSERT(me && (mutex >= 0));

    uv_mutex_init(me);

    return am_pal_id_from_index(mutex);
}

void am_pal_mutex_lock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_lock(&mutexes_[index].mutex);
}

void am_pal_mutex_unlock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_unlock(&mutexes_[index].mutex);
}

void am_pal_mutex_destroy(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_destroy(&mutexes_[index].mutex);
    mutexes_[index].valid = false;
}

static void task_entry_wrapper(void *arg) {
    struct am_pal_task *task = (struct am_pal_task *)arg;

    int policy = SCHED_OTHER;
    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);

    int prio_scaled =
        task->prio * (max_prio - min_prio) / (AM_PAL_TASK_NUM_MAX - 1);
    int prio = min_prio + prio_scaled;

    struct sched_param param = {.sched_priority = prio};
    pthread_t thread = pthread_self();
    int ret = pthread_setschedparam(thread, policy, &param);
    AM_ASSERT(0 == ret);

    task->entry(task->arg);
}

int am_pal_task_create(
    const char *name,
    int prio,
    void *stack,
    int stack_size,
    void (*entry)(void *),
    void *arg
) {
    (void)name;
    (void)stack;
    (void)stack_size;
    AM_ASSERT(ntasks_ < AM_PAL_TASK_NUM_MAX);

    int index = -1;
    struct am_pal_task *task = NULL;
    for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
        if (!tasks_[i].valid) {
            index = i;
            task = &tasks_[i];
            tasks_[i].valid = true;
            break;
        }
    }
    AM_ASSERT(task && (index >= 0));

    AM_DISABLE_WARNING(AM_W_NULL_DEREFERENCE)
    task->entry = entry;
    task->arg = arg;
    task->id = am_pal_id_from_index(index);
    task->prio = prio;
    AM_ENABLE_WARNING(AM_W_NULL_DEREFERENCE)

    int rc = uv_sem_init(&task->semaphore, 0);
    AM_ASSERT(rc == 0);
    rc = uv_thread_create(&task->thread, task_entry_wrapper, task);
    AM_ASSERT(rc == 0);

    return task->id;
}

void am_pal_task_notify(int task) {
    AM_ASSERT(task != AM_PAL_TASK_ID_NONE);

    struct am_pal_task *t = NULL;
    if (AM_PAL_TASK_ID_MAIN == task) {
        t = &task_main_;
    } else {
        t = &tasks_[am_pal_index_from_id(task)];
    }
    uv_sem_post(&t->semaphore);
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
        t = &tasks_[am_pal_index_from_id(task)];
    }
    uv_sem_wait(&t->semaphore);
}

int am_pal_task_get_own_id(void) {
    uv_thread_t thread = uv_thread_self();
    if (task_main_.thread == thread) {
        return AM_PAL_TASK_ID_MAIN;
    }
    uv_thread_t self = uv_thread_self();
    for (int i = 0; i < AM_COUNTOF(tasks_); i++) {
        if (tasks_[i].valid && uv_thread_equal(&tasks_[i].thread, &self)) {
            return tasks_[i].id;
        }
    }
    AM_ASSERT(0);
    return AM_PAL_TASK_ID_NONE;
}

uint32_t am_pal_time_get_ms(void) { return (uint32_t)(uv_hrtime() / 1000000); }

uint32_t am_pal_time_get_tick(int domain) {
    (void)domain;
    return am_pal_time_get_ms();
}

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    (void)domain;
    return ms;
}

uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick) {
    (void)domain;
    return tick;
}

void am_pal_sleep_ticks(int domain, int ticks) {
    (void)domain;
    AM_ASSERT(ticks >= 0);
    AM_ASSERT(ticks <= INT_MAX);
    uv_sleep((unsigned)ticks);
}

void am_pal_sleep_till_ticks(int domain, uint32_t ticks) {
    uint32_t now = am_pal_time_get_tick(domain);
    if (ticks > now) {
        uint32_t diff = ticks - now;
        AM_ASSERT(diff <= INT_MAX);
        am_pal_sleep_ticks(domain, (int)diff);
    }
}

void am_pal_sleep_ms(int ms) {
    AM_ASSERT(ms >= 0);
    uv_sleep((unsigned)ms);
}

void am_pal_sleep_till_ms(uint32_t ms) {
    uint32_t now = am_pal_time_get_ms();
    if (ms > now) {
        uint32_t diff = ms - now;
        AM_ASSERT(diff <= INT_MAX);
        am_pal_sleep_ms((int)diff);
    }
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

void am_pal_on_idle(void) {
    am_pal_crit_exit();
    am_pal_task_wait(am_pal_task_get_own_id());
    am_pal_crit_enter();
}

int am_pal_get_cpu_count(void) {
    int count;
    uv_cpu_info_t *info;

    if (uv_cpu_info(&info, &count) == 0) {
        uv_free_cpu_info(info, count);
        return count;
    }
    return 1;
}

void am_pal_lock_all(void) { am_pal_mutex_lock(startup_complete_mutex_); }

void am_pal_unlock_all(void) { am_pal_mutex_unlock(startup_complete_mutex_); }

void am_pal_wait_all(void) {
    am_pal_mutex_lock(startup_complete_mutex_);
    am_pal_mutex_unlock(startup_complete_mutex_);
}

void am_pal_run_all(void) {}
