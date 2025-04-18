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

/* Platform abstraction layer (PAL) API Zephyr implementation. */

#include <stdarg.h>
#include <stdio.h>

/* amast-pragma: verbatim-include-std-on */
#include <zephyr.h>
#include <sys/printk.h>
/* amast-pragma: verbatim-include-std-off */

static struct k_spinlock am_pal_spinlock_;
static k_spinlock_key_t am_pal_spinlock_key_;
static char am_pal_crit_entered_ = 0;

/** Maximum number of mutexes */
#ifndef AM_PAL_MUTEX_NUM_MAX
#define AM_PAL_MUTEX_NUM_MAX 2
#endif

/** PAL mutex descriptor */
struct am_pal_mutex {
    /** Zephyr mutex */
    struct k_mutex mutex;
    /** mutex validity */
    bool valid;
};

static struct am_pal_mutex am_pal_mutexes_[AM_PAL_MUTEX_NUM_MAX] = {0};

/** PAL task descriptor */
struct am_pal_task {
    /** thread data */
    struct k_thread thread;
    /** thread ID */
    k_tid_t tid;
    /** the task is valid */
    bool valid;
    /** entry function */
    void (*entry)(void *arg);
    /** entry function argument */
    void *arg;
};

static struct am_pal_task am_pal_task_main_ = {0};
static struct am_pal_task am_pal_tasks_[AM_PAL_TASK_NUM_MAX] = {0};

void *am_pal_ctor(void *arg) {
    (void)arg;
    am_pal_spinlock_ = (struct k_spinlock){};
    return NULL;
}

void am_pal_dtor(void) {}

void am_pal_crit_enter(void) {
    k_spinlock_key_t key = k_spin_lock(&am_pal_spinlock_);
    AM_ASSERT(!am_pal_crit_entered_);
    am_pal_crit_entered_ = true;
}

void am_pal_crit_exit(void) {
    AM_ASSERT(am_pal_crit_entered_);
    am_pal_crit_entered_ = false;
    k_sched_unlock();
}

int am_pal_mutex_create(void) {
    int mutex = -1;
    for (int i = 0; i < AM_COUNTOF(am_pal_mutexes_); ++i) {
        struct am_pal_mutex *me = &am_pal_mutexes_[i];
        if (!me->valid) {
            mutex = i;
            me->valid = true;
            int rc = k_mutex_init(&me->mutex);
            AM_ASSERT(0 == rc);
            break;
        }
    }
    AM_ASSERT(mutex >= 0);

    return am_pal_id_from_index(mutex);
}

void am_pal_mutex_lock(int mutex) {
    AM_ASSERT(!k_is_in_isr);

    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));

    struct am_pal_mutex *me = &am_pal_mutexes_[index];
    AM_ASSERT(me->valid);

    int rc = k_mutex_lock(&me->mutex, K_FOREVER);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_unlock(int mutex) {
    AM_ASSERT(!k_is_in_isr);

    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));

    struct am_pal_mutex *me = &am_pal_mutexes_[index];
    AM_ASSERT(me->valid);

    int rc = k_mutex_lock(&me->mutex);
    AM_ASSERT(0 == rc);
}

void am_pal_mutex_destroy(int mutex) {
    int index = am_pal_index_from_id(mutex);
    AM_ASSERT(index < AM_COUNTOF(am_pal_mutexes_));

    struct am_pal_mutex *me = &am_pal_mutexes_[index];
    AM_ASSERT(me->valid);

    me->valid = false;
}

static void am_pal_thread_entry(void *p1, void *p2, void *p3) {
    (void)p2;
    (void)p3;

    struct am_pal_task *task = (struct am_pal_task *)p1;

    AM_ASSERT(task->entry);
    task->entry(task->arg);
}

int am_pal_task_create(
    const char *name,
    int priority,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    AM_ASSERT(entry);
    AM_ASSERT(priority >= 0);
    AM_ASSERT(priority < AM_PAL_TASK_NUM_MAX);
    AM_ASSERT(stack);
    AM_ASSERT(stack_size > K_THREAD_STACK_RESERVED);

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

    int zephyr_prio = AM_PAL_TASK_NUM_MAX - priority;

    me->entry = entry;
    me->arg = arg;

    me->tid = k_thread_create(
        /*new_thread=*/&me->thread,
        /*stack=*/(k_thread_stack_t *)stack,
        /*stack_size=*/(size_t)(stack_size - K_THREAD_STACK_RESERVED),
        /*entry=*/am_pal_thread_entry,
        /*p1=*/(void *)me,
        /*p2=*/NULL,
        /*p3=*/NULL,
        /*prio=*/zephyr_prio,
        /*options=*/K_FP_REGS,
        /*delay=*/K_NO_WAIT
    );

    int rc = k_thread_name_set(me->tid, name ? name : "ao");
    AM_ASSERT(0 == rc);

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
    k_wakeup(t->tid);
}

void am_pal_task_wait(int task) { k_sleep(K_FOREVER); }

int am_pal_task_get_own_id(void) {
    k_tid_t tid = k_current_get();
    if (task_main_.tid == tid) {
        return AM_PAL_TASK_ID_MAIN;
    }
    for (int i = 0; i < AM_COUNTOF(am_pal_tasks_); ++i) {
        struct am_pal_task *me = &am_pal_tasks_[i];
        if (me->valid && (me->tid == tid)) {
            return am_pal_id_from_index(i);
        }
    }
    AM_ASSERT(0);
    return AM_PAL_TASK_ID_NONE;
}

uint32_t am_pal_time_get_ms(void) { return k_uptime_get_32(); }

uint32_t am_pal_time_get_tick(int domain) { return k_cycle_get_32(); }

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    return k_ms_to_ticks_ceil32(ms);
}

uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick) {
    return k_ticks_to_ms_ceil32(tick);
}

void am_pal_sleep_ticks(int domain, int ticks) {
    if (ticks < 0) {
        k_sleep(K_FOREVER);
    } else {
        k_sleep(K_TICKS(ticks));
    }
}

void am_pal_sleep_till_ticks(int domain, uint32_t ticks) {
    (void)domain;
    uint32_t now = k_cycle_get_32();
    int64_t delta = (int64_t)ticks - now;
    if (delta > 0) {
        k_sleep(K_TICKS((uint32_t)delta));
    }
}

void am_pal_sleep_ms(int ms) {
    if (ms < 0) {
        k_sleep(K_FOREVER);
    } else {
        k_sleep(K_MSEC(ms));
    }
}

void am_pal_sleep_till_ms(uint32_t ms) {
    uint32_t now = k_uptime_get_32();
    int32_t delta = (int32_t)ms - now;
    if (delta > 0) {
        k_sleep(K_MSEC((uint32_t)delta));
    }
}

int am_pal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = am_pal_vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_pal_printf_unsafe(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintk(fmt, args);
    va_end(args);
    return rc;
}

int am_pal_printff(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = am_pal_vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_pal_vprintf(const char *fmt, va_list args) {
    am_pal_crit_enter();
    int rc = vprintk(fmt, args);
    am_pal_crit_exit();
    return rc;
}

int am_pal_vprintff(const char *fmt, va_list args) {
    return am_pal_vprintf(fmt, args);
}

/* Zephyr does not require an explicit flush for printk. */
void am_pal_flush(void) {}

void am_pal_on_idle(void) {
    am_pal_crit_exit();
    int task = am_pal_task_get_own_id();
    am_pal_task_wait(task);
    am_pal_crit_enter();
}
