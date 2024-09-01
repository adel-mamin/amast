/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2022,2024 Adel Mamin
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
#define QUEUE_MAGIC1 0xCAFEABBA
/** Magic number to detect properly initialized queue */
#define QUEUE_MAGIC2 0xDEADBEEF

void queue_init(struct queue *hnd, int isize, int alignment, struct blk *blk) {
    ASSERT(hnd);
    ASSERT(isize > 0);
    ASSERT(alignment > 0);
    ASSERT(IS_POWER_OF_TWO(alignment));
    ASSERT(blk);
    ASSERT(blk->ptr);
    ASSERT(blk->size > 0);

    memset(hnd, 0, sizeof(*hnd));

    void *alignedptr = ALIGN_PTR_UP(blk->ptr, alignment);
    int affix = (int)((uintptr_t)alignedptr - (uintptr_t)blk->ptr);
    ASSERT(affix < blk->size);
    blk->size -= affix;
    blk->ptr = alignedptr;

    hnd->isize = MAX(isize, alignment);

    ASSERT(blk->size >= (2 * hnd->isize));

    hnd->blk = *blk;
    hnd->magic1 = QUEUE_MAGIC1;
    hnd->magic2 = QUEUE_MAGIC2;
}

bool queue_is_empty(struct queue *hnd) {
    ASSERT(hnd);
    return hnd->rd == hnd->wr;
}

bool queue_is_full(struct queue *hnd) {
    ASSERT(hnd);
    return ((hnd->wr + 1) % (hnd->blk.size / hnd->isize)) == hnd->rd;
}

int queue_length(struct queue *hnd) {
    ASSERT(hnd);
    if (hnd->wr >= hnd->rd) {
        return hnd->wr - hnd->rd;
    }
    int len = (hnd->blk.size / hnd->isize) - hnd->rd - 1;
    return hnd->wr ? (len + hnd->wr) : len;
}

int queue_capacity(struct queue *hnd) {
    ASSERT(hnd);
    return (hnd->blk.size / hnd->isize) - 1;
}

int queue_isize(struct queue *hnd) {
    ASSERT(hnd);
    return hnd->isize;
}

void *queue_peek_front(struct queue *hnd) {
    ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    return (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
}

void *queue_peek_back(struct queue *hnd) {
    ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    int ind = (0 == hnd->wr) ? (hnd->blk.size / hnd->isize - 1) : (hnd->wr - 1);
    return (char *)hnd->blk.ptr + ind * hnd->isize;
}

void *queue_pop_front(struct queue *hnd) {
    ASSERT(hnd);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    void *ptr = (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
    hnd->rd = (hnd->rd + 1) % (hnd->blk.size / hnd->isize);

    return ptr;
}

void *queue_pop_front_and_copy(struct queue *hnd, void *buf, int size) {
    ASSERT(hnd);
    ASSERT(buf);
    ASSERT(size >= hnd->isize);

    if (hnd->rd == hnd->wr) {
        return NULL;
    }
    void *popped = queue_pop_front(hnd);
    memcpy(buf, popped, (size_t)hnd->isize);

    return popped;
}

bool queue_push_back(struct queue *hnd, const void *ptr, int size) {
    ASSERT(hnd);
    ASSERT(ptr);
    ASSERT(size > 0);
    ASSERT(size <= hnd->isize);

    if (queue_is_full(hnd)) {
        return false;
    }
    void *dst = (char *)hnd->blk.ptr + hnd->wr * hnd->isize;
    memcpy(dst, ptr, (size_t)size);
    hnd->wr = (hnd->wr + 1) % (hnd->blk.size / hnd->isize);

    return true;
}

bool queue_push_front(struct queue *hnd, const void *ptr, int size) {
    ASSERT(hnd);
    ASSERT(ptr);
    ASSERT(size > 0);
    ASSERT(size <= hnd->isize);

    if (queue_is_full(hnd)) {
        return false;
    }
    hnd->rd = (0 == hnd->rd) ? (hnd->blk.size / hnd->isize - 1) : (hnd->rd - 1);
    void *dst = (char *)hnd->blk.ptr + hnd->rd * hnd->isize;
    memcpy(dst, ptr, (size_t)hnd->isize);

    return true;
}

bool queue_is_valid(struct queue *hnd) {
    ASSERT(hnd);
    return (QUEUE_MAGIC1 == hnd->magic1) && (QUEUE_MAGIC2 == hnd->magic2);
}