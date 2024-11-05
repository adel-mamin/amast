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

#define AM_EVENT_REF_COUNTER_BITS 6
#define AM_EVENT_REF_COUNTER_MASK ((1U << AM_EVENT_REF_COUNTER_BITS) - 1U)

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
    unsigned ref_counter : AM_EVENT_REF_COUNTER_BITS;
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

/**
 * Duplicate an event.
 *
 * Allocate it from memory pools provided at initialization and
 * then copy the content of the given event to it.
 * The allocation cannot fail, if margin is 0.
 *
 * @param event   the event to duplicate
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to be available after the allocation
 * @return the newly allocated event.
 */
struct am_event *am_event_dup(
    const struct am_event *event, int size, int margin
);

/**
 * Log event content callback.
 *
 * @param pool_index   pool index
 * @param event_index  event_index within the pool
 * @param event        event to log
 * @param size         the event size [bytes]
 */
typedef void (*am_event_log_func)(
    int pool_index, int event_index, const struct am_event *event, int size
);

/**
 * Log event content of the first num events in each event pool.
 *
 * @param num  the number of events to log in each pool (if <0, then log all)
 * @param cb   the logging callback
 */
void am_event_log_pools(int num, am_event_log_func cb);

/**
 * Increment event reference counter.
 *
 * @param event  the event
 */
void am_event_inc_ref_cnt(struct am_event *event);

/**
 * Decrement event reference counter.
 *
 * @param event  the event
 */
void am_event_dec_ref_cnt(struct am_event *event);

/**
 * Return event reference counter.
 *
 * @param event  the event, which reference counter is to be returned
 * @return the event reference counter
 */
int am_event_get_ref_cnt(const struct am_event *event);

#endif /* EVENT_H_INCLUDED */
