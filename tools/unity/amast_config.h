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
/* #define AM_HSM_SPY */

/**
 * Enable am_fsm_set_spy() API to register user callback
 * to intercept external events passed via am_fsm_dispatch() API.
 * Increases the size of struct am_fsm by the size of a function
 * pointer.
 */
/* #define AM_FSM_SPY */

/** Enable am_assert_failure() implementation */
/* #define AM_ASSERT_FAILURE */

#endif /* AMAST_CONFIG_H_INCLUDED */
