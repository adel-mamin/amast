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

#include <stdbool.h>

#include "common/macros.h"
#include "queue/queue.h"

#define AM_EVT_RANGE_HSM_BEGIN 1
#define AM_EVT_RANGE_HSM_END 4
#define AM_EVT_RANGE_FSM_BEGIN 5
#define AM_EVT_RANGE_FSM_END 6
#define AM_EVT_INTERNAL_MAX AM_EVT_RANGE_FSM_END

/**
 * The event IDs below this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER (AM_EVT_INTERNAL_MAX + 1) /* 7 */

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

#define AM_EVENT_POOL_INDEX_BITS 5
#define AM_EVENT_POOL_INDEX_MASK ((1U << AM_EVENT_POOL_INDEX_BITS) - 1U)

/** Event descriptor */
struct am_event {
    /** event ID */
    int id;

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

/** Event module configuration. */
struct am_event_cfg {
    /** Push event to the front of owner event queue */
    void (*push_front)(void *owner, const struct am_event *event);
    /** Notify owner about event queue is busy. */
    void (*notify_event_queue_busy)(void *owner);
    /** Notify owner about event queue is empty. */
    void (*notify_event_queue_empty)(void *owner);
    /** Wait owner notification about event queue is busy. */
    void (*wait_event_queue_busy)(void *owner);
    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);
};

/**
 * Event state constructor.
 *
 * @param cfg  event state configuration
 *             The event module makes an internal copy of the configuration.
 */
void am_event_state_ctor(const struct am_event_cfg *cfg);

/**
 * Add an event memory pool.
 *
 * Event memory pools must be added in the order of increasing block sizes.
 *
 * @param pool        the memory pool pointer
 * @param size        the memory pool size [bytes]
 * @param block_size  the maximum size of allocated memory block [bytes]
 * @param alignment the required alignment of allocated memory blocks [bytes]
 */
void am_event_add_pool(void *pool, int size, int block_size, int alignment);

/**
 * Get minimum number of free memory blocks available so far in the memory pool
 * with a given index.
 * Could be used to assess the usage of the underlying memory pool.
 *
 * @param index  memory pool index
 *
 * @return the minimum number of blocks of size block_size available so far
 */
int am_event_get_pool_min_nfree(int index);

/**
 * Get minimum number of free memory blocks available now in the memory pool
 * with a given index.
 *
 * @param index  memory pool index
 *
 * @return the number of free blocks of size block_size available now
 */
int am_event_get_pool_nfree_now(int index);

/**
 * The number of blocks in the pool with the given index.
 *
 * @param index  the pool index
 * @return the number of blocks
 */
int am_event_get_pool_nblocks(int index);

/**
 * The number of registered pools.
 *
 * @return the number of pools
 */
int am_event_get_pools_num(void);

/**
 * Allocate an event from the memory pools provided at initialization.
 * The allocation cannot fail, if margin is 0.
 *
 * @param id      the event identifier
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to remain available after the allocation
 *
 * @return the newly allocated event
 */
struct am_event *am_event_allocate(int id, int size, int margin);

/**
 * Free the event allocated earlier with am_event_allocate().
 *
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
 *
 * @return the newly allocated event.
 */
struct am_event *am_event_dup(
    const struct am_event *event, int size, int margin
);

/**
 * Log event content callback type.
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
 * Check if event is static.
 * @param e  the event to check
 * @return true   the event is static
 * @return false  the event is not static
 */
static inline bool am_event_is_static(const struct am_event *event) {
    AM_ASSERT(event);
    return (0 == (event->pool_index & AM_EVENT_POOL_INDEX_MASK));
}

/**
 * Increment event reference counter.
 *
 * @param event  the event
 */
static inline void am_event_inc_ref_cnt(struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(event->ref_counter < AM_EVENT_REF_COUNTER_MASK);
    ++event->ref_counter;
}

/**
 * Decrement event reference counter.
 *
 * @param event  the event
 */
static inline void am_event_dec_ref_cnt(struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(event->ref_counter > 0);
    --event->ref_counter;
}

/**
 * Return event reference counter.
 *
 * @param event  the event, which reference counter is to be returned
 * @return the event reference counter
 */
static inline int am_event_get_ref_cnt(const struct am_event *event) {
    AM_ASSERT(event);
    return event->ref_counter;
}

/**
 * Push event to the back of event queue.
 *
 * Notify owner (if set) if it is the first event in the queue.
 * Does not assert if margin is non-zero and the event was not posted.
 *
 * @param owner   the event queue owner (optional)
 * @param queue   the event queue
 * @param event   the event to pst
 * @param margin  free event queue slots to be available after event was posted
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_event_push_back_x(
    void *owner,
    struct am_queue *queue,
    const struct am_event *event,
    int margin
);

/**
 * Push event to the back of event queue.
 *
 * Notify owner (if set) if it is the first event in the queue.
 * Assert if the event was not posted.
 *
 * @param owner   the event queue owner (optional)
 * @param queue   the event queue
 * @param event   the event to post
 */
void am_event_push_back(
    void *owner, struct am_queue *queue, const struct am_event *event
);

/**
 * Push event to the front of event queue.
 *
 * Notify owner (if set) if it is the first event in the queue.
 * Assert if the event was not posted.
 *
 * @param owner   the event queue owner (optional)
 * @param queue   the event queue
 * @param event   the event to post
 */
void am_event_push_front(
    void *owner, struct am_queue *queue, const struct am_event *event
);

/**
 * Pop event from the front of event queue.
 *
 * @param owner   the event queue owner (optional)
 * @param queue   the event queue
 *
 * @return the popped event. Cannot be NULL.
 */
const struct am_event *am_event_pop_front(void *owner, struct am_queue *queue);

/**
 * Defer an event.
 *
 * Assert if the event was not defered.
 *
 * @param queue  the queue to store the deferred event
 * @param event  the event to defer
 */
void am_event_defer(struct am_queue *queue, const struct am_event *event);

/**
 * Defer an event.
 *
 * @param queue   the queue to store the deferred event
 * @param event   the event to defer
 * @param margin  free event queue slots to be available after event is deferred
 */
void am_event_defer_x(
    struct am_queue *queue, const struct am_event *event, int margin
);

/**
 * Recall deferred event.
 *
 * @param owner  event owner
 * @param queue  queue of deferred events
 * @retval non-NULL the recalled event.
 *         DO NOT USE IT FOR ANYTHING BUT FOR THE REFERENCE.
 *         For example, do not push it to any event queue or free it.
 * @retval NULL no recalled events
 */
const struct am_event *am_event_recall(void *owner, struct am_queue *queue);

/**
 * Flush all event from event queue.
 * @param queue  the queue to flush
 * @return the number of events flushed
 */
int am_event_flush_queue(struct am_queue *queue);

#endif /* EVENT_H_INCLUDED */
