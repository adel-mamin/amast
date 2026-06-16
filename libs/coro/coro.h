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
 * Coroutine API implementation.
 * See test.c for usage examples.
 *
 * Based on the work from the following sources:
 *
 * - https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 * - https://github.com/naasking/async.h
 * - https://dunkels.com/adam/pt/
 */

#ifndef AM_CORO_H_INCLUDED
#define AM_CORO_H_INCLUDED

#include <stdbool.h>

#include "common/types.h" /* IWYU pragma: keep */

/**
 * Init value of coroutine function/block state.
 *
 * Only used by implementation.
 * Not to be used directly by user code.
 */
#define AM_CORO_STATE_INIT 0

/** Coroutine state. */
struct am_coro {
    int state; /**< a line number or #AM_CORO_STATE_INIT constant */
};

/* clang-format off */

/**
 * Mark the beginning of coroutine function/block.
 *
 * Should be called at the beginning of coroutine function/block.
 *
 * @param me  pointer to the `struct am_coro` managing the coroutine state
 */
#define AM_CORO_BEGIN(me) {                                 \
    struct am_coro *am_coro_ = (struct am_coro *)(me);      \
    switch (am_coro_->state) {                              \
    default:                                                \
        AM_ASSERT(0);                                       \
        break;                                              \
    case AM_CORO_STATE_INIT:                                \
        /* to suppress cppcheck warnings */                 \
        am_coro_->state = AM_CORO_STATE_INIT

/**
 * Mark the end of coroutine function/block.
 *
 * Should be called at the end of coroutine function/block.
 */
#define AM_CORO_END() }} do {} while (0)

/**
 * Await a condition before proceeding.
 *
 * Checks the provided condition `cond`.
 * Returns, if the condition is not met (false) and
 * on next invocation of the function the condition is evaluated again.
 *
 * Continues the function execution once the `cond` evaluates to `true`.
 *
 * @param cond  the condition to check for continuation
 */
#define AM_CORO_AWAIT(cond) do {                            \
            am_coro_->state = __LINE__;                     \
            /* FALLTHROUGH */                               \
        case __LINE__:                                      \
            if (!(cond)) {                                  \
                return AM_RC_CORO_BUSY;                     \
            }                                               \
            am_coro_->state = AM_CORO_STATE_INIT;           \
    } while (0)

/**
 * Chain an coroutine function call and evaluate its return value.
 *
 * Returns, if the coroutine function call returns
 * #AM_RC_CORO_BUSY,
 *
 * The function call is evaluated again on next invocation,
 * if #AM_RC_CORO_BUSY is returned. Otherwise the execution continues
 * on next invocation without the function call.
 *
 * @param func  the function to check the return value of
 */
#define AM_CORO_CALL(func) do {                             \
            am_coro_->state = __LINE__;                     \
            /* FALLTHROUGH */                               \
        case __LINE__: {                                    \
            enum am_rc rc_ = (func);                        \
            if (AM_RC_CORO_BUSY == rc_) {                   \
                return AM_RC_CORO_BUSY;                     \
            }                                               \
            AM_ASSERT(AM_RC_CORO_DONE == rc_);              \
            am_coro_->state = AM_CORO_STATE_INIT;           \
        }                                                   \
    } while (0)

/**
 * Yield control back to caller.
 *
 * Allows the coroutine function/block to yield.
 *
 * Control resumes after this point, when the function is called again.
 */
#define AM_CORO_YIELD() do {                                \
            am_coro_->state = __LINE__;                     \
            return AM_RC_CORO_BUSY;                         \
        case __LINE__:                                      \
            am_coro_->state = AM_CORO_STATE_INIT;           \
    } while (0)

/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Construct coroutine state.
 *
 * Sets the coroutine state to #AM_CORO_STATE_INIT
 * preparing it for use in coroutine operation.
 *
 * @param me  the coroutine state to construct
 */
static inline void am_coro_ctor(struct am_coro* me) {
    me->state = AM_CORO_STATE_INIT;
}

/**
 * Check if coroutine operation is in progress.
 *
 * @param me  the coroutine state
 *
 * @return true   the coroutine operation is in progress
 * @return false  the coroutine operation is not in progress
 */
static inline bool am_coro_is_busy(const struct am_coro* me) {
    return me->state != AM_CORO_STATE_INIT;
}

#ifdef __cplusplus
}
#endif

#endif /* AM_CORO_H_INCLUDED */
