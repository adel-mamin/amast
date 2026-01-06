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
 * Active Object (AO) API declaration.
 */

#ifndef AM_AO_H_INCLUDED
#define AM_AO_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"

#ifndef AM_AO_NUM_MAX
/** The maximum number of active objects. */
#define AM_AO_NUM_MAX 64
#endif

struct am_ao_prio;

/** Invalid AO priority. */
#define AM_AO_PRIO_INVALID \
    (struct am_ao_prio){.ao = (unsigned char)-1, .task = (unsigned char)-1}
/** The minimum AO priority level. */
#define AM_AO_PRIO_MIN 0
/** The maximum AO priority level. */
#define AM_AO_PRIO_MAX (AM_AO_NUM_MAX - 1)

/** The low AO priority level. */
#define AM_AO_PRIO_LOW (AM_AO_PRIO_MAX / 4)
/** The medium AO priority level. */
#define AM_AO_PRIO_MID (AM_AO_PRIO_MAX / 2)
/** The high AO priority level. */
#define AM_AO_PRIO_HIGH (3 * AM_AO_PRIO_MAX) / 4

/**
 * Check if active object priority is valid.
 *
 * @param prio  the active object priority
 *
 * @retval true   the priority is valid
 * @retval false  the priority is invalid.
 */
#define AM_AO_PRIO_IS_VALID(prio) \
    ((prio.ao <= AM_AO_PRIO_MAX) && (prio.task <= AM_AO_PRIO_MAX))

AM_ASSERT_STATIC(AM_AO_NUM_MAX <= AM_TASK_NUM_MAX);

/** AO priorities. */
struct am_ao_prio {
    /**
     * Define the priority of active object.
     * Used by AO library. Valid range [0, #AM_TASK_NUM_MAX[.
     * Must be unique for different active objects.
     * Used by both cooperative and preemptive ports of active objects.
     */
    unsigned ao : 8;
    /**
     * Define the priority of the task, which runs active object.
     * Used by PAL library. Valid range [0, #AM_TASK_NUM_MAX[.
     * More than one active object may have same task priority.
     * Only used by preemptive port of active objects.
     */
    unsigned task : 8;
};

/** The active object. */
struct am_ao {
    struct am_hsm hsm;                 /**< top level AO state machine */
    const char *name;                  /**< human readable name of AO */
    struct am_event_queue event_queue; /**< event queue */
    int last_event;                    /**< last processed event */
    int task_id;                       /**< task handle */
    /** AO priority */
    struct am_ao_prio prio;
    /** safety net to catch missing am_ao_ctor() call */
    unsigned ctor_called : 1;
    /** am_ao_stop() call was made for the AO */
    unsigned stopped : 1;
};

/** Active object library state configuration. */
struct am_ao_state_cfg {
    /** Debug callback. */
    void (*debug)(const struct am_ao *ao, const struct am_event *e);
    /**
     * Callback to enter low power mode.
     *
     * The callback is called with critical section being entered
     * by calling am_ao_state_cfg::crit_enter() to allow
     * for race condition free transition to low power mode(s).
     * The am_ao_state_cfg::crit_exit() is called by the library after
     * the callback is returned.
     *
     * Do not post or publish events from this callback.
     *
     * Please read the article called
     * "Use an MCU's low-power modes in foreground/background systems"
     * by Miro Samek for more information about the reasoning of the approach.
     */
    void (*on_idle)(void);

    /** Callback to enter critical section. */
    void (*crit_enter)(void);
    /** Callback to exit critical section. */
    void (*crit_exit)(void);
};

/** The subscribe list for one event. */
struct am_ao_subscribe_list {
    uint8_t list[AM_DIV_CEIL(AM_AO_NUM_MAX, 8)]; /**< the bitmask */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Publish \p event to all subscribed active objects except the given one
 * (eXtended version).
 *
 * The \p event is delivered to event queues of all the active objects,
 * which are subscribed to the \p event excluding the specified AO.
 * The \p event is then handled asynchronously by the active objects.
 *
 * Might be useful, if the AO publishing the \p event does not want
 * the library to route the same event back to this AO.
 *
 * Application might observe changed order of events after the publishing.
 * This change of order might happen, if the publishing is from low-priority
 * active object, which then will get immediately preempted by higher-priority
 * subscribers. This might or might not matter to your application.
 *
 * Use am_ao_subscribe() to subscribe an active object to an event.
 * Use am_ao_unsubscribe() to unsubscribe it from the event
 * or am_ao_unsubscribe_all() to unsubscribe it from all events.
 *
 * Guarantees availability of \p margin free slots in destination event queues
 * after the \p event was delivered to subscribed active objects.
 *
 * If any active object cannot accommodate the \p event,
 * then the function skips the \p event delivery to the active object.
 *
 * If your application is not prepared for loosing the \p event,
 * then use am_ao_publish_exclude() function instead.
 *
 * Internally the \p event is pushed to subscribed active object event queues
 * using am_event_queue_push_back() function.
 *
 * Tries to free the \p event synchronously, if it was not delivered
 * to any subscriber.
 *
 * The library takes care of freeing the \p event once all
 * subscribed active objects handled it.
 * This is done asynchronously after this function returns.
 *
 * Statically allocated events, i.e. the events for which
 * am_event_is_static() returns true, are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param event   the event to publish
 * @param ao      do not post the event to this active object even
 *                if it is subscribed to the event.
 *                If set to NULL, the API behaves same way as
 *                am_ao_publish_x()
 * @param margin  the number of free event queue slots to be available in each
 *                subscribed active object after the event is pushed
 *                to their event queues
 *
 * @retval true   the event was delivered to all subscribed active objects
 *                except the active object \p ao
 * @retval false  at least one delivery of the event has failed
 */
bool am_ao_publish_exclude_x(
    const struct am_event *event, const struct am_ao *ao, int margin
);

/**
 * Publish \p event to all subscribed active objects except the given one.
 *
 * Same as am_ao_publish_exclude_x() except this function
 * crashes with assert, if it fails delivering the \p event to at
 * least one subscribed active object.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param event  the event to publish
 * @param ao     do not post the event to this active object even
 *               if it is subscribed to the event.
 *               If set to NULL, the API behaves same way as
 *               am_ao_publish().
 */
void am_ao_publish_exclude(
    const struct am_event *event, const struct am_ao *ao
);

/**
 * Publish \p event to all subscribed active objects (eXtended version).
 *
 * The \p event is delivered to event queues of all the active objects,
 * which are subscribed to the \p event including the AO publishing
 * the \p event.
 * The \p event is then handled asynchronously by the active objects.
 *
 * Application might observe changed order of events after the publishing.
 * This change of order might happen, if the publishing is from low-priority
 * active object, which then will get immediately preempted by higher-priority
 * subscribers. This might or might not matter to your application.
 *
 * Use am_ao_subscribe() to subscribe an active object to an event.
 * Use am_ao_unsubscribe() to unsubscribe it from the event.
 * or am_ao_unsubscribe_all() to unsubscribe it from all events.
 *
 * Guarantees availability of \p margin free slots in destination event queues
 * after the \p event was delivered to subscribed active objects.
 *
 * If any active object cannot accommodate the \p event, then the function skips
 * the \p event delivery to the active object.
 *
 * If your application is not prepared for loosing the \p event,
 * then use am_ao_publish() function instead.
 *
 * Internally the \p event is pushed to subscribed active object event queues
 * using am_event_queue_push_back() function.
 *
 * Tries to free the event synchronously, if it was not delivered
 * to any subscriber.
 *
 * The library takes care of freeing the \p event once all
 * subscribed active objects handled it.
 * This is done asynchronously after this function returns.
 *
 * Statically allocated events, i.e. the events for which am_event_is_static()
 * returns true, are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param event   the event to publish
 * @param margin  the number of free event queue slots to be available in each
 *                subscribed active object after the event is pushed
 *                to their event queues
 *
 * @retval true   the event was delivered to all subscribed active objects
 * @retval false  at least one delivery has failed
 */
bool am_ao_publish_x(const struct am_event *event, int margin);

/**
 * Publish \p event to all subscribed active objects.
 *
 * Same as am_ao_publish_x() except this function
 * crashes with assert, if it fails delivering the \p event to at
 * least one subscribed active object.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param event  the event to publish
 */
void am_ao_publish(const struct am_event *event);

/**
 * Post \p event to the back of active object's event queue (eXtended version).
 *
 * The \p event is then handled asynchronously by the active object.
 *
 * Guarantees availability of \p margin free slots in destination event queue
 * after the \p event was delivered to the active object.
 *
 * Tries to free the \p event synchronously, if it was not posted.
 *
 * The library takes care of freeing the \p event once the
 * active object handled the \p event.
 * This is done asynchronously after this function returns.
 *
 * Statically allocated events, i.e. events for which am_event_is_static()
 * returns true are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param ao      the event is posted to this active object
 * @param event   the event to post
 * @param margin  the number of free event queue slots to be available
 *                after event was posted
 *
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_ao_post_fifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
);

/**
 * Post \p event to the back of active object's event queue.
 *
 * Same as am_ao_post_fifo_x() except this function
 * crashes with assert, if it fails delivering the \p event to the active
 * object.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param ao     the event is posted to this active object
 * @param event  the event to post
 */
void am_ao_post_fifo(struct am_ao *ao, const struct am_event *event);

/**
 * Post \p event to the front of AO event queue (eXtended version).
 *
 * The \p event is then handled asynchronously by the active object.
 *
 * If active object's event queue is full and \p margin is >0,
 * then the function fails gracefully.
 *
 * Tries to free the \p event synchronously, if it was not posted.
 *
 * The library takes care of freeing the \p event once the
 * active object handled the \p event.
 * This is done asynchronously after this function returns.
 *
 * Statically allocated events, i.e. events for which am_event_is_static()
 * returns true, are never freed.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param ao      the event is posted to this active object
 * @param event   the event to post
 * @param margin  the number of free event queue slots to be available
 *                after event was posted
 *
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_ao_post_lifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
);

/**
 * Post \p event to the front of active object's event queue.
 *
 * Same as am_ao_post_lifo_x() except this function
 * crashes with assert, if it fails delivering the \p event to the active
 * object.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param ao     the event is posted to this active object
 * @param event  the event to post
 */
void am_ao_post_lifo(struct am_ao *ao, const struct am_event *event);

/**
 * Active object constructor.
 *
 * @param ao     the active object to construct
 * @param state  the initial state of the active object
 */
void am_ao_ctor(struct am_ao *ao, struct am_hsm_state state);

/**
 * Start active object.
 *
 * Starts managing the active object as part of application.
 *
 * The safest is to start active objects in the order of their priority,
 * beginning from the lowest priority active objects because they tend
 * to have bigger event queues.
 *
 * @param ao          the active object to start
 * @param prio        priority
 * @param queue       the active object's event queue
 * @param queue_size  the event queue size [sizeof(struct am_event *)]
 * @param stack       active object stack
 * @param stack_size  the stack size [bytes]
 * @param name        human readable name of active object.
 *                    Not copied. Must remain valid after the call.
 *                    Can be NULL.
 * @param init_event  init event. Can be NULL.
 *                    The event is not freed. The caller is responsible
 *                    for freeing the event after the call.
 */
void am_ao_start(
    struct am_ao *ao,
    struct am_ao_prio prio,
    const struct am_event *queue[],
    int queue_size,
    void *stack,
    int stack_size,
    const char *name,
    const struct am_event *init_event
);

/**
 * Stop active object.
 *
 * Can only be called by the active object itself.
 * The active object is expected to release all allocated
 * resources before calling this function.
 *
 * @param ao  the active object to stop
 */
void am_ao_stop(struct am_ao *ao);

/**
 * Active object library state constructor.
 *
 * @param cfg  active object library configuration
 *             The active object library makes an internal copy of
 *             the configuration. Can be NULL.
 */
void am_ao_state_ctor(const struct am_ao_state_cfg *cfg);

/**
 * Active object library state destructor.
 */
void am_ao_state_dtor(void);

/**
 * Subscribe active object to \p event ID.
 *
 * The \p event ID must be smaller than the number of elements
 * in the array of active object subscribe lists provided to
 * am_ao_init_subscribe_list().
 *
 * @param ao     active object to subscribe
 * @param event  the event ID to subscribe to
 */
void am_ao_subscribe(const struct am_ao *ao, int event);

/**
 * Unsubscribe active object from \p event ID.
 *
 * The \p event ID must be smaller than the number of elements
 * in the array of active object subscribe lists provided to
 * am_ao_init_subscribe_list().
 *
 * @param ao     active object to unsubscribe
 * @param event  the event ID to unsubscribe from
 */
void am_ao_unsubscribe(const struct am_ao *ao, int event);

/**
 * Unsubscribe active object from all events.
 *
 * @param ao  active object to unsubscribe
 */
void am_ao_unsubscribe_all(const struct am_ao *ao);

/**
 * Initialize active object global subscribe list.
 *
 * Optional. Only needed, if active object pub/sub functionality is used.
 * The pub/sub functionality is provided by
 * am_ao_publish(), am_ao_publish_x(),
 * am_ao_publish_exclude(), am_ao_publish_exclude_x(),
 * am_ao_subscribe(), am_ao_unsubscribe() and am_ao_unsubscribe_all() APIs.
 *
 * @param sub   the array of active object subscribe lists
 * @param nsub  the number of elements in sub array
 */
void am_ao_init_subscribe_list(struct am_ao_subscribe_list *sub, int nsub);

/**
 * Run all active objects.
 *
 * Blocks for preemptive AO library build and returns when all active objects
 * were stopped.
 *
 * What follows only applies to cooperative library build of AO.
 *
 * Executes initial transition of all newly started active objects.
 *
 * Non blocking and returns after dispatching zero or one event.
 *
 * The function is expected to be called repeatedly to dispatch
 * events to active objects.
 *
 * If no events were dispatched (the function returned false),
 * then the event processor is in idle state.
 *
 * @retval true   dispatched one event
 * @retval false  dispatched no events.
 *                Call am_ao_get_cnt() to make sure there are still
 *                running active objects available.
 */
bool am_ao_run_all(void);

/**
 * Check if active object event queue is empty.
 *
 * Used for debugging.
 *
 * @param ao  check the event queue of this active object
 *
 * @retval true   the queue is empty
 * @retval false  the queue is not empty
 */
bool am_ao_event_queue_is_empty(struct am_ao *ao);

/**
 * Log the content of the first \p num events in each event queue of every AO.
 *
 * Used for debugging.
 *
 * Not thread safe.
 *
 * @param num  the number of events to log. Use -1 to log all events.
 * @param log  the logging callback
 */
void am_ao_log_event_queues_unsafe(
    int num,
    void (*log)(
        const char *name, int i, int len, int cap, const struct am_event *event
    )
);

/**
 * Log last event of every active object.
 *
 * Used for debugging.
 *
 * @param log  the logging callback
 */
void am_ao_log_last_events(void (*log)(const char *name, int event));

/**
 * Block until all active objects are ready to run.
 *
 * Prevents using active objects before they are ready to
 * process events.
 *
 * To be run once at the start of regular (non-AO) user tasks created with
 * am_task_create() API. These regular user tasks are typically
 * used to execute blocking calls and post/publish events to active objects.
 */
void am_ao_wait_start_all(void);

/**
 * Get number of running active objects.
 *
 * @return the number of running active objects.
 */
int am_ao_get_cnt(void);

/**
 * Get active object own priority level.
 *
 * @return the priority level
 */
int am_ao_get_own_prio(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_AO_H_INCLUDED */
