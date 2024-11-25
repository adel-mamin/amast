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

#ifndef AM_ASYNC_H_INCLUDED
#define AM_ASYNC_H_INCLUDED

/**
 * Async/await API implementation.
 * See test.c for usage examples.
 *
 * Based on the work from the following sources:
 *
 * https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 * https://github.com/naasking/async.h
 * https://dunkels.com/adam/pt/
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Init state of async function */
#define AM_ASYNC_STATE_INIT 0

/** Async function return codes */
enum am_async_rc {
    AM_ASYNC_RC_DONE = -2, /**< Async function is done */
    AM_ASYNC_RC_BUSY = -1  /**< Async function is busy */
};

/** Async state */
struct am_async {
    int state; /**< a line number or #AM_ASYNC_ constant */
};

/* clang-format off */

/**
 * Mark the beginning of async function block.
 *
 * Should be called at the beginning of an async function.
 *
 * @param me  pointer to the `struct am_async` managing the async state
 */
#define AM_ASYNC_BEGIN(me)                              \
    struct am_async *am_async_ = (struct am_async *)me; \
    switch (am_async_->state) {                         \
    case AM_ASYNC_STATE_INIT:                           \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:

/**
 * Mark the end of async function and return completion.
 *
 * Resets the async state to the initial state and returns
 * #AM_ASYNC_RC_DONE, indicating that the async operation has completed.
 */
#define AM_ASYNC_EXIT()                                 \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = AM_ASYNC_STATE_INIT;         \
        return AM_ASYNC_RC_DONE

/**
 * Mark the end of an async function block and handle any unexpected states.
 *
 * Ensure proper handling for completed and unexpected states.
 * Call `AM_ASYNC_EXIT()` internally to reset the async state.
 */
#define AM_ASYNC_END()                                  \
        /* FALLTHROUGH */                               \
    case AM_ASYNC_RC_DONE:                              \
        AM_ASYNC_EXIT();                                \
    default:                                            \
        AM_ASSERT(0);                                   \
    }

/**
 * Set a label in the async function.
 *
 * Store the current line number in the `state` field,
 * enabling the async function to resume from this point.
 */
#define AM_ASYNC_LABEL()                                \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:


/**
 * Await a condition before proceeding.
 *
 * Check the provided condition `cond`. If the condition
 * is not met - return #AM_ASYNC_RC_BUSY, allowing
 * the caller to re-enter the function later.
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
            return AM_ASYNC_RC_BUSY;                    \
        }

/**
 * Yield control back to the caller.
 *
 * Allow the async function to yield, returning
 * #AM_ASYNC_RC_BUSY to signal that the operation is not yet complete.
 * Control resumes after this point, when the function is called again.
 */
#define AM_ASYNC_YIELD()                                \
        /* to suppress cppcheck warnings */             \
        (void)am_async_->state;                         \
        am_async_->state = __LINE__;                    \
        return AM_ASYNC_RC_BUSY;                        \
    case __LINE__:

/* clang-format on */

/**
 * Initialize the `struct am_async`.
 *
 * Set the async structure's state to #AM_ASYNC_STATE_INIT
 * preparing it for use in an async operation.
 *
 * @param me  pointer to the `struct am_async` structure to initialize
 */
static inline void am_async_init(struct am_async *me) {
    me->state = AM_ASYNC_STATE_INIT;
}

#ifdef __cplusplus
}
#endif

#endif /* AM_ASYNC_H_INCLUDED */
