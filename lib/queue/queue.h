/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018,2019,2022,2024 Adel Mamin
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

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Queue handler */
struct queue {
    int isize;       /**< item size [bytes] */
    int rd;          /**< read index */
    int wr;          /**< write index */
    struct blk blk;  /**< queue memory block */
    unsigned magic1; /**< magic number1 */
    unsigned magic2; /**< magic number2 */
};

/**
 * Queue is empty predicate.
 * @param hnd     queue handler
 * @retval true   queue is empty
 * @retval false  queue is not empty
 */
bool queue_is_empty(struct queue *hnd);

/**
 * Queue is full predicate.
 * @param hnd     the queue
 * @retval true   queue is full
 * @retval false  queue is not empty
 */
bool queue_is_full(struct queue *hnd);

/**
 * Return how many items are in the queue.
 * @param hnd  the queue
 * @return number of queued items
 */
int queue_length(struct queue *hnd);

/**
 * Return queue capacity.
 * @param hnd  the queue
 * @return queue capacity
 */
int queue_capacity(struct queue *hnd);

/**
 * Return queue item size.
 * @param hnd  the queue
 * @return queue item size [bytes]
 */
int queue_item_size(struct queue *hnd);

/**
 * Queue initialization with memory a block.
 * @param hnd        the queue
 * @param isize      item size [bytes]
 *                   The queue will only support the items of this size.
 * @param alignment  the queue alignment [bytes]
 *                   Can be set to DEFAULT_ALIGNMENT.
 * @param blk        the memory block
 */
void queue_init(struct queue *hnd, int isize, int alignment, struct blk *blk);

/**
 * Checks if the queue was properly initialized.
 * @param hnd     the queue
 * @return true   the initialization was done
 * @return false  the initialization was not done
 */
bool queue_is_valid(struct queue *hnd);

/**
 * Pop an item from the front (head) of the queue.
 * Takes O(1) to complete.
 * @param hnd  the queue
 * @return The popped item. The memory is owned by the queue
 *         Do not free it!
 *         If queue is empty then NULL is returned.
 */
void *queue_pop_front(struct queue *hnd);

/**
 * Pop an item from the front (head) of the queue to the provided buffer.
 * Takes O(1) to complete.
 * @param queue  the queue
 * @param buf    the popped item is compied here.
 * @param size   the byte size of buf.
 * @return The popped item. NULL if queue was empty.
 */
void *queue_pop_front_and_copy(struct queue *hnd, void *buf, int size);

/**
 * Peek an item from the front (head) of the queue.
 * Takes O(1) to complete.
 * @param hnd  the queue
 * @return The peeked item. The memory is owned by the queue.
 *         Do not free it!
 *         If queue is empty, then NULL is returned.
 */
void *queue_peek_head(struct queue *hnd);

/**
 * Peek an item from the back (tail) of the queue.
 * Takes O(1) to complete.
 * @param hnd  the queue
 * @return The peeked item. The memory is owned by the queue.
 *         Do not free it!
 *         If queue is empty, then NULL is returned.
 */
void *queue_peek_back(struct queue *hnd);

/**
 * Push an item to the front (head) of the queue.
 * Takes O(1) to complete.
 * @param hnd   the queue
 * @param ptr   the new queue item
 *              The API copies the content of ptr.
 * @param size  the size of the new queue item in bytes
 *              Must be <= than queue item size.
 * @retval true   success
 * @retval false  failure
 */
bool queue_push_front(struct queue *hnd, const void *ptr, int size);

/**
 * Push an item to the back (tail) of the queue.
 * Takes O(1) to complete.
 * @param hnd   the queue
 * @param ptr   the new queue item
 *              The API copies the content of ptr.
 * @param size  the size of the new queue item in bytes
 *              Must be <= than queue item size.
 * @retval true   success
 * @retval false  failure
 */
bool queue_push_back(struct queue *hnd, const void *ptr, int size);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_INCLUDED */
