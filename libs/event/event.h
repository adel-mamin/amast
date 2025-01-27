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

#ifndef AM_EVENT_H_INCLUDED
#define AM_EVENT_H_INCLUDED

#include <stdbool.h>

#include "queue/queue.h"

/** No event ID should have this value */
#define AM_EVT_INVALID 0

/** HSM events range */
#define AM_EVT_RANGE_HSM_BEGIN 1
#define AM_EVT_RANGE_HSM_END 4

/** FSM events range */
#define AM_EVT_RANGE_FSM_BEGIN 5
#define AM_EVT_RANGE_FSM_END 6

#define AM_EVT_INTERNAL_MAX AM_EVT_RANGE_FSM_END

/**
 * The event IDs below this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER (AM_EVT_INTERNAL_MAX + 1) /* 7 */

#ifndef AM_EVENT_POOL_NUM_MAX
/**
 * The max number of event pool.
 *
 * Can be redefined by user.
 */
#define AM_EVENT_POOL_NUM_MAX 3
#endif

/**
 * Check if event has a valid user event ID.
 *
 * @param event   event to check
 * @retval true   the event has user event ID
 * @retval false  the event does not have user event ID
 */
#define AM_EVENT_HAS_USER_ID(event) \
    (((const struct am_event *)(event))->id >= AM_EVT_USER)

/* _BIS and _MASK defines are used for internal purposes */
#define AM_EVENT_REF_COUNTER_BITS 6
#define AM_EVENT_REF_COUNTER_MASK ((1U << AM_EVENT_REF_COUNTER_BITS) - 1U)

#define AM_EVENT_TICK_DOMAIN_BITS 3
#define AM_EVENT_TICK_DOMAIN_MASK ((1U << AM_EVENT_TICK_DOMAIN_BITS) - 1U)

#define AM_EVENT_POOL_INDEX_BITS 5
#define AM_EVENT_POOL_INDEX_MASK ((1U << AM_EVENT_POOL_INDEX_BITS) - 1U)

extern const int am_alignof_event;
extern const int am_alignof_event_ptr;

#define AM_ALIGNOF_EVENT am_alignof_event
#define AM_ALIGNOF_EVENT_PTR am_alignof_event_ptr

/** Event API return codes */
enum am_event_rc {
    AM_EVENT_RC_ERR = -1,
    AM_EVENT_RC_OK = 0,
    AM_EVENT_RC_OK_QUEUE_WAS_EMPTY
};

/** Event descriptor */
struct am_event {
    /** event ID */
    int id;

    /** reference counter */
    unsigned ref_counter : AM_EVENT_REF_COUNTER_BITS;
    /** if set to zero, then event is statically allocated */
    unsigned pool_index_plus_one : AM_EVENT_POOL_INDEX_BITS;
    /** tick domain for time events */
    unsigned tick_domain : AM_EVENT_TICK_DOMAIN_BITS;
    /** PUB/SUB time event */
    unsigned pubsub_time : 1;
};

/** Event module state configuration. */
struct am_event_state_cfg {
    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Event state constructor.
 *
 * @param cfg  event state configuration
 *             The event module makes an internal copy of the configuration.
 */
void am_event_state_ctor(const struct am_event_state_cfg *cfg);

/**
 * Add event memory pool.
 *
 * Event memory pools must be added in the order of increasing block sizes.
 * Not thread safe. Expected to be called at initialization.
 *
 * @param pool        the memory pool pointer
 * @param size        the memory pool size [bytes]
 * @param block_size  the maximum size of allocated memory block [bytes]
 * @param alignment   the required alignment of allocated memory blocks [bytes]
 */
void am_event_add_pool(void *pool, int size, int block_size, int alignment);

/**
 * Get minimum number of free memory blocks available so far in the memory pool
 * with a given index.
 *
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
 *
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
 * Allocate event.
 *
 * The event is allocated from one of the memory pools provided
 * with am_event_add_pool() function.
 *
 * The allocation cannot fail, if margin is 0.
 *
 * The function asserts if the margin is 0 and there is no memory left
 * to accommodate the event.
 *
 * @param id      the event identifier
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to remain available after the allocation
 *
 * @return the newly allocated event
 */
struct am_event *am_event_allocate(int id, int size, int margin);

/**
 * Try to free the event allocated earlier with am_event_allocate().
 *
 * Decrements event reference counter by one and free the event,
 * if the reference counter is zero.
 *
 * If the event is freed, then the pointer to the event is set to NULL
 * to catch double free cases.
 *
 * The function does nothing for statically allocated events
 * (events for which am_event_is_static() returns true).
 *
 * @param event  the event to free
 */
void am_event_free(const struct am_event **event);

/**
 * Duplicate an event.
 *
 * Allocates it from memory pools provided at with am_event_add_pool() function
 * and then copy the content of the given event to it.
 *
 * The allocation cannot fail, if margin is 0.
 *
 * The function asserts if the margin is 0 and there is no memory left
 * to accommodate the event.
 *
 * @param event   the event to duplicate
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to be available after the allocation
 *
 * @return the newly allocated event
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
 * Log events content of the first num events in each event pool.
 *
 * To be used for debugging purposes.
 *
 * @param num  the number of events to log in each pool (if <0, then log all)
 * @param cb   the logging callback
 */
void am_event_log_pools(int num, am_event_log_func cb);

/**
 * Check if event is static.
 *
 * @param event  the event to check
 *
 * @retval true   the event is static
 * @retval false  the event is not static
 */
bool am_event_is_static(const struct am_event *event);

/**
 * Increment event reference counter by one.
 *
 * Incrementing the event reference prevents the automatic event disposal.
 * Used to hold on to the event.
 * Call am_event_dec_ref_cnt(), when the event is not needed anymore
 * and can be disposed.
 *
 * @param event  the event
 */
void am_event_inc_ref_cnt(const struct am_event *event);

/**
 * Decrement event reference counter by one.
 *
 * Frees the event, if the reference counter drops to zero.
 *
 * The function does nothing for statically allocated events
 * (events for which am_event_is_static() returns true).
 *
 * @param event  the event
 */
void am_event_dec_ref_cnt(const struct am_event *event);

/**
 * Return event reference counter.
 *
 * @param event  the event, which reference counter is to be returned
 *
 * @return the event reference counter
 */
int am_event_get_ref_cnt(const struct am_event *event);

/**
 * Push event to the back of event queue with margin.
 *
 * Does not assert if margin is non-zero and the event was not pushed.
 *
 * Tries to free the event, if it was not pushed.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param margin  free event queue slots to be available after event was pushed
 *
 * @retval AM_EVENT_RC_OK                  the event was pushed
 * @retval AM_EVENT_RC_OK_QUEUE_WAS_EMPTY  the event was pushed, queue was empty
 * @retval AM_EVENT_RC_ERR                 the event was not pushed
 */
enum am_event_rc am_event_push_back_x(
    struct am_queue *queue, const struct am_event *event, int margin
);

/**
 * Push event to the back of event queue.
 *
 * Asserts if the event was not pushed.
 *
 * @param queue  the event queue
 * @param event  the event to push
 *
 * @retval AM_EVENT_RC_OK                  the event was pushed
 * @retval AM_EVENT_RC_OK_QUEUE_WAS_EMPTY  the event was pushed, queue was empty
 * @retval AM_EVENT_RC_ERR                 the event was not pushed
 */
enum am_event_rc am_event_push_back(
    struct am_queue *queue, const struct am_event *event
);

/**
 * Push event to the front of event queue with margin.
 *
 * Does not assert if margin is non-zero and the event was not pushed.
 *
 * Tries to free the event, if it was not pushed.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param margin  free event queue slots to be available after event was pushed
 *
 * @retval AM_EVENT_RC_OK                  the event was pushed
 * @retval AM_EVENT_RC_OK_QUEUE_WAS_EMPTY  the event was pushed, queue was empty
 * @retval AM_EVENT_RC_ERR                 the event was not pushed
 */
enum am_event_rc am_event_push_front_x(
    struct am_queue *queue, const struct am_event *event, int margin
);

/**
 * Push event to the front of event queue.
 *
 * Asserts if the event was not pushed.
 *
 * @param queue   the event queue
 * @param event   the event to push
 *
 * @retval AM_EVENT_RC_OK                  the event was pushed
 * @retval AM_EVENT_RC_OK_QUEUE_WAS_EMPTY  the event was pushed, queue was empty
 * @retval AM_EVENT_RC_ERR                 the event was not pushed
 */
enum am_event_rc am_event_push_front(
    struct am_queue *queue, const struct am_event *event
);

/**
 * Pop event from the front of event queue.
 *
 * @param queue  the event queue
 *
 * @return the popped event. Cannot be NULL.
 */
const struct am_event *am_event_pop_front(struct am_queue *queue);

/**
 * Defer event.
 *
 * Asserts if the event was not deferred.
 *
 * @param queue  the queue to store the deferred event
 * @param event  the event to defer
 */
void am_event_defer(struct am_queue *queue, const struct am_event *event);

/**
 * Defer event with margin.
 *
 * Tries to free event, if defer fails.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * @param queue   the queue to store the deferred event
 * @param event   the event to defer
 * @param margin  free event queue slots to be available after event is deferred
 *
 * @retval true   the event was deferred
 * @retval false  the event was not deferred
 */
bool am_event_defer_x(
    struct am_queue *queue, const struct am_event *event, int margin
);

/** The type of a callback used to handle recalled events */
typedef void (*am_event_recall_fn)(void *ctx, const struct am_event *event);

/**
 * Recall deferred event.
 *
 * @param queue  queue of deferred events
 * @param cb     the recalled event is provided to this callback
 * @param ctx    the callback context
 *
 * @retval true   an event was recalled
 * @retval false  no event was recalled
 */
bool am_event_recall(struct am_queue *queue, am_event_recall_fn cb, void *ctx);

/**
 * Flush all events from event queue.
 *
 * Takes care of recycling the events by calling am_event_free().
 * The provided queue might be deferred events queue.
 *
 * @param queue  the queue to flush
 *
 * @return the number of events flushed
 */
int am_event_flush_queue(struct am_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_H_INCLUDED */
