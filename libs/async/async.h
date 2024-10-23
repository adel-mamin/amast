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

#ifndef ASYNC_H_INCLUDED
#define ASYNC_H_INCLUDED

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

#define AM_ASYNC_STATE_INIT 0

enum am_async_rc { AM_ASYNC_RC_DONE = -2, AM_ASYNC_RC_BUSY = -1 };

struct am_async {
    int state;
};

/* clang-format off */

#define AM_ASYNC_BEGIN(me)                              \
    struct am_async *am_async_ = (struct am_async *)me; \
    switch (am_async_->state) {                         \
    case AM_ASYNC_STATE_INIT:                           \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:

#define AM_ASYNC_EXIT()                                 \
        am_async_->state = AM_ASYNC_STATE_INIT;         \
        return AM_ASYNC_RC_DONE

#define AM_ASYNC_END()                                  \
        /* FALLTHROUGH */                               \
    case AM_ASYNC_RC_DONE:                              \
        AM_ASYNC_EXIT();                                \
    default:                                            \
        AM_ASSERT(0);                                   \
    }

#define AM_ASYNC_LABEL()                                \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:

#define AM_ASYNC_AWAIT(cond)                            \
        am_async_->state = __LINE__;                    \
        /* FALLTHROUGH */                               \
    case __LINE__:                                      \
        if (!(cond)) {                                  \
            return AM_ASYNC_RC_BUSY;                    \
        }

#define AM_ASYNC_YIELD()                                \
        am_async_->state = __LINE__;                    \
        return AM_ASYNC_RC_BUSY;                        \
    case __LINE__:

/* clang-format on */

static inline void am_async_init(struct am_async *me) {
    me->state = AM_ASYNC_STATE_INIT;
}

#ifdef __cplusplus
}
#endif

#endif /* ASYNC_H_INCLUDED */
