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

/** No event ID should have this value. */
#define AM_EVT_INVALID -1

/** State machine events range start (inclusive). */
#define AM_EVT_RANGE_SM_BEGIN 0
/** State machine events range end (inclusive). */
#define AM_EVT_RANGE_SM_END 3

#define AM_EVT_INTERNAL_MAX AM_EVT_RANGE_SM_END

/**
 * The event IDs smaller than this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER (AM_EVT_INTERNAL_MAX + 1) /* 7 */

#ifndef AM_EVENT_POOLS_NUM_MAX
/**
 * The max number of event pools.
 *
 * Can be redefined by user.
 */
#define AM_EVENT_POOLS_NUM_MAX 3
#endif

/**
 * Check if event has a valid user event ID.
 *
 * @param event   the event to check
 * @retval true   the event has valid user event ID
 * @retval false  the event does not have valid user event ID
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

/** Event API return codes. */
enum am_event_rc {
    AM_EVENT_RC_ERR = -1, /**< failure */
    AM_EVENT_RC_OK = 0,   /**< success */
    /**
     * Success.
     * Also tells that event queue was empty before the call.
     * This allows to signal the event queue owner about
     * new event available for processing.
     */
    AM_EVENT_RC_OK_QUEUE_WAS_EMPTY
};

/** Event descriptor. */
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
 * Must be called before calling any other event APIs.
 *
 * Not thread safe.
 *
 * @param cfg  event state configuration.
 *             The event module makes an internal copy of the configuration.
 */
void am_event_state_ctor(const struct am_event_state_cfg *cfg);

/**
 * Add event memory pool.
 *
 * Event memory pools must be added in the order of increasing block sizes.
 *
 * Not thread safe.
 *
 * Must be called before using any event allocation APIs.
 *
 * @param pool        the memory pool pointer
 * @param size        the memory pool size [bytes]
 * @param block_size  the maximum size of allocated memory block [bytes]
 * @param alignment   the required alignment of allocated memory blocks [bytes]
 */
void am_event_add_pool(void *pool, int size, int block_size, int alignment);

/**
 * Get minimum number of free memory blocks observed so far.
 *
 * Could be used to assess the usage of the underlying memory pool.
 *
 * Thread safe.
 *
 * @param index  memory pool index
 *
 * @return the minimum number of memory blocks observed so far
 *         in the memory pool
 */
int am_event_get_pool_nfree_min(int index);

/**
 * Get number of free memory blocks.
 *
 * Thread safe.
 *
 * Use sparingly as the return value could be volatile due to multitasking.
 *
 * @param index  memory pool index
 *
 * @return the number of free blocks available now in the memory pool
 */
int am_event_get_pool_nfree(int index);

/**
 * The number of memory blocks in the pool with the given index.
 *
 * Thread safe.
 *
 * @param index  the pool index
 *
 * @return the number of memory blocks
 */
int am_event_get_pool_nblocks(int index);

/**
 * The number of registered pools.
 *
 * Thread safe.
 *
 * @return the number of pools
 */
int am_event_get_npools(void);

/**
 * Allocate event (eXtended version).
 *
 * The event is allocated from one of the memory pools provided
 * with am_event_add_pool() function.
 *
 * Checks if there are more free memory blocks available than \p margin.
 * If not, then returns NULL. Otherwise allocates the event and returns it.
 *
 * Thread safe.
 *
 * @param id      the event identifier
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to remain available after the allocation
 *
 * @return the newly allocated event
 */
struct am_event *am_event_allocate_x(int id, int size, int margin);

/**
 * Allocate event.
 *
 * The event is allocated from one of the memory pools provided
 * with am_event_add_pool() function.
 *
 * The function asserts if there is no memory left to accommodate the event.
 *
 * Thread safe.
 *
 * @param id    the event identifier
 * @param size  the event size [bytes]
 *
 * @return the newly allocated event
 */
struct am_event *am_event_allocate(int id, int size);

/**
 * Try to free the event allocated earlier with am_event_allocate().
 *
 * Decrements event reference counter by one and frees the event,
 * if the reference counter drops to zero.
 *
 * If the event is freed, then the pointer to the event is set to NULL
 * to help catching double free cases.
 *
 * The function does nothing for statically allocated events -
 * the events for which am_event_is_static() returns true.
 *
 * Thread safe.
 *
 * @param event  the event to free
 */
void am_event_free(const struct am_event **event);

/**
 * Duplicate an event (eXtended version).
 *
 * Allocates it from memory pools provided with am_event_add_pool() function.
 *
 * Checks if there are more free memory blocks available than \p margin.
 * If not, then returns NULL. Otherwise allocates memory block
 * and then copies the content of the given event to it.
 *
 * Thread safe.
 *
 * @param event   the event to duplicate
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to be available after the allocation
 *
 * @return the newly allocated copy of \p event
 */
struct am_event *am_event_dup_x(
    const struct am_event *event, int size, int margin
);

/**
 * Duplicate an event.
 *
 * Allocates it from memory pools provided with am_event_add_pool() function.
 *
 * The function asserts if there is no memory left to allocated the event.
 *
 * Thread safe.
 *
 * @param event  the event to duplicate
 * @param size   the event size [bytes]
 *
 * @return the newly allocated copy of \p event
 */
struct am_event *am_event_dup(const struct am_event *event, int size);

/**
 * Log event content callback type.
 *
 * Used as a parameter to am_event_log_pools() API.
 *
 * @param pool_index   pool index
 * @param event_index  event_index within the pool
 * @param event        event to log
 * @param size         the event size [bytes]
 */
typedef void (*am_event_log_fn)(
    int pool_index, int event_index, const struct am_event *event, int size
);

/**
 * Log events content of the first \p num events in each event pool.
 *
 * To be used for debugging purposes.
 *
 * Not thread safe.
 *
 * @param num  the number of events to log in each pool (if <0, then log all)
 * @param cb   the logging callback
 */
void am_event_log_pools(int num, am_event_log_fn cb);

/**
 * Check if event is static.
 *
 * Thread safe.
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
 * Thread safe.
 *
 * @param event  the event
 */
void am_event_inc_ref_cnt(const struct am_event *event);

/**
 * Decrement event reference counter by one.
 *
 * Frees the event, if the reference counter drops to zero.
 *
 * The function does nothing for statically allocated events -
 * the events for which am_event_is_static() returns true.
 *
 * Thread safe.
 *
 * @param event  the event
 */
void am_event_dec_ref_cnt(const struct am_event *event);

/**
 * Return event reference counter.
 *
 * Thread safe.
 *
 * Use sparingly as the return value could be volatile due to multitasking.
 *
 * @param event  the event, which reference counter is to be returned
 *
 * @return the event reference counter
 */
int am_event_get_ref_cnt(const struct am_event *event);

/**
 * Push event to the back of event queue (eXtended version).
 *
 * Checks if there are more free queue slots available than \p margin.
 * If not, then does not push. Otherwise pushes the event to the back of
 * the event queue.
 *
 * Tries to free the event, if it was not pushed.
 *
 * Statically allocated events (the events for which am_event_is_static()
 * returns true) are never freed.
 *
 * Thread safe.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param margin  free event queue slots to be available after
 *                the event was pushed
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
 * Thread safe.
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
 * Push event to the back of event queue without using critical section
 * (unsafe).
 *
 * Asserts if the event was not pushed.
 *
 * Thread unsafe.
 *
 * So far the only use of this function was as a part of the implementation
 * of `am_timer_publish_fn` callback in timer module.
 *
 * @param queue  the event queue
 * @param event  the event to push
 *
 * @retval AM_EVENT_RC_OK                  the event was pushed
 * @retval AM_EVENT_RC_OK_QUEUE_WAS_EMPTY  the event was pushed, queue was empty
 * @retval AM_EVENT_RC_ERR                 the event was not pushed
 */
enum am_event_rc am_event_push_back_unsafe(
    struct am_queue *queue, const struct am_event *event
);

/**
 * Push event to the front of event queue with margin.
 *
 * Checks if there are more free queue slots available than \p margin.
 * If not, then does not push. Otherwise pushes the event to the front of
 * the event queue.
 *
 * Tries to free the event, if it was not pushed.
 *
 * Statically allocated events (the events for which am_event_is_static()
 * returns true) are never freed.
 *
 * Thread safe.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param margin  free event queue slots to be available after
 *                the event was pushed
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
 * Thread safe.
 *
 * @param queue  the event queue
 * @param event  the event to push
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
 * Thread safe.
 *
 * @param queue  the event queue
 *
 * @return the popped event. Cannot be NULL.
 */
const struct am_event *am_event_pop_front(struct am_queue *queue);

/**
 * Defer event (eXtended version).
 *
 * Checks if there are more free queue slots available than \p margin.
 * If not, then does not defer. Otherwise defers the event by pushing it
 * to the back of the event queue.
 *
 * Tries to free event, if defer fails.
 *
 * Statically allocated events (the events for which am_event_is_static()
 * returns true) are never freed.
 *
 * Thread safe.
 *
 * @param queue   the queue to store the deferred event
 * @param event   the event to defer
 * @param margin  free event queue slots to be available after the event is
 * deferred
 *
 * @retval true   the event was deferred
 * @retval false  the event was not deferred
 */
bool am_event_defer_x(
    struct am_queue *queue, const struct am_event *event, int margin
);

/**
 * Defer event.
 *
 * Defers the event by pushing the event to the back of the event queue.
 *
 * Asserts if the event was not deferred.
 *
 * Thread safe.
 *
 * @param queue  the queue to store the deferred event
 * @param event  the event to defer
 */
void am_event_defer(struct am_queue *queue, const struct am_event *event);

/**
 * The type of a callback used to handle recalled events.
 *
 * Used as a parameter to am_event_recall() API.
 *
 * @param ctx    the callback context
 * @param event  the recalled event
 */
typedef void (*am_event_recall_fn)(void *ctx, const struct am_event *event);

/**
 * Recall deferred event.
 *
 * Event recalling is done by popping event from the front of event queue
 * and calling the provided callback with the event.
 *
 * Tries to free the recalled event after the callback call.
 *
 * Statically allocated events (the events for which am_event_is_static()
 * returns true) are never freed.
 *
 * Thread safe.
 *
 * @param queue  queue of deferred events
 * @param cb     the recalled event is provided to this callback.
 *               If set to NULL, then event is recalled and immediately
 *               recycled. Therefore cb set to NULL can be used, when
 *               the queue of deferred events needs to be fully or
 *               partially emptied to make space for newer events.
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
 * Thread safe.
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
