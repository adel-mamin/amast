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

/** queue handler */
struct a1queue {
    int isize;       /**< item size [bytes] */
    int rd;          /**< read index */
    int wr;          /**< write index */
    struct blk blk;  /**< a1queue memory block */
    unsigned magic1; /**< magic number1 */
    unsigned magic2; /**< magic number2 */
};

/**
 * A1queue is empty predicate.
 * @param hnd     a1queue handler
 * @retval true   a1queue is empty
 * @retval false  a1queue is not empty
 */
bool a1queue_is_empty(struct a1queue *hnd);

/**
 * A1queue is full predicate.
 * @param hnd     the a1queue
 * @retval true   a1queue is full
 * @retval false  a1queue is not empty
 */
bool a1queue_is_full(struct a1queue *hnd);

/**
 * Return how many items are in the a1queue.
 * @param hnd  the a1queue
 * @return number of a1queued items
 */
int a1queue_length(struct a1queue *hnd);

/**
 * Return a1queue capacity.
 * @param hnd  the a1queue
 * @return a1queue capacity
 */
int a1queue_capacity(struct a1queue *hnd);

/**
 * Return a1queue item size.
 * @param hnd  the a1queue
 * @return a1queue item size [bytes]
 */
int a1queue_item_size(struct a1queue *hnd);

/**
 * A1queue initialization with memory a block.
 * @param hnd        the a1queue
 * @param isize      item size [bytes]
 *                   The a1queue will only support the items of this size.
 * @param alignment  the a1queue alignment [bytes]
 *                   Can be set to DEFAULT_ALIGNMENT.
 * @param blk        the memory block
 */
void a1queue_init(
    struct a1queue *hnd, int isize, int alignment, struct blk *blk
);

/**
 * Checks if the a1queue was properly initialized.
 * @param hnd     the a1queue
 * @return true   the initialization was done
 * @return false  the initialization was not done
 */
bool a1queue_is_valid(struct a1queue *hnd);

/**
 * Pop an item from the front (head) of the a1queue.
 * Takes O(1) to complete.
 * @param hnd  the a1queue
 * @return The popped item. The memory is owned by the a1queue
 *         Do not free it!
 *         If a1queue is empty then NULL is returned.
 */
void *a1queue_pop_front(struct a1queue *hnd);

/**
 * Pop an item from the front (head) of the a1queue to the provided buffer.
 * Takes O(1) to complete.
 * @param a1queue  the a1queue
 * @param buf    the popped item is compied here.
 * @param size   the byte size of buf.
 * @return The popped item. NULL if a1queue was empty.
 */
void *a1queue_pop_front_and_copy(struct a1queue *hnd, void *buf, int size);

/**
 * Peek an item from the front (head) of the a1queue.
 * Takes O(1) to complete.
 * @param hnd  the a1queue
 * @return The peeked item. The memory is owned by the a1queue.
 *         Do not free it!
 *         If a1queue is empty, then NULL is returned.
 */
void *a1queue_peek_head(struct a1queue *hnd);

/**
 * Peek an item from the back (tail) of the a1queue.
 * Takes O(1) to complete.
 * @param hnd  the a1queue
 * @return The peeked item. The memory is owned by the a1queue.
 *         Do not free it!
 *         If a1queue is empty, then NULL is returned.
 */
void *a1queue_peek_back(struct a1queue *hnd);

/**
 * Push an item to the front (head) of the a1queue.
 * Takes O(1) to complete.
 * @param hnd   the a1queue
 * @param ptr   the new a1queue item
 *              The API copies the content of ptr.
 * @param size  the size of the new a1queue item in bytes
 *              Must be <= than a1queue item size.
 * @retval true   success
 * @retval false  failure
 */
bool a1queue_push_front(struct a1queue *hnd, const void *ptr, int size);

/**
 * Push an item to the back (tail) of the a1queue.
 * Takes O(1) to complete.
 * @param hnd   the a1queue
 * @param ptr   the new a1queue item
 *              The API copies the content of ptr.
 * @param size  the size of the new a1queue item in bytes
 *              Must be <= than a1queue item size.
 * @retval true   success
 * @retval false  failure
 */
bool a1queue_push_back(struct a1queue *hnd, const void *ptr, int size);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_INCLUDED */
