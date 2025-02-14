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
    struct am_queue *queue, int isize, int alignment, struct am_blk *blk
) {
    AM_ASSERT(queue);
    AM_ASSERT(isize > 0);
    AM_ASSERT(alignment > 0);
    AM_ASSERT(AM_IS_POW2((unsigned)alignment));
    AM_ASSERT(blk);
    AM_ASSERT(blk->ptr);
    AM_ASSERT(AM_ALIGNOF_PTR(blk->ptr) >= alignment);
    AM_ASSERT(blk->size > 0);

    memset(queue, 0, sizeof(*queue));

    queue->isize = AM_ALIGN_SIZE(isize, alignment);

    AM_ASSERT(blk->size >= queue->isize);

    queue->blk = *blk;
    queue->nfree = queue->nfree_min = queue->blk.size / queue->isize;
    queue->ctor_called = true;
}

void am_queue_dtor(struct am_queue *queue) {
    AM_ASSERT(queue);
    memset(queue, 0, sizeof(*queue));
}

bool am_queue_is_empty(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return (queue->rd == queue->wr) && !queue->full;
}

bool am_queue_is_full(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->full;
}

int am_queue_get_nbusy(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return am_queue_get_capacity(queue) - queue->nfree;
}

int am_queue_get_capacity(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->blk.size / queue->isize;
}

int am_queue_item_size(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->isize;
}

void *am_queue_peek_front(struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    if (am_queue_is_empty(queue)) {
        return NULL;
    }
    return (char *)queue->blk.ptr + queue->rd * queue->isize;
}

void *am_queue_peek_back(struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    if (am_queue_is_empty(queue)) {
        return NULL;
    }
    int ind =
        queue->wr ? (queue->wr - 1) : (queue->blk.size / queue->isize - 1);
    return (char *)queue->blk.ptr + ind * queue->isize;
}

void *am_queue_pop_front(struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    if (am_queue_is_empty(queue)) {
        return NULL;
    }
    void *ptr = (char *)queue->blk.ptr + queue->rd * queue->isize;
    queue->rd = (queue->rd + 1) % (queue->blk.size / queue->isize);
    queue->full = 0;
    ++queue->nfree;

    return ptr;
}

void *am_queue_pop_front_and_copy(struct am_queue *queue, void *buf, int size) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    AM_ASSERT(buf);
    AM_ASSERT(size >= queue->isize);

    if (am_queue_is_empty(queue)) {
        return NULL;
    }
    void *popped = am_queue_pop_front(queue);
    memcpy(buf, popped, (size_t)queue->isize);
    ++queue->nfree;

    return popped;
}

bool am_queue_push_back(struct am_queue *queue, const void *ptr, int size) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= queue->isize);

    if (am_queue_is_full(queue)) {
        return false;
    }
    void *dst = (char *)queue->blk.ptr + queue->wr * queue->isize;
    memcpy(dst, ptr, (size_t)size);
    queue->wr = (queue->wr + 1) % (queue->blk.size / queue->isize);
    if (queue->wr == queue->rd) {
        queue->full = 1;
    }
    AM_ASSERT(queue->nfree);
    --queue->nfree;
    queue->nfree_min = AM_MIN(queue->nfree, queue->nfree_min);
    return true;
}

bool am_queue_push_front(struct am_queue *queue, const void *ptr, int size) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= queue->isize);

    if (am_queue_is_full(queue)) {
        return false;
    }
    queue->rd =
        queue->rd ? (queue->rd - 1) : (queue->blk.size / queue->isize - 1);
    void *dst = (char *)queue->blk.ptr + queue->rd * queue->isize;
    memcpy(dst, ptr, (size_t)queue->isize);
    if (queue->wr == queue->rd) {
        queue->full = 1;
    }
    AM_ASSERT(queue->nfree);
    --queue->nfree;
    queue->nfree_min = AM_MIN(queue->nfree, queue->nfree_min);
    return true;
}

int am_queue_get_nfree(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->nfree;
}

int am_queue_get_nfree_min(const struct am_queue *queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->nfree_min;
}
