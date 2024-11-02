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
 *
 * Event API declaration.
 */

#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

/**
 * The event IDs below this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER 5

#ifndef AM_EVENT_POOL_NUM_MAX
#define AM_EVENT_POOL_NUM_MAX 3
#endif

/**
 * Check if event has a valid user event ID
 * @param event   event to check
 * @retval true   the event has user event ID
 * @retval false  the event does not have user event ID
 */
#define AM_EVENT_HAS_USER_ID(event) \
    (((const struct am_event *)(event))->id >= AM_EVT_USER)

#define AM_EVENT_TICK_DOMAIN_BITS 3
#define AM_EVENT_TICK_DOMAIN_MASK ((1U << AM_EVENT_TICK_DOMAIN_BITS) - 1U)

#define AM_EVT_CTOR(id_) ((struct am_event){.id = (id_)})

#define AM_EVENT_POOL_INDEX_BITS 5

/** Event descriptor */
struct am_event {
    /** event ID */
    int id;

    /**
     * The flags below have special purpose in active objects framework (AOF).
     * Otherwise are unused.
     */

    /** reference counter */
    unsigned ref_counter : 6;
    /** if set to zero, then event is statically allocated */
    unsigned pool_index : AM_EVENT_POOL_INDEX_BITS;
    /** tick domain for time events */
    unsigned tick_domain : AM_EVENT_TICK_DOMAIN_BITS;
    /** PUB/SUB time event */
    unsigned pubsub_time : 1;
    /** n/a */
    unsigned reserved : 1;
};

/**
 * Adds an event memory pool.
 * The event memory pools must be added in the order of increasing block sizes.
 * @param pool        the memory pool pointer
 * @param size        the memory pool size [bytes]
 * @param block_size  the maximum size of allocated memory block [bytes]
 * @param alignment the required alignment of allocated memory blocks [bytes].
 */
void am_event_add_pool(void *pool, int size, int block_size, int alignment);

/**
 * The minimum number of free memory blocks available so far in the memory pool
 * with a given index.
 * Could be used to assess the usage of the underlying memory pool.
 * @param index  memory pool index
 * @return the minimum number of blocks of size block_size available so far
 */
int am_event_get_pool_min_nfree(int index);

/**
 * The minimum number of free memory blocks available now in the memory pool
 * with a given index.
 * @param index  memory pool index
 * @return the number of free blocks of size block_size available now
 */
int am_event_get_pool_nfree_now(int index);

/**
 * The number of blocks in the pool with the given index.
 * @param index  the pool index
 * @return the number of blocks
 */
int am_event_get_pool_nblocks(int index);

/**
 * The number of registered pools.
 * @return the number of pools
 */
int am_event_get_pools_num(void);

/**
 * Allocates an event from memory pools provided at initialization.
 * The allocation cannot fail, if margin is 0.
 * @param id      the event identifier
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to be available after the allocation
 * @return the newly allocated event.
 */
struct am_event *am_event_allocate(int id, int size, int margin);

/**
 * Free the event allocated earlier with am_event_allocate().
 * @param event  the event to free
 */
void am_event_free(const struct am_event *event);

#endif /* EVENT_H_INCLUDED */
