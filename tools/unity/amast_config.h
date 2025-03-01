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

#ifndef AMAST_CONFIG_H_INCLUDED
#define AMAST_CONFIG_H_INCLUDED

/**
 * Enable am_hsm_set_spy() API to register user callback
 * to intercept external events passed via am_hsm_dispatch() API.
 * Increases the size of struct am_hsm by the size of a function
 * pointer.
 */
#define AM_HSM_SPY

/**
 * Enable am_fsm_set_spy() API to register user callback
 * to intercept external events passed via am_fsm_dispatch() API.
 * Increases the size of struct am_fsm by the size of a function
 * pointer.
 */
#define AM_FSM_SPY

/** Enable am_assert_failure() implementation */
#define AM_ASSERT_FAILURE

/**
 * The max number of event pools.
 */
#define AM_EVENT_POOLS_NUM_MAX 3

/** The maximum number of active objects. */
#define AM_AO_NUM_MAX 64

/** HSM hierarchy maximum depth */
#define AM_HSM_HIERARCHY_DEPTH_MAX 16

/** total number of tick domains */
#define AM_PAL_TICK_DOMAIN_MAX 1

/** Maximum number of mutexes */
#define AM_PAL_MUTEX_NUM_MAX 2

#endif /* AMAST_CONFIG_H_INCLUDED */
