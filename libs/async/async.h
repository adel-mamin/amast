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

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AM_ASYNC_FN_CALL_DEPTH_MAX
#define AM_ASYNC_FN_CALL_DEPTH_MAX 4
#endif

#define AM_ASYNC_STATE_INIT 0

enum am_async_rc { AM_ASYNC_RC_DONE = -2, AM_ASYNC_RC_BUSY = -1 };

struct am_async {
    struct am_async_fn {
        struct am_async_id {
            const char *file;
            int line;
        } id;
        int state;
    } fn[AM_ASYNC_FN_CALL_DEPTH_MAX];
};

static inline int am_async_find_fn_index_(
    const struct am_async *async, const char *file, int line
) {
    for (int i = 0; i < AM_COUNTOF(async->fn); ++i) {
        const struct am_async_id *id = &async->fn[i].id;
        if (!id->file || ((id->line == line) && (id->file == file))) {
            return i;
        }
    }
    AM_ASSERT(0); /* increase the value of AM_ASYNC_FN_CALL_DEPTH_MAX */
}

/* clang-format off */

#define AM_ASYNC_BEGIN(me)                                                 \
    struct am_async *am_async_ = (struct am_async *)me;                    \
    int am_async_fn_index_ = am_async_find_fn_index_(                      \
        am_async_, __FILE__, __LINE__);                                    \
    struct am_async_fn *am_async_fn_ = &am_async_->fn[am_async_fn_index_]; \
    switch (am_async_fn_->state) {                                         \
    case AM_ASYNC_STATE_INIT:                                              \
        am_async_fn_->id.file = __FILE__;                                  \
        am_async_fn_->id.line = __LINE__;                                  \
        am_async_fn_->state = __LINE__;                                    \
        /* FALLTHROUGH */                                                  \
    case __LINE__:

#define AM_ASYNC_EXIT()                                                    \
        am_async_fn_->id.file = NULL;                                      \
        am_async_fn_->id.line = am_async_fn_->state = AM_ASYNC_STATE_INIT; \
        return AM_ASYNC_RC_DONE

#define AM_ASYNC_END()                                                     \
        /* FALLTHROUGH */                                                  \
    case AM_ASYNC_RC_DONE:                                                 \
        AM_ASYNC_EXIT();                                                   \
    default:                                                               \
        AM_ASSERT(0);                                                      \
    }

#define AM_ASYNC_LABEL()                                                   \
        am_async_fn_->state = __LINE__;                                    \
        /* FALLTHROUGH */                                                  \
    case __LINE__:

#define AM_ASYNC_AWAIT(cond)                                               \
        am_async_fn_->state = __LINE__;                                    \
        /* FALLTHROUGH */                                                  \
    case __LINE__:                                                         \
        if (!(cond)) {                                                     \
            return AM_ASYNC_RC_BUSY;                                       \
        }

#define AM_ASYNC_YIELD()                                                   \
        am_async_fn_->state = __LINE__;                                    \
        return AM_ASYNC_RC_BUSY;                                           \
    case __LINE__:

/* clang-format on */

static inline void am_async_init(struct am_async *me) {
    for (int i = 0; i < AM_COUNTOF(me->fn); ++i) {
        me->fn[i].id.file = NULL;
        me->fn[i].id.line = me->fn[i].state = AM_ASYNC_STATE_INIT;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* ASYNC_H_INCLUDED */
