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
 *
 * Event API declaration.
 */

#ifndef AM_EVENT_POOL_H_INCLUDED
#define AM_EVENT_POOL_H_INCLUDED

#include "event/event_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Init event pool module */
void am_event_alloc_init(struct am_event_alloc* alloc);

/**
 * Add event memory pool.
 *
 * Event memory pools must be added in the order of increasing block sizes.
 *
 * Thread unsafe.
 *
 * Must be called before using any event allocation APIs.
 *
 * @param alloc       the event allocator
 * @param pool        the memory pool pointer
 * @param size        the memory pool size [bytes]
 * @param block_size  the maximum size of allocated memory block [bytes]
 * @param alignment   the required alignment of allocated memory blocks [bytes]
 */
void am_event_alloc_add_pool(
    struct am_event_alloc* alloc,
    void* pool,
    int size,
    int block_size,
    int alignment
);

/**
 * Get minimum number of free memory blocks observed so far.
 *
 * Could be used to assess the usage of the underlying memory pool.
 *
 * Thread safe.
 *
 * @param alloc  the event allocator
 * @param index  memory pool index
 *
 * @return the minimum number of memory blocks observed so far
 *         in the memory pool
 */
int am_event_alloc_get_nfree_min(struct am_event_alloc* alloc, int index);

/**
 * Get number of free memory blocks.
 *
 * Thread safe.
 *
 * Use sparingly as the return value could be volatile due to multitasking.
 *
 * @param alloc  the event allocator
 * @param index  memory pool index
 *
 * @return the number of free blocks available now in the memory pool
 */
int am_event_alloc_get_nfree(struct am_event_alloc* alloc, int index);

/**
 * The number of memory blocks in the pool with the given index.
 *
 * Thread safe.
 *
 * @param alloc  the event allocator
 * @param index  the pool index
 *
 * @return the number of memory blocks
 */
int am_event_alloc_get_nblocks(struct am_event_alloc* alloc, int index);

/**
 * The number of registered pools.
 *
 * Thread safe.
 *
 * @param alloc  the event allocator
 *
 * @return the number of pools
 */
int am_event_alloc_get_num(const struct am_event_alloc* alloc);

/**
 * Log events content of the first \p num events in each event pool.
 *
 * To be used for debugging purposes.
 *
 * Thread usafe.
 *
 * @param alloc  the event allocator
 * @param num    the number of events to log in each pool (if <0, then log all)
 * @param cb     the logging callback
 */
void am_event_alloc_log_unsafe(
    struct am_event_alloc* alloc, int num, am_event_log_fn cb
);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_POOL_H_INCLUDED */
