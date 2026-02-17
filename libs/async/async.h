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
 * Async/await API implementation.
 * See test.c for usage examples.
 *
 * Based on the work from the following sources:
 *
 * - https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 * - https://github.com/naasking/async.h
 * - https://dunkels.com/adam/pt/
 */

#ifndef AM_ASYNC_H_INCLUDED
#define AM_ASYNC_H_INCLUDED

#include <stdbool.h>

#include "common/types.h" /* IWYU pragma: keep */

/**
 * Init value of async function/block state.
 *
 * Only used by implementation.
 * Not to be used directly by user code.
 */
#define AM_ASYNC_STATE_INIT 0

/** Async state. */
struct am_async {
    int state; /**< a line number or #AM_ASYNC_STATE_INIT constant */
};

/* clang-format off */

/**
 * Mark the beginning of async function/block.
 *
 * Should be called at the beginning of async function/block.
 *
 * @param me  pointer to the `struct am_async` managing the async state
 */
#define AM_ASYNC_BEGIN(me) {                            \
    struct am_async *am_async_ = (struct am_async *)me; \
    switch (am_async_->state) {                         \
    default:                                            \
        AM_ASSERT(0);                                   \
        break;                                          \
    case AM_ASYNC_STATE_INIT:                           \
        /* to suppress cppcheck warnings */             \
        am_async_->state = AM_ASYNC_STATE_INIT

/**
 * Mark the end of async function/block.
 *
 * Should be called at the end of async function/block.
 */
#define AM_ASYNC_END() }} do {} while (0)

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
#define AM_ASYNC_AWAIT(cond)                            \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:                                      \
        if (!(cond)) {                                  \
            return AM_RC_BUSY;                          \
        }                                               \
        am_async_->state = AM_ASYNC_STATE_INIT

/**
 * Chain an async function call and evaluate its return value.
 *
 * Returns, if the async function call returns #AM_RC_BUSY.
 * The function call is evaluated again on next invocation.
 *
 * @param call  the function call to check the return value of
 */
#define AM_ASYNC_CHAIN(call)                            \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:                                      \
        do {                                            \
            enum am_rc rc_ = (call);                    \
            if (AM_RC_BUSY == rc_) {                    \
                return rc_;                             \
            }                                           \
            am_async_->state = AM_ASYNC_STATE_INIT;     \
            if ((AM_RC_TRAN == rc_) ||                  \
               (AM_RC_TRAN_REDISPATCH == rc_)) {        \
                return rc_;                             \
            }                                           \
            AM_ASSERT(rc_ == AM_RC_DONE);               \
        } while (0)

/**
 * Yield control back to caller.
 *
 * Allows the async function/block to yield.
 *
 * Control resumes after this point, when the function is called again.
 */
#define AM_ASYNC_YIELD()                                \
        am_async_->state = __LINE__;                    \
        return AM_RC_BUSY;                              \
    case __LINE__:                                      \
        am_async_->state = AM_ASYNC_STATE_INIT

/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Construct async state.
 *
 * Sets the async state to #AM_ASYNC_STATE_INIT
 * preparing it for use in async operation.
 *
 * @param me  the async state to construct
 */
static inline void am_async_ctor(struct am_async *me) {
    me->state = AM_ASYNC_STATE_INIT;
}

/**
 * Check if async operation is in progress.
 *
 * @param me  the async state
 *
 * @return true   the async operation is in progress
 * @return false  the async operation is not in progress
 */
static inline bool am_async_is_busy(const struct am_async *me) {
    return me->state != AM_ASYNC_STATE_INIT;
}

#ifdef __cplusplus
}
#endif

#endif /* AM_ASYNC_H_INCLUDED */
