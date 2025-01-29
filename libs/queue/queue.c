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
 * Queue API implementation.
 */

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "common/macros.h"
#include "common/alignment.h"
#include "common/types.h"
#include "queue/queue.h"

void am_queue_ctor(
    struct am_queue *me, int isize, int alignment, struct am_blk *blk
) {
    AM_ASSERT(me);
    AM_ASSERT(isize > 0);
    AM_ASSERT(alignment > 0);
    AM_ASSERT(AM_IS_POW2((unsigned)alignment));
    AM_ASSERT(blk);
    AM_ASSERT(blk->ptr);
    AM_ASSERT(AM_ALIGNOF_PTR(blk->ptr) >= alignment);
    AM_ASSERT(blk->size > 0);

    memset(me, 0, sizeof(*me));

    me->isize = AM_ALIGN_SIZE(isize, alignment);

    AM_ASSERT(blk->size >= me->isize);

    me->blk = *blk;
}

void am_queue_dtor(struct am_queue *me) {
    AM_ASSERT(me);
    memset(me, 0, sizeof(*me));
}

bool am_queue_is_empty(const struct am_queue *me) {
    AM_ASSERT(me);
    return (me->rd == me->wr) && !me->full;
}

bool am_queue_is_full(const struct am_queue *me) {
    AM_ASSERT(me);
    return me->full;
}

int am_queue_length(const struct am_queue *me) {
    AM_ASSERT(me);
    if (me->wr > me->rd) {
        return me->wr - me->rd;
    }
    if (me->wr < me->rd) {
        return (me->blk.size / me->isize) - me->rd + me->wr;
    }
    if (me->full) {
        return (me->blk.size / me->isize);
    }
    return 0;
}

int am_queue_capacity(const struct am_queue *me) {
    AM_ASSERT(me);
    return me->blk.size / me->isize;
}

int am_queue_item_size(const struct am_queue *me) {
    AM_ASSERT(me);
    return me->isize;
}

void *am_queue_peek_front(struct am_queue *me) {
    AM_ASSERT(me);

    if (am_queue_is_empty(me)) {
        return NULL;
    }
    return (char *)me->blk.ptr + me->rd * me->isize;
}

void *am_queue_peek_back(struct am_queue *me) {
    AM_ASSERT(me);

    if (am_queue_is_empty(me)) {
        return NULL;
    }
    int ind = me->wr ? (me->wr - 1) : (me->blk.size / me->isize - 1);
    return (char *)me->blk.ptr + ind * me->isize;
}

void *am_queue_pop_front(struct am_queue *me) {
    AM_ASSERT(me);

    if (am_queue_is_empty(me)) {
        return NULL;
    }
    void *ptr = (char *)me->blk.ptr + me->rd * me->isize;
    me->rd = (me->rd + 1) % (me->blk.size / me->isize);
    me->full = 0;

    return ptr;
}

void *am_queue_pop_front_and_copy(struct am_queue *me, void *buf, int size) {
    AM_ASSERT(me);
    AM_ASSERT(buf);
    AM_ASSERT(size >= me->isize);

    if (am_queue_is_empty(me)) {
        return NULL;
    }
    void *popped = am_queue_pop_front(me);
    memcpy(buf, popped, (size_t)me->isize);

    return popped;
}

bool am_queue_push_back(struct am_queue *me, const void *ptr, int size) {
    AM_ASSERT(me);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= me->isize);

    if (am_queue_is_full(me)) {
        return false;
    }
    void *dst = (char *)me->blk.ptr + me->wr * me->isize;
    memcpy(dst, ptr, (size_t)size);
    me->wr = (me->wr + 1) % (me->blk.size / me->isize);
    if (me->wr == me->rd) {
        me->full = 1;
    }
    return true;
}

bool am_queue_push_front(struct am_queue *me, const void *ptr, int size) {
    AM_ASSERT(me);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= me->isize);

    if (am_queue_is_full(me)) {
        return false;
    }
    me->rd = me->rd ? (me->rd - 1) : (me->blk.size / me->isize - 1);
    void *dst = (char *)me->blk.ptr + me->rd * me->isize;
    memcpy(dst, ptr, (size_t)me->isize);
    if (me->wr == me->rd) {
        me->full = 1;
    }
    return true;
}
