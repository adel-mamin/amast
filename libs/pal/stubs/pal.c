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

/* Platform abstraction layer (PAL) API stubs */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "common/compiler.h" /* IWYU pragma: keep */
#include "pal/pal.h"

void *am_pal_ctor(void *arg) {
    (void)arg;
    return NULL;
}

void am_pal_dtor(void) {}

void am_on_idle(void) {}

void am_crit_enter(void) {}

void am_crit_exit(void) {}

int am_task_create(
    const char *name,
    int prio,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    (void)name;
    (void)prio;
    (void)stack;
    (void)stack_size;
    (void)entry;
    (void)arg;

    return AM_TASK_ID_NONE;
}

void am_task_notify(int task_id) { (void)task_id; }

void am_task_wait(int task_id) { (void)task_id; }

int am_mutex_create(void) { return 0; }

void am_mutex_lock(int mutex) { (void)mutex; }

void am_mutex_unlock(int mutex) { (void)mutex; }

void am_mutex_destroy(int mutex) { (void)mutex; }

static uint32_t am_pal_ms = 0;

void am_time_set_ms(uint32_t ms);

void am_time_set_ms(uint32_t ms) { am_pal_ms = ms; }

uint32_t am_time_get_ms(void) { return am_pal_ms; }

uint32_t am_time_get_tick(int domain) {
    (void)domain;
    return 0;
}

uint32_t am_time_get_tick_from_ms(int domain, uint32_t ms) {
    (void)domain;
    (void)ms;
    return 0;
}

uint32_t am_time_get_ms_from_tick(int domain, uint32_t tick) {
    (void)domain;
    (void)tick;
    return 0;
}

void am_sleep_ticks(int domain, int ticks) {
    (void)domain;
    (void)ticks;
}

void am_sleep_till_ticks(int domain, uint32_t ticks) {
    (void)domain;
    (void)ticks;
}

void am_sleep_ms(int ms) { (void)ms; }

int am_task_get_own_id(void) { return -1; }

int am_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_printf_unsafe(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

int am_printff(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    am_pal_flush();
    return rc;
}

int am_vprintf(const char *fmt, va_list args) { return vprintf(fmt, args); }

int am_vprintff(const char *fmt, va_list args) {
    int rc = vprintf(fmt, args);
    am_pal_flush();
    return rc;
}

void am_pal_flush(void) {}

int am_get_cpu_count(void) { return 1; }

void am_task_run_all(void) {}

void am_task_lock_all(void) {}

void am_task_unlock_all(void) {}

void am_task_wait_all(void) {}
