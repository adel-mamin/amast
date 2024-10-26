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

/**
 * @file
 * Queue API.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/alignment.h"
#include "blk/blk.h"
#include "queue/queue.h"

/** Magic number to detect properly initialized queue */
#define AM_QUEUE_MAGIC1 0xCAFEABBA
/** Magic number to detect properly initialized am_queue */
#define AM_QUEUE_MAGIC2 0xDEADBEEF

void am_queue_init(
    struct am_queue *hnd, int isize, int alignment, struct am_blk *blk
) {
    AM_ASSERT(hnd);
    AM_ASSERT(isize > 0);
    AM_ASSERT(alignment > 0);
    AM_ASSERT(AM_IS_POWER_OF_TWO((unsigned)alignment));
    AM_ASSERT(blk);
    AM_ASSERT(blk->ptr);
    AM_ASSERT(blk->size > 0);

    memset(hnd, 0, sizeof(*hnd));

    void *alignedptr = AM_ALIGN_PTR_UP(blk->ptr, alignment);
    int affix = (int)((uintptr_t)alignedptr - (uintptr_t)blk->ptr);
    AM_ASSERT(affix < blk->size);
    blk->size -= affix;
    blk->ptr = alignedptr;

    hnd->isize = AM_MAX(isize, alignment);

    AM_ASSERT(blk->size >= (2 * hnd->isize));

    hnd->blk = *blk;
    hnd->magic1 = AM_QUEUE_MAGIC1;
    hnd->magic2 = AM_QUEUE_MAGIC2;
}

bool am_queue_is_empty(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    return hnd->rd == hnd->wr;
}

bool am_queue_is_full(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    return ((hnd->wr + 1) % (hnd->blk.size / hnd->isize)) == hnd->rd;
}

int am_queue_length(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    if (hnd->wr >= hnd->rd) {
        return hnd->wr - hnd->rd;
    }
    int len = (hnd->blk.size / hnd->isize) - hnd->rd - 1;
    return hnd->wr ? (len + hnd->wr) : len;
}

int am_queue_capacity(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    return (hnd->blk.size / hnd->isize) - 1;
}

int am_queue_item_size(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    return hnd->isize;
}

void *am_queue_peek_front(struct am_queue *hnd) {
    AM_ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    return (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
}

void *am_queue_peek_back(struct am_queue *hnd) {
    AM_ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    int ind = (0 == hnd->wr) ? (hnd->blk.size / hnd->isize - 1) : (hnd->wr - 1);
    return (char *)hnd->blk.ptr + ind * hnd->isize;
}

void *am_queue_pop_front(struct am_queue *hnd) {
    AM_ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    void *ptr = (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
    hnd->rd = (hnd->rd + 1) % (hnd->blk.size / hnd->isize);

    return ptr;
}

void *am_queue_pop_front_and_copy(struct am_queue *hnd, void *buf, int size) {
    AM_ASSERT(hnd);
    AM_ASSERT(buf);
    AM_ASSERT(size >= hnd->isize);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    void *popped = am_queue_pop_front(hnd);
    memcpy(buf, popped, (size_t)hnd->isize);

    return popped;
}

bool am_queue_push_back(struct am_queue *hnd, const void *ptr, int size) {
    AM_ASSERT(hnd);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= hnd->isize);

    if (am_queue_is_full(hnd)) {
        return false;
    }
    void *dst = (char *)hnd->blk.ptr + hnd->wr * hnd->isize;
    memcpy(dst, ptr, (size_t)size);
    hnd->wr = (hnd->wr + 1) % (hnd->blk.size / hnd->isize);

    return true;
}

bool am_queue_push_front(struct am_queue *hnd, const void *ptr, int size) {
    AM_ASSERT(hnd);
    AM_ASSERT(ptr);
    AM_ASSERT(size > 0);
    AM_ASSERT(size <= hnd->isize);

    if (am_queue_is_full(hnd)) {
        return false;
    }
    hnd->rd = (0 == hnd->rd) ? (hnd->blk.size / hnd->isize - 1) : (hnd->rd - 1);
    void *dst = (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
    memcpy(dst, ptr, (size_t)hnd->isize);

    return true;
}

bool am_queue_is_valid(struct am_queue *hnd) {
    AM_ASSERT(hnd);
    return (AM_QUEUE_MAGIC1 == hnd->magic1) && (AM_QUEUE_MAGIC2 == hnd->magic2);
}
