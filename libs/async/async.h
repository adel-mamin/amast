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

/** Init state of async function. */
#define AM_ASYNC_STATE_INIT 0

/** Async state. */
struct am_async {
    int state; /**< a line number or #AM_ASYNC_STATE_INIT constant */
};

/* clang-format off */

/**
 * Mark the beginning of async function.
 *
 * Should be called at the beginning of async function.
 *
 * @param me  pointer to the `struct am_async` managing the async state
 */
#define AM_ASYNC_BEGIN(me) {                            \
    struct am_async *am_async_ = (struct am_async *)me; \
    switch (am_async_->state) {                         \
    case AM_ASYNC_STATE_INIT:                           \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:

/**
 * Mark the end of async function.
 *
 * Resets the async state to the initial state
 * indicating that the async operation has completed.
 */
#define AM_ASYNC_EXIT()                                 \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = AM_ASYNC_STATE_INIT;         \
        return;

/**
 * Mark the end of async function block.
 *
 * Resets the async state.
 */
#define AM_ASYNC_END()                                  \
        AM_ASYNC_EXIT();                                \
    default:                                            \
        AM_ASSERT(0);                                   \
    }}

/**
 * Await a condition before proceeding.
 *
 * Checks the provided condition `cond`.
 * Returns if the condition is not met (false) and
 * on next invocation of the function the condition is evaluated again.
 *
 * Continues the function execution once the `cond` evaluates to `true`.
 *
 * @param cond  the condition to check for continuation
 */
#define AM_ASYNC_AWAIT(cond)                            \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:                                      \
        if (!(cond)) {                                  \
            return;                                     \
        }

/**
 * Yield control back to caller.
 *
 * Allows the async function to yield.
 *
 * Control resumes after this point, when the function is called again.
 */
#define AM_ASYNC_YIELD()                                \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = __LINE__;                    \
        return;                                         \
    case __LINE__:

/**
 * Check if async operation is in progress.
 *
 * @param me  the async instance pointer
 *
 * @return true   the async operation is in progress
 * @return false  the async operation is not in progress
 */
#define AM_ASYNC_IS_BUSY(me) (((struct am_async*)(me))->state != AM_ASYNC_STATE_INIT)

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
void am_async_ctor(struct am_async *me);

#ifdef __cplusplus
}
#endif

#endif /* AM_ASYNC_H_INCLUDED */
