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
 * @file
 *
 * Platform Abstraction Layer (PAL) API documentation.
 */

#ifndef AM_PAL_H_INCLUDED
#define AM_PAL_H_INCLUDED

#include <stdint.h>
#include <stdarg.h>

/** Maximum number of PAL tasks. */
#define AM_PAL_TASK_NUM_MAX 64

/** Invalid task ID. */
#define AM_PAL_TASK_ID_NONE 0

/** Main task ID. */
#define AM_PAL_TASK_ID_MAIN -1

/** Default tick domain. */
#define AM_PAL_TICK_DOMAIN_DEFAULT 0

#ifndef AM_PAL_TICK_DOMAIN_MAX
#define AM_PAL_TICK_DOMAIN_MAX 1 /**< Total number of tick domains. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PAL constructor.
 *
 * @param arg  platform specific handler. Can be NULL.
 *
 * @return Platform specific handler. Can be NULL.
 */
void *am_pal_ctor(void *arg);

/** PAL destructor. */
void am_pal_dtor(void);

/**
 * Enter critical section.
 *
 * Non reentrant.
 */
void am_pal_crit_enter(void);

/**
 * Exit critical section.
 *
 * Non reentrant.
 */
void am_pal_crit_exit(void);

/**
 * Create mutex.
 *
 * @return unique mutex ID
 */
int am_pal_mutex_create(void);

/**
 * Lock mutex.
 *
 * If the mutex is locked by another task,
 * the calling task waits until the mutex becomes available.
 *
 * A task is not permitted to lock a mutex it has already locked.
 *
 * May not be called from ISRs.
 *
 * @param mutex  the mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_lock(int mutex);

/**
 * Unlock mutex.
 *
 * The mutex must already be locked with am_pal_mutex_lock()
 * by the calling task.
 *
 * The mutex cannot be claimed by another task until it has been
 * unlocked by the calling task.
 *
 * Mutexes may not be unlocked in ISRs.
 *
 * @param mutex  the mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_unlock(int mutex);

/**
 * Destroy mutex.
 *
 * @param mutex  the mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_destroy(int mutex);

/**
 * Initialize a task, then schedules it for execution.
 *
 * The new task may be scheduled for immediate execution.
 * The kernel scheduler may preempt the current task to allow
 * the new task to execute.
 *
 * @param name        human readable task name. Not copied.
 *                    Must remain valid after the call.
 * @param prio        task priority [0, #AM_PAL_TASK_NUM_MAX[.
 * @param stack       task stack
 * @param stack_size  task stack size [bytes]
 * @param entry       task entry function
 * @param arg         task entry function argument.
 *                    PAL does not use other than providing it as an argument
 *                    to task entry function
 * @return unique task ID
 */
int am_pal_task_create(
    const char *name,
    int prio,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
);

/**
 * Wake up PAL task.
 *
 * @param task  task ID returned by am_pal_task_create()
 */
void am_pal_task_notify(int task);

/**
 * Block PAL task till am_pal_task_notify() is called.
 *
 * @param task  the task ID returned by am_pal_task_create()
 */
void am_pal_task_wait(int task);

/**
 * Return task own ID.
 *
 * @return task ID
 */
int am_pal_task_get_own_id(void);

/**
 * Get current time in milliseconds.
 *
 * @return current time [ms]
 */
uint32_t am_pal_time_get_ms(void);

/**
 * Get current time in ticks.
 *
 * @param domain  tick domain [0, #AM_PAL_TICK_DOMAIN_MAX[
 *
 * @return current time [tick]
 */
uint32_t am_pal_time_get_tick(int domain);

/**
 * Convert ms to ticks for the given tick domain.
 *
 * @param domain  tick domain [0, #AM_PAL_TICK_DOMAIN_MAX[
 * @param ms      milliseconds to convert
 *
 * @return time [tick]
 */
uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms);

/**
 * Convert ticks from the given tick domain to milliseconds.
 *
 * @param domain  tick domain [0, #AM_PAL_TICK_DOMAIN_MAX[
 * @param tick    ticks to convert
 *
 * @return time [ms]
 */
uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick);

/**
 * Sleep for given number of ticks from the given tick domain.
 *
 * @param domain  tick domain [0, #AM_PAL_TICK_DOMAIN_MAX[
 * @param ticks   ticks to sleep. Sleep forever, if ticks < 0.
 */
void am_pal_sleep_ticks(int domain, int ticks);

/**
 * Sleep till the given number of ticks from the given tick domain.
 *
 * @param domain  tick domain [0, #AM_PAL_TICK_DOMAIN_MAX[
 * @param ticks   sleep till this ticks value
 */
void am_pal_sleep_till_ticks(int domain, uint32_t ticks);

/**
 * Sleep for given number of milliseconds.
 *
 * @param ms  milliseconds to sleep. Sleep forever, if ms < 0.
 */
void am_pal_sleep_ms(int ms);

/**
 * Sleep till the given number of milliseconds.
 *
 * @param ms  sleep till this milliseconds value
 */
void am_pal_sleep_till_ms(uint32_t ms);

/**
 * printf-like logging.
 *
 * Thread safe.
 *
 * @param fmt  printf-like format string
 *
 * @return printf-like return value
 */
AM_PRINTF(1, 2) int am_pal_printf(const char *fmt, ...);

/**
 * printf-like logging.
 *
 * Thread unsafe.
 *
 * @param fmt  printf-like format string
 *
 * @return printf-like return value
 */
AM_PRINTF(1, 2) int am_pal_printf_unsafe(const char *fmt, ...);

/**
 * printf-like logging + flushing.
 *
 * Thread safe.
 *
 * @param fmt  printf-like format string
 *
 * @return printf-like return value
 */
AM_PRINTF(1, 2) int am_pal_printff(const char *fmt, ...);

/**
 * vprintf-like logging.
 *
 * Thread safe.
 *
 * @param fmt   printf-like format string
 * @param args  va_list argument returned by standard va_start() call
 *
 * @return vprintf-like return value
 */
AM_PRINTF(1, 0) int am_pal_vprintf(const char *fmt, va_list args);

/**
 * vprintf-like logging + flushing.
 *
 * Thread safe.
 *
 * @param fmt   printf-like format string
 * @param args  va_list argument returned by standard va_start() call
 *
 * @return vprintf-like return value
 */
AM_PRINTF(1, 0) int am_pal_vprintff(const char *fmt, va_list args);

/** Flush am_pal_printf() intermediate buffer. */
void am_pal_flush(void);

/**
 * Block current task with am_pal_task_wait() call.
 *
 * Exists critical section before calling am_pal_task_wait() and
 * reenters it after am_pal_task_wait() returns.
 *
 * To be used as on_idle() callback for cooperative active objects library.
 */
void am_pal_on_idle(void);

/** Return the number of CPU cores. */
int am_pal_get_cpu_count(void);

/**
 * Block until all tasks are ready to run.
 *
 * Prevents using tasks before they are ready to run.
 * To be run once at the start of tasks created with am_pal_task_create() API.
 */
void am_pal_task_wait_all(void);

/**
 * Lock all tasks until am_pal_task_unlock_all() is called.
 *
 * Only used at boot-up to synchronize tasks execution.
 */
void am_pal_task_lock_all(void);

/**
 * Unlock all tasks.
 *
 * All tasks blocked on am_pal_task_wait_all() are unblocked.
 */
void am_pal_task_unlock_all(void);

/** Run all PAL tasks */
void am_pal_task_run_all(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_PAL_H_INCLUDED */
