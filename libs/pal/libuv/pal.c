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
struct am_task {
    /** libuv thread */
    uv_thread_t thread;
    /** libuv semaphore */
    uv_sem_t semaphore;
    /** libuv thread init */
    void (*init)(void*);
    /** libuv thread entry */
    void (*entry)(void*);
    /** libuv thread argument */
    void* arg;
    /** task name */
    const char* name;
    /** libuv thread ID */
    int id;
    /** priority */
    int prio;
    /* flags */
    unsigned flags;
    /** the task is created */
    bool created;
    /** the task is running */
    bool running;
    /** the task completed its init procedure */
    bool init_complete;
    /** the task is joinable */
    bool joinable;
};

/** PAL mutex descriptor */
struct am_mutex {
    /** libuv mutex */
    uv_mutex_t mutex;
    /** the mutex is valid */
    bool valid;
};

static uv_loop_t* loop_;
static uv_mutex_t crit_section_;

/** Maximum number of mutexes */
#ifndef AM_PAL_MUTEX_NUM_MAX
#define AM_PAL_MUTEX_NUM_MAX 2
#endif

static struct am_mutex mutexes_[AM_PAL_MUTEX_NUM_MAX];

static struct am_task task_main_ = {0};
static struct am_task tasks_[AM_TASK_NUM_MAX];
static int ntasks_ = 0;

static int init_complete_mutex_;
static int init_complete_mutex_acquired_;

static int am_pal_index_from_id(int id) {
    AM_ASSERT(id > 0);
    return id - 1;
}

static int am_pal_id_from_index(int index) {
    AM_ASSERT(index >= 0);
    return index + 1;
}

void* am_pal_create(void* arg) {
    if (arg) {
        loop_ = (uv_loop_t*)arg;
    } else {
        loop_ = malloc(sizeof(uv_loop_t));
        uv_loop_init(loop_);
    }
    uv_mutex_init(&crit_section_);

    struct am_task* task = &task_main_;

    memset(task, 0, sizeof(*task));

    AM_ATOMIC_STORE_N(&task->running, true);
    task->thread = uv_thread_self();
    uv_sem_init(&task->semaphore, 0);

    init_complete_mutex_ = am_mutex_create();

    am_mutex_lock(init_complete_mutex_);

    init_complete_mutex_acquired_ = true;

    return loop_;
}

/* callback to close handles */
static void close_cb(uv_handle_t* handle, void* arg) {
    (void)arg;
    uv_close(handle, NULL);
}

void am_pal_destroy(void) {
    if (init_complete_mutex_acquired_) {
        am_mutex_unlock(init_complete_mutex_);
        init_complete_mutex_acquired_ = false;
    }
    for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
        struct am_task* task = &tasks_[i];
        if (AM_ATOMIC_LOAD_N(&task->joinable)) {
            uv_thread_join(&task->thread);
            uv_sem_destroy(&task->semaphore);
            AM_ATOMIC_STORE_N(&task->joinable, false);
        }
    }
    for (int i = 0; i < AM_COUNTOF(mutexes_); ++i) {
        struct am_mutex* mutex = &mutexes_[i];
        if (mutex->valid) {
            uv_mutex_destroy(&mutex->mutex);
            mutex->valid = false;
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

void am_crit_enter(void) { uv_mutex_lock(&crit_section_); }

void am_crit_exit(void) { uv_mutex_unlock(&crit_section_); }

int am_mutex_create(void) {
    int mutex = -1;
    uv_mutex_t* me = NULL;
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

void am_mutex_lock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_lock(&mutexes_[index].mutex);
}

void am_mutex_unlock(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_unlock(&mutexes_[index].mutex);
}

void am_mutex_destroy(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(mutexes_));
    uv_mutex_destroy(&mutexes_[index].mutex);
    mutexes_[index].valid = false;
}

static void task_entry_wrapper(void* arg) {
    int policy = SCHED_OTHER;
    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);

    struct am_task* task = (struct am_task*)arg;

    AM_ATOMIC_STORE_N(&task->running, true);

    int prio_scaled =
        task->prio * (max_prio - min_prio) / (AM_TASK_NUM_MAX - 1);
    int prio = min_prio + prio_scaled;

    struct sched_param param = {.sched_priority = prio};
    pthread_t thread = pthread_self();
    int ret = pthread_setschedparam(thread, policy, &param);
    AM_ASSERT(0 == ret);

    if (task->init) {
        task->init(task->arg);
    }
    if ((task->flags & AM_TASK_FLAG_WAIT_INIT) == AM_TASK_FLAG_WAIT_INIT) {
        am_task_init_wait();
    }

    task->entry(task->arg);

    if (!AM_ATOMIC_LOAD_N(&task->joinable)) {
        uv_sem_destroy(&task->semaphore);
    }

    AM_ATOMIC_STORE_N(&task->created, false);
}

static bool am_task_id_is_valid(int task_id) {
    if (AM_TASK_ID_MAIN == task_id) {
        return true;
    }
    if ((AM_TASK_ID_NONE == task_id) || (task_id < 0)) {
        return false;
    }
    return task_id <= AM_COUNTOF(tasks_);
}

static struct am_task* am_task_get_hnd(int task_id) {
    AM_ASSERT(am_task_id_is_valid(task_id));
    if (AM_TASK_ID_MAIN == task_id) {
        return &task_main_;
    }
    return &tasks_[am_pal_index_from_id(task_id)];
}

int am_task_create(
    const char* name,
    int prio,
    void* stack,
    int stack_size,
    void (*init)(void*),
    void (*entry)(void*),
    unsigned flags,
    void* arg
) {
    (void)stack;
    (void)stack_size;
    AM_ASSERT(ntasks_ < AM_TASK_NUM_MAX);

    int index = -1;
    struct am_task* task = NULL;
    for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
        task = &tasks_[i];
        bool was_created = AM_ATOMIC_EXCHANGE_N(&task->created, true);
        if (!was_created) {
            index = i;
            break;
        }
        task = NULL;
    }
    AM_ASSERT(task);

    task->init = init;
    task->entry = entry;
    task->arg = arg;
    task->id = am_pal_id_from_index(index);
    task->prio = prio;
    task->flags = flags;
    task->name = name;

    int rc = uv_sem_init(&task->semaphore, 0);
    AM_ASSERT(rc == 0);
    rc = uv_thread_create(&task->thread, task_entry_wrapper, task);
    AM_ASSERT(rc == 0);

    if (AM_TASK_FLAG_DETACH == (flags & AM_TASK_FLAG_DETACH)) {
        rc = uv_thread_detach(&task->thread);
        AM_ASSERT(rc == 0);
        AM_ATOMIC_STORE_N(&task->joinable, false);
    } else {
        AM_ATOMIC_STORE_N(&task->joinable, true);
    }

    return task->id;
}

void am_task_notify(int task_id) {
    AM_ASSERT(task_id != AM_TASK_ID_NONE);

    struct am_task* t = am_task_get_hnd(task_id);
    uv_sem_post(&t->semaphore);
}

void am_task_wait(int task_id) {
    if (AM_TASK_ID_NONE == task_id) {
        task_id = am_task_get_own_id();
    }
    AM_ASSERT(task_id != AM_TASK_ID_NONE);

    struct am_task* t = am_task_get_hnd(task_id);
    uv_sem_wait(&t->semaphore);
}

int am_task_get_own_id(void) {
    uv_thread_t thread = uv_thread_self();
    if (task_main_.thread == thread) {
        return AM_TASK_ID_MAIN;
    }
    uv_thread_t self = uv_thread_self();
    for (int i = 0; i < AM_COUNTOF(tasks_); i++) {
        if (AM_ATOMIC_LOAD_N(&tasks_[i].running) &&
            uv_thread_equal(&tasks_[i].thread, &self)) {
            return tasks_[i].id;
        }
    }
    AM_ASSERT(0);
    return AM_TASK_ID_NONE;
}

uint32_t am_time_get_ms(void) {
    uv_update_time(loop_);
    return (uint32_t)uv_now(loop_);
}

uint32_t am_time_get_ticks(int timebase) {
    (void)timebase;
    return am_time_get_ms();
}

uint32_t am_time_get_ticks_from_ms(int timebase, uint32_t ms) {
    (void)timebase;
    return ms;
}

uint32_t am_time_get_ms_from_ticks(int timebase, uint32_t ticks) {
    (void)timebase;
    return ticks;
}

void am_sleep_ticks(int timebase, uint32_t ticks) {
    (void)timebase;
    AM_ASSERT(ticks <= UINT_MAX);
    uv_sleep((unsigned)ticks);
}

void am_sleep_till_ticks(int timebase, uint32_t ticks) {
    uint32_t now = am_time_get_ticks(timebase);
    if (ticks > now) {
        uint32_t diff = ticks - now;
        am_sleep_ticks(timebase, diff);
    }
}

void am_sleep_ms(uint32_t ms) { uv_sleep((unsigned)ms); }

void am_sleep_till_ms(uint32_t ms) {
    uint32_t now = am_time_get_ms();
    if (ms > now) {
        uint32_t diff = ms - now;
        am_sleep_ms(diff);
    }
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

void am_on_idle(void) {
    am_crit_exit();
    am_task_wait(am_task_get_own_id());
    am_crit_enter();
}

int am_get_cpu_count(void) {
    int count;
    uv_cpu_info_t* info;

    if (uv_cpu_info(&info, &count) == 0) {
        uv_free_cpu_info(info, count);
        return count;
    }
    return 1;
}

void am_task_init_wait(void) {
    int task_id = am_task_get_own_id();

    if (task_id == AM_TASK_ID_MAIN) {
        bool init_complete = false;
        while (!init_complete) {
            init_complete = true;
            for (int i = 0; i < AM_COUNTOF(tasks_); ++i) {
                struct am_task* task = &tasks_[i];

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

void am_task_run_all(void) {}

#define NSEC_PER_MSEC 1000000ULL

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

static void am_ticker_task(void* arg) {
    struct am_ticker* ticker = arg;
    AM_ASSERT(ticker);

    const uint64_t period_ns = (uint64_t)ticker->period_ns;
    uint64_t next_ns = uv_hrtime();

    while (AM_ATOMIC_LOAD_N(&ticker->running)) {
        next_ns += period_ns;

        while (AM_ATOMIC_LOAD_N(&ticker->running)) {
            uint64_t now_ns = uv_hrtime();
            if (now_ns >= next_ns) {
                break;
            }

            uint64_t sleep_ns = next_ns - now_ns;
            unsigned sleep_ms = (unsigned)(sleep_ns / NSEC_PER_MSEC);
            if (sleep_ms == 0) {
                sleep_ms = 1;
            }
            uv_sleep(sleep_ms);
        }

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
        (long)(NSEC_PER_MSEC * am_time_get_ms_from_ticks(cfg->timebase, 1));
    AM_ASSERT(ticker->period_ns > 0);

    return am_pal_id_from_index(0);
}

void am_ticker_start(int ticker_id) {
    struct am_ticker* ticker = &tickers[am_pal_index_from_id(ticker_id)];
    AM_ASSERT(ticker->busy);

    bool was_running = AM_ATOMIC_EXCHANGE_N(&ticker->running, true);
    AM_ASSERT(!was_running);

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
    struct am_task* task = &tasks_[task_index];

    if (AM_ATOMIC_LOAD_N(&task->joinable)) {
        uv_thread_join(&task->thread);
        uv_sem_destroy(&task->semaphore);
        AM_ATOMIC_STORE_N(&task->joinable, false);
    }

    ticker->task_id = AM_TASK_ID_NONE;
}
