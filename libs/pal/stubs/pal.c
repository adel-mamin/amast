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
 * Platform abstraction layer (PAL) API stubs
 */

#include <stddef.h>
#include <stdint.h>

#include "pal/pal.h"

void am_pal_crit_enter(void) {}

void am_pal_crit_exit(void) {}

int am_pal_task_create(
    const char *name,
    int priority,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    (void)name;
    (void)priority;
    (void)stack;
    (void)stack_size;
    (void)entry;
    (void)arg;

    return AM_PAL_TASK_ID_NONE;
}

void am_pal_task_notify(int task_id) { (void)task_id; }

void am_pal_task_wait(int task_id) { (void)task_id; }

uint32_t am_pal_time_get_ms(void) { return 0; }

uint32_t am_pal_time_get_tick(int domain) {
    (void)domain;
    return 0;
}

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    (void)domain;
    (void)ms;
    return 0;
}

void am_pal_sleep_ticks(int domain, int ticks) {
    (void)domain;
    (void)ticks;
}

void am_pal_sleep_ms(int ms) { (void)ms; }

int am_pal_task_get_own_id(void) { return -1; }

AM_PRINTF(1, 2) int am_pal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int rc = vprintf(fmt, args);
    va_end(args);
    return rc;
}

void am_pal_flush(void) {}
