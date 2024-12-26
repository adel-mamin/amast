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
 * Platform abstraction layer (PAL) API
 */

#ifndef AM_PAL_H_INCLUDED
#define AM_PAL_H_INCLUDED

#include <stdint.h>

/** Maximum number of PAL tasks */
#define AM_PAL_TASK_NUM_MAX 64

/** Invalid task ID */
#define AM_PAL_TASK_ID_NONE 0

/** Main task ID */
#define AM_PAL_TASK_ID_MAIN -1

/** Default tick domain */
#define AM_PAL_TICK_DOMAIN_DEFAULT 0

#ifndef AM_PAL_TICK_DOMAIN_MAX
#define AM_PAL_TICK_DOMAIN_MAX 1 /** total number of tick domains */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** PAL constructor. */
void am_pal_ctor(void);

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
 * @return  unique mutex ID
 */
int am_pal_mutex_create(void);

/**
 * Lock mutex.
 *
 * @param mutex  mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_lock(int mutex);

/**
 * Unlock mutex.
 *
 * @param mutex  mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_unlock(int mutex);

/**
 * Destroy mutex.
 *
 * @param mutex  mutex ID returned by am_pal_mutex_create()
 */
void am_pal_mutex_destroy(int mutex);

/**
 * Create task.
 *
 * @param name        human readable task name. Not copied.
 *                    Must remain valid after the call.
 * @param priority    task priority (>=0)
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
    int priority,
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
 * @param task  task ID returned by am_pal_task_create()
 */
void am_pal_task_wait(int task);

/**
 * Return task own ID.
 *
 * @return  task ID
 */
int am_pal_task_own_id(void);

/**
 * Get current time in milliseconds.
 *
 * @return  current time [ms]
 */
uint32_t am_pal_time_get_ms(void);

/**
 * Get current time in ticks.
 *
 * @param domain  tick domain [0..AM_PAL_TICK_DOMAIN_MAX]
 * @return  current time [tick]
 */
uint32_t am_pal_time_get_tick(int domain);

/**
 * Convert ms to ticks for the given tick domain.
 *
 * @param domain  tick domain [0..AM_PAL_TICK_DOMAIN_MAX]
 * @param ms      milliseconds to convert
 *
 * @return  time [tick]
 */
uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms);

/**
 * Convert ticks from the given tick domain to milliseconds.
 *
 * @param domain  tick domain [0..AM_PAL_TICK_DOMAIN_MAX]
 * @param tick    ticks to convert
 *
 * @return time [ms]
 */
uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick);

/**
 * Sleep for given number of ticks from the given tick domain.
 *
 * @param domain  tick domain [0..AM_PAL_TICK_DOMAIN_MAX]
 * @param tick    ticks to sleep
 */
void am_pal_sleep_ticks(int domain, int ticks);

/**
 * Sleep till the given number of ticks from the given tick domain.
 *
 * @param domain  tick domain [0..AM_PAL_TICK_DOMAIN_MAX]
 * @param tick    sleep till this ticks value
 */
void am_pal_sleep_till_ticks(int domain, uint32_t ticks);

/**
 * Sleep for given number of milliseconds.
 *
 * @param ms  milliseconds to sleep
 */
void am_pal_sleep_ms(int ms);

/**
 * Sleep till the given number of milliseconds.
 *
 * @param ms    sleep till this milliseconds value
 */
void am_pal_sleep_till_ms(uint32_t ms);

/**
 * printf like logging.
 *
 * @param fmt  printf-like format string
 *
 * @return  printf-like return value
 */
AM_PRINTF(1, 2) int am_pal_printf(const char *fmt, ...);

/** Flush am_pal_printf() intermediate buffer. */
void am_pal_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_PAL_H_INCLUDED */
