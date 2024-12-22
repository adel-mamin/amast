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

void am_pal_ctor(void);
void am_pal_dtor(void);

void am_pal_crit_enter(void);
void am_pal_crit_exit(void);

int am_pal_mutex_create(void);
void am_pal_mutex_lock(int mutex);
void am_pal_mutex_unlock(int mutex);
void am_pal_mutex_destroy(int mutex);

int am_pal_task_create(
    const char *name,
    int priority,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
);

void am_pal_task_notify(int task_id);
void am_pal_task_wait(int task_id);
int am_pal_task_own_id(void);

uint32_t am_pal_time_get_ms(void);
uint32_t am_pal_time_get_tick(int domain);
uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms);
uint32_t am_pal_time_get_ms_from_tick(int domain, uint32_t tick);

void am_pal_sleep_ticks(int domain, int ticks);
void am_pal_sleep_till_ticks(int domain, uint32_t ticks);

void am_pal_sleep_ms(int ms);
void am_pal_sleep_till_ms(uint32_t ms);

AM_PRINTF(1, 2) int am_pal_printf(const char *fmt, ...);
void am_pal_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_PAL_H_INCLUDED */
