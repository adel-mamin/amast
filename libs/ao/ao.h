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

#include "common/compiler.h"
#include "common/macros.h"
#include "event/event.h"
#include "queue/queue.h"
#include "hsm/hsm.h"
#include "pal/pal.h"

#ifndef AM_AO_EVT_PUB_MAX
/**
 * All pub/sub event IDs must be smaller or equal to this constants.
 *
 * Users using pub/sub events with IDs bigger that this constant
 * are expected to override it with a custom value.
 */
#define AM_AO_EVT_PUB_MAX AM_EVT_USER
#endif

/** The active object. */
struct am_ao {
    struct am_hsm hsm;           /**< the state machine member */
    int prio;                    /**< the priority of active object */
    const char *name;            /**< the human readable name of AO */
    struct am_queue event_queue; /**< the event queue */
    int last_event;              /**< last processed event */
    int task_id;                 /**< task handle */
    bool stopped;                /**< 1: AO was stopped, 0: AO wasn't stopped */
    /** initial user event - the parameter of am_ao_start() API */
    const struct am_event *init_event;
};

/** Active object library state configuration. */
struct am_ao_state_cfg {
    /** Debug callback. */
    void (*debug)(const struct am_ao *ao, const struct am_event *e);
    /**
     * Callback to enter low power mode
     *
     * The callback is called with critical section being entered
     * (struct am_ao_state_cfg::crit_enter()) to allow for race condition free
     * transition to low power mode(s).
     * The ao_state_cfg::crit_exit() is called by the library after the callback
     * is returned.
     *
     * Please read the article called
     * "Use an MCU's low-power modes in foreground/background systems"
     * by Miro Samek for more information about the reasoning of the approach.
     */
    void (*on_idle)(void);

    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);
};

/** The minimum AO priority level. */
#define AM_AO_PRIO_MIN 0
/** The maximum AO priority level. */
#define AM_AO_PRIO_MAX 63

/** Checks if active object priority is valid. */
#define AM_AO_PRIO_IS_VALID(ao) \
    ((AM_AO_PRIO_MIN <= (ao)->prio) && ((ao)->prio <= AM_AO_PRIO_MAX))

#ifndef AM_AO_NUM_MAX
/** The maximum number of active objects. */
#define AM_AO_NUM_MAX 64
#endif

AM_ASSERT_STATIC(AM_AO_NUM_MAX <= AM_PAL_TASK_NUM_MAX);

/** The subscribe list for one event. */
struct am_ao_subscribe_list {
    uint8_t list[AM_DIV_CEIL(AM_AO_NUM_MAX, 8)]; /**< the bitmask */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Publish event.
 *
 * The event is delivered to event queues of all the active objects,
 * which are subscribed to the event ID.
 * The event is then handled asynchronously by the active objects.
 *
 * Use am_ao_subscribe() to subscribe an active object to an event ID.
 * Use am_ao_unsubscribe() to unsubscribe it from the event ID
 * or am_ao_unsubscribe_all() to unsubscribe it from all event IDs.
 *
 * If any active object has full event queue and cannot
 * accommodate the event, then the function crashes with assert.
 *
 * If your application is prepared for loosing the event,
 * then use am_ao_publish_x() function instead.
 *
 * Internally the event is added to subscribed active object event queues
 * using am_event_push_back() function.
 *
 * Tries to free the event, if no active objects are subscribed to it.
 *
 * The library takes care of freeing the event once all
 * subscribed active objects handled the event.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * @param event  the event to publish
 */
void am_ao_publish(const struct am_event *event);

/**
 * Same as am_ao_publish(), but the event is not delivered to the given AO.
 *
 * @param event  the event to publish
 * @param ao     do not post the event to this active object even
 *               if it is subscribed to the event.
 *               If set to NULL, the the API behaves same way as
 *               am_ao_publish()
 */
void am_ao_publish_exclude(
    const struct am_event *event, const struct am_ao *ao
);

/**
 * Publish event.
 *
 * The event is delivered to event queues of all the active objects,
 * which are subscribed to the event ID.
 * The event is then handled asynchronously by the active objects.
 *
 * Use am_ao_subscribe() to subscribe an active object to an event ID.
 * Use am_ao_unsubscribe() or am_ao_unsubscribe_all() to unsubscribe it
 * from the event ID.
 *
 * If any active object has full event queue and cannot
 * accommodate the event, then the function skips the event delivery
 * to the active object.
 *
 * If your application is not prepared for loosing the event,
 * then use am_ao_publish() function instead.
 *
 * Internally the event is added to subscribed active object event queues
 * using am_event_push_back() function.
 *
 * Tries to free the event, if it was not delivered to any subscriber.
 *
 * The library takes care of freeing the event once all
 * subscribed active objects handled the event.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * @param event   the event to publish
 * @param margin  free event queue slots to be available in each subscribed
 *                active object after the event is pushed to their event queues
 *
 * @retval true   the event was delivered to all subscribed active objects
 * @retval false  at least one delivery has failed
 */
bool am_ao_publish_x(const struct am_event *event, int margin);

/**
 * Same as am_ao_publish_x(), but the event is not delivered to the given AO.
 *
 * @param event   the event to publish
 * @param margin  free event queue slots to be available in each subscribed
 *                active object after the event is pushed to their event queues
 * @param ao      do not post the event to this active object even
 *                if it is subscribed to the event.
 *                If set to NULL, the the API behaves same way as
 *                am_ao_publish_x()
 *
 * @retval true   the event was delivered to all subscribed active objects
 *                except the active object given as the parameter
 * @retval false  at least one delivery has failed
 */
bool am_ao_publish_x_exclude(
    const struct am_event *event, int margin, const struct am_ao *ao
);

/**
 * Post event to the back of active object's event queue.
 *
 * The event is then handled asynchronously by the active object.
 *
 * If the active object's event queue is full the function crashes with assert.
 *
 * The library takes care of freeing the event once the
 * active object handled the event.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * @param ao     the event is posted to this active object
 * @param event  the event to post
 */
void am_ao_post_fifo(struct am_ao *ao, const struct am_event *event);

/**
 * Post event to the back of active object's event queue.
 *
 * The event is then handled asynchronously by the active object.
 *
 * If the active object's event queue is full and margin is >0,
 * then the function fails gracefully.
 *
 * Tries to free the event, if it was not posted.
 *
 * The library takes care of freeing the event once the
 * active object handled the event.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * @param ao      the event is posted to this active object
 * @param event   the event to post
 * @param margin  free event queue slots to be available after event was posted
 *
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_ao_post_fifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
);

/**
 * Post event to the front of active object's event queue.
 *
 * The event is then handled asynchronously by the active object.
 *
 * The library takes care of freeing the event once the
 * active object handled the event.
 *
 * If active object's event queue is full the function crashes with assert.
 *
 * The function is fast, thread safe and usable from
 * interrupt service routines (ISR).
 *
 * @param ao     the event is posted to this active object
 * @param event  the event to post
 */
void am_ao_post_lifo(struct am_ao *ao, const struct am_event *event);

/**
 * Post event to the front of AO event queue.
 *
 * The event is then handled asynchronously by the active object.
 *
 * If active object's event queue is full and margin is >0,
 * then the function fails gracefully.
 *
 * Tries to free the event, if it was not posted.
 *
 * The library takes care of freeing the event once the
 * active object handled the event.
 *
 * Statically allocated events (events for which am_event_is_static()
 * returns true) are never freed.
 *
 * @param ao      the event is posted to this active object
 * @param event   the event to post
 * @param margin  free event queue slots to be available after event was posted
 *
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_ao_post_lifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
);

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
 * Start managing the active object as part of application.
 *
 * The safest is to start active objects in the order of their priority,
 * beginning from the lowest priority active objects because they tend
 * to have the bigger event queues.
 *
 * @param ao          the active object to start
 * @param prio        priority level [AM_AO_PRIO_MIN, AM_AO_PRIO_MAX]
 * @param queue       the active object's event queue
 * @param queue_size  the event queue size [sizeof(struct am_event*)]
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
    int prio,
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
 * @param ao  the active object to stop
 */
void am_ao_stop(struct am_ao *ao);

/**
 * Active object library state constructor.
 *
 * @param cfg  active object library configuration
 *             The active object module makes an internal copy of
 *             the configuration.
 */
void am_ao_state_ctor(const struct am_ao_state_cfg *cfg);

/** Destroy the internal state of active object library */
void am_ao_state_dtor(void);

/**
 * Subscribe active object to event ID.
 *
 * @param ao     active object to subscribe
 * @param event  the event ID to subscribe to
 */
void am_ao_subscribe(const struct am_ao *ao, int event);

/**
 * Unsubscribe active object from event ID.
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
 * Optional. Only needed if active object pub/sub functionality is used.
 * The pub/sub functionality is provided by
 * am_ao_publish(), am_ao_publish_x(),
 * am_ao_subscribe(), am_ao_unsubscribe() and am_ao_unsubscribe_all() APIs.
 *
 * @param sub   the array of active object subscribe lists
 * @param nsub  the number of elements in sub array
 */
void am_ao_init_subscribe_list(struct am_ao_subscribe_list *sub, int nsub);

/**
 * Run all active objects.
 *
 * Executes initial transition of all active objects once on
 * the first call.
 *
 * Blocks forever for preemptive AO build.
 *
 * What follows only applies to cooperative build of AO.
 *
 * Non blocking and returns after dispatching zero or one event.
 *
 * The function is expected to be called repeatedly to dispatch
 * events to active objects.
 *
 * If zero events were dispatched (the function returned false),
 * then the event processor is in idle state.
 *
 * @retval true   dispatched one event
 * @retval false  dispatched no events
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
 * Log the content of the first num events in each event queue of every AO.
 *
 * Used for debugging.
 *
 * @param num  the number of events to log. Use -1 to log all events.
 * @param log  the logging callback
 */
void am_ao_dump_event_queues(
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
 * To be run once at the start of regular user threads started with
 * am_pal_task_create() API running blocking calls and using
 * active objects for event posting/publishing.
 */
void am_ao_wait_startup(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_AO_H_INCLUDED */
