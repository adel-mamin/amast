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

#ifndef AM_EVENT_QUEUE_H_INCLUDED
#define AM_EVENT_QUEUE_H_INCLUDED

#include <stdbool.h>

#include "common/types.h"
#include "event_common.h"

/** Event queue handler. */
struct am_event_queue {
    int rd;        /**< read index */
    int wr;        /**< write index */
    int nfree;     /**< number of free slots */
    int nfree_min; /**< minimum number of free slots observed so far */
    int capacity;  /**< queue capacity [number of items of isize] */
    const struct am_event** events; /**< event queue */

    struct am_event_alloc* alloc; /**< the event allocator */

    unsigned full : 1; /**< queue is full */
    /** safety net to catch missing am_event_queue_ctor() call */
    unsigned ctor_called : 1;
};

/** Event queue handling policy. */
struct am_event_queue_policy {
    /**
     * Event should be served in LIFO mode, if set to true.
     * Otherwise FIFO mode is to be used.
     */
    unsigned lifo : 1;
    /**
     * The number of the free slots, which must remain
     * in event queue after placing the event.
     */
    int margin;
    /**
     * Event should not be placed to the event queue of
     * the event handler with this ID.
     * Can be set to AM_EVENT_PUBLISHER_ID_NONE
     * if not relevant.
     */
    int exclude_id;
};

/** Default event queue policy */
#define AM_EVENT_QUEUE_POLICY_DEFAULT (struct am_event_queue_policy){0}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Construct event queue with a array of event pointers.
 *
 * @param queue    the queue
 * @param events   the array of event pointers
 * @param nevents  the number of event pointers in \a events
 * @param alloc    the event memory allocator
 */
void am_event_queue_ctor(
    struct am_event_queue* queue,
    const struct am_event* events[],
    int nevents,
    struct am_event_alloc* alloc
);

/**
 * Destruct event queue.
 *
 * @param queue  the queue
 */
void am_event_queue_dtor(struct am_event_queue* queue);

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
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param policy  the event queue handling policy
 *
 * @retval #AM_RC_OK               the event was pushed
 * @retval #AM_RC_QUEUE_WAS_EMPTY  the event was pushed,
 *                                 queue was empty
 * @retval #AM_RC_ERR              the event was not pushed
 */
enum am_rc am_event_queue_push(
    struct am_event_queue* queue,
    const struct am_event* event,
    struct am_event_queue_policy policy
);

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
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param queue   the event queue
 * @param event   the event to push
 * @param policy  the event queue handling policy
 *
 * @retval #AM_RC_OK               the event was pushed
 * @retval #AM_RC_QUEUE_WAS_EMPTY  the event was pushed,
 *                                 queue was empty
 * @retval #AM_RC_ERR              the event was not pushed
 */
enum am_rc am_event_queue_push_unsafe(
    struct am_event_queue* queue,
    const struct am_event* event,
    struct am_event_queue_policy policy
);

/**
 * Push event to the back of event queue.
 *
 * Asserts, if the event was not pushed.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param queue  the event queue
 * @param event  the event to push
 *
 * @retval #AM_RC_OK               the event was pushed
 * @retval #AM_RC_QUEUE_WAS_EMPTY  the event was pushed,
 *                                 queue was empty
 */
enum am_rc am_event_queue_push_back(
    struct am_event_queue* queue, const struct am_event* event
);

/**
 * Push event to the front of event queue.
 *
 * Asserts, if the event was not pushed.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param queue  the event queue
 * @param event  the event to push
 *
 * @retval #AM_RC_OK               the event was pushed
 * @retval #AM_RC_QUEUE_WAS_EMPTY  the event was pushed,
 *                                 queue was empty
 */
enum am_rc am_event_queue_push_front(
    struct am_event_queue* queue, const struct am_event* event
);

/**
 * The type of a callback used to handle popped events.
 *
 * Used as a parameter to am_event_queue_pop_front_with_cb() API.
 *
 * @param ctx    the callback context
 * @param event  the popped event
 *
 * @return Return code.
 */
typedef enum am_rc (*am_event_handler_fn)(
    void* ctx, const struct am_event* event
);

/**
 * Pop event from the front of event queue.
 *
 * Pops event from the front of event queue and calls
 * the provided callback with the event.
 *
 * Tries to free the popped event after the callback call.
 *
 * Statically allocated events (the events for which am_event_is_static()
 * returns true) are never freed.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param queue  queue of events
 * @param cb     the popped event is provided to this callback.
 *               If set to NULL, then event is popped and immediately
 *               recycled. Therefore cb set to NULL can be used, when
 *               the queue of events needs to be fully or
 *               partially emptied to make space for newer events.
 * @param ctx    the callback context
 *
 * @retval #AM_RC_OK   the event was popped
 * @retval #AM_RC_ERR  the event was not popped
 */
enum am_rc am_event_queue_pop_front_with_cb(
    struct am_event_queue* queue, am_event_handler_fn cb, void* ctx
);

/**
 * Check if constructor was called for event queue.
 *
 * Thread safe.
 *
 * @param queue  the event queue
 *
 * @retval true   the queue is valid
 * @retval false  the queue is invalid
 */
bool am_event_queue_is_valid(const struct am_event_queue* queue);

/**
 * Check if event queue is empty.
 *
 * Thread safe.
 *
 * @param queue  the event queue
 *
 * @retval true   queue is empty
 * @retval false  queue is not empty
 */
bool am_event_queue_is_empty(const struct am_event_queue* queue);

/**
 * Check if event queue is empty.
 *
 * Thread unsafe.
 *
 * @param queue  the event queue
 *
 * @retval true   queue is empty
 * @retval false  queue is not empty
 */
bool am_event_queue_is_empty_unsafe(const struct am_event_queue* queue);

/**
 * Return how many items are in event queue.
 *
 * Thread unsafe.
 *
 * @param queue  the event queue
 *
 * @return number of queued events
 */
int am_event_queue_get_nbusy_unsafe(const struct am_event_queue* queue);

/**
 * Return event queue capacity.
 *
 * Thread usafe.
 *
 * @param queue  the event queue
 *
 * @return the event queue capacity in number of items (slots)
 */
int am_event_queue_get_capacity(const struct am_event_queue* queue);

/**
 * Pop an item from the front (head) of event queue.
 *
 * Takes O(1) to complete.
 *
 * Thread unsafe.
 *
 * @param queue  the event queue
 *
 * @return The popped item or NULL, if event queue was empty.
 */
const struct am_event* am_event_queue_pop_front_unsafe(
    struct am_event_queue* queue
);

/**
 * Pop an item from the front (head) of event queue.
 *
 * Takes O(1) to complete.
 *
 * Thread safe.
 *
 * @param queue  the event queue
 *
 * @return The popped item or NULL, if event queue was empty.
 */
const struct am_event* am_event_queue_pop_front(struct am_event_queue* queue);

/**
 * Get minimum number of free slots ever observed in event queue.
 *
 * Could be used to assess the usage of the event queue.
 *
 * Thread safe.
 *
 * @param queue  the event queue
 *
 * @return the minimum number of slots observed so far
 */
int am_event_queue_get_nfree_min(const struct am_event_queue* queue);

/**
 * Flush all events from event queue.
 *
 * Takes care of recycling the events by calling am_event_free().
 *
 * Thread safe.
 *
 * @param queue  the event queue to flush
 *
 * @return the number of events flushed
 */
int am_event_queue_flush(struct am_event_queue* queue);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_QUEUE_H_INCLUDED */
