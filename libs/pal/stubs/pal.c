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
 * Platform abstraction layer (PAL) API stubs
 */

#ifdef AMAST_PAL_STUBS

#include <stddef.h>
#include <stdint.h>

#include "pal/pal.h"

void am_pal_crit_enter(void) {}

void am_pal_crit_exit(void) {}

void *am_pal_task_create(
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

    return NULL;
}

void am_pal_task_notify(void *task) { (void)task; }

void am_pal_task_wait(void *task) { (void)task; }

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

#endif /* AMAST_PAL_STUBS */
