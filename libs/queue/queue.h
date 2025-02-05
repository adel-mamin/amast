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
 * Queue API.
 */

#ifndef AM_QUEUE_H_INCLUDED
#define AM_QUEUE_H_INCLUDED

#include <stdbool.h>
#include "common/types.h"

/** Queue handler. */
struct am_queue {
    int isize;         /**< item size [bytes] */
    int rd;            /**< read index */
    int wr;            /**< write index */
    int nfree;         /**< number of free slots */
    int nfree_min;     /**< minimum number of free slots observed so far */
    struct am_blk blk; /**< queue memory block */
    unsigned full : 1; /**< queue is full */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Queue construction with a memory block.
 *
 * @param queue      the queue
 * @param isize      item size [bytes].
 *                   The queue will only support the items of this size.
 * @param alignment  the queue alignment [bytes].
 *                   Can be set to #AM_ALIGN_MAX.
 * @param blk        the memory block
 */
void am_queue_ctor(
    struct am_queue *queue, int isize, int alignment, struct am_blk *blk
);

/**
 * Queue destruction.
 *
 * @param queue  the queue
 */
void am_queue_dtor(struct am_queue *queue);

/**
 * Check if queue is empty.
 *
 * @param queue  the queue
 *
 * @retval true   queue is empty
 * @retval false  queue is not empty
 */
bool am_queue_is_empty(const struct am_queue *queue);

/**
 * Check if queue is full.
 *
 * @param queue  the queue
 *
 * @retval true   queue is full
 * @retval false  queue is not full
 */
bool am_queue_is_full(const struct am_queue *queue);

/**
 * Return how many items are in queue.
 *
 * @param queue  the queue
 *
 * @return number of queued items
 */
int am_queue_get_nbusy(const struct am_queue *queue);

/**
 * Return queue capacity.
 *
 * @param queue  the queue
 *
 * @return queue capacity
 */
int am_queue_get_capacity(const struct am_queue *queue);

/**
 * Return queue item size.
 *
 * @param queue  the queue
 *
 * @return queue item size [bytes]
 */
int am_queue_item_size(const struct am_queue *queue);

/**
 * Pop an item from the front (head) of queue.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 *
 * @return The popped item. The memory is owned by the queue.
 *         Do not free it!
 *         If queue is empty, then NULL is returned.
 */
void *am_queue_pop_front(struct am_queue *queue);

/**
 * Pop an item from the front (head) of queue to the provided buffer.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 * @param buf    the popped item is copied here
 * @param size   the byte size of buf
 *
 * @return The popped item. NULL if queue was empty.
 */
void *am_queue_pop_front_and_copy(struct am_queue *queue, void *buf, int size);

/**
 * Peek an item from the front (head) of queue.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 *
 * @return The peeked item. The memory is owned by the queue.
 *         Do not free it!
 *         If queue is empty, then NULL is returned.
 */
void *am_queue_peek_front(struct am_queue *queue);

/**
 * Peek an item from the back (tail) of queue.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 *
 * @return The peeked item. The memory is owned by the queue.
 *         Do not free it!
 *         If queue is empty, then NULL is returned.
 */
void *am_queue_peek_back(struct am_queue *queue);

/**
 * Push an item to the front (head) of queue.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 * @param ptr    the new queue item.
 *               The API copies the content of ptr.
 * @param size   the size of the new queue item in bytes.
 *               Must be <= than queue item size.
 *
 * @retval true   success
 * @retval false  failure
 */
bool am_queue_push_front(struct am_queue *queue, const void *ptr, int size);

/**
 * Push an item to the back (tail) of queue.
 *
 * Takes O(1) to complete.
 *
 * @param queue  the queue
 * @param ptr    the new queue item.
 *               The API copies the content of ptr.
 * @param size   the size of the new queue item in bytes.
 *               Must be <= than queue item size.
 *
 * @retval true   success
 * @retval false  failure
 */
bool am_queue_push_back(struct am_queue *queue, const void *ptr, int size);

/**
 * Get minimum number of free slots available in queue.
 *
 * @param queue  the queue
 *
 * @return the number of free slots available now
 */
int am_queue_get_nfree(const struct am_queue *queue);

/**
 * Get minimum number of free slots available so far in queue.
 *
 * Could be used to assess the usage of the queue.
 *
 * @param queue  the queue
 *
 * @return the minimum number of slots available so far
 */
int am_queue_get_nfree_min(const struct am_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* AM_QUEUE_H_INCLUDED */
