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

#ifndef AM_AO_EVT_PUB_MAX
#define AM_AO_EVT_PUB_MAX AM_EVT_USER
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** The active object. */
struct am_ao {
    struct am_hsm hsm;           /**< the state machine member */
    int prio;                    /**< the priority of active object */
    const char *name;            /**< the human readable name of AO */
    struct am_queue event_queue; /**< the event queue */
    int last_event;              /**< last processed event */
    void *task;                  /**< task handle */
};

/** AO state configuration. */
struct am_ao_state_cfg {
    /** User callback on idle state, when no AO is running. */
    void (*on_idle)(void);
    /** Debug callback. */
    void (*debug)(const struct am_ao *ao, const struct am_event *e);

    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);
};

/** The minumum AO priority level. */
#define AM_AO_PRIO_MIN 0
/** The maximum AO priority level. */
#define AM_AO_PRIO_MAX 63

/** Checks if active object priority is valid. */
#define AM_AO_PRIO_IS_VALID(ao) \
    ((AM_AO_PRIO_MIN <= (ao)->prio) && ((ao)->prio <= AM_AO_PRIO_MAX))

#ifndef AM_AO_NUM_MAX
#define AM_AO_NUM_MAX 64
#endif

AM_ASSERT_STATIC(AM_AO_NUM_MAX <= 64);

/** The subscribe list for one event. */
struct am_ao_subscribe_list {
    uint8_t list[AM_DIVIDE_ROUND_UP(AM_AO_NUM_MAX, 8)]; /* the bitmask */
};

/**
 * Publish event.
 *
 * @param event  the event to publish
 */
void am_ao_publish(const struct am_event *event);

/**
 * Publish event.
 *
 * @param event   the event to publish
 * @param margin  free event queue slots to be available after event is pushed
 * @retval true   success
 * @retval false  at least one delivery has failed
 */
bool am_ao_publish_x(const struct am_event *event, int margin);

/**
 * Post event to the back of AO event queue.
 *
 * If AO event queue is full the API asserts.
 * The event is handled asynchronously.
 *
 * @param ao      the event is posted to this AO
 * @param event   the event to post
 */
void am_ao_post_fifo(struct am_ao *ao, const struct am_event *event);

/**
 * Post event to the back of AO event queue.
 *
 * If AO event queue is full and margin is >0 the API fails gracefully.
 * The event is handled asynchronously.
 *
 * @param ao      the event is posted to this AO
 * @param event   the event to post
 * @param margin  free event queue slots to be available after event is posted
 * @retval true   the event was posted
 * @retval false  the event was not posted
 */
bool am_ao_post_fifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
);

/**
 * Post event to the front of AO event queue.
 *
 * If AO event queue is full the API asserts.
 * The event is handled asynchronously.
 *
 * @param ao      the event is posted to this AO
 * @param event   the event to post
 */
void am_ao_post_lifo(struct am_ao *ao, const struct am_event *event);

/**
 * Post event to the front of AO event queue.
 *
 * If AO event queue is full and margin is >0 the API fails gracefully.
 * The event is handled asynchronously.
 *
 * @param ao      the event is posted to this AO
 * @param event   the event to post
 * @param margin  free event queue slots to be available after event is posted
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
void am_ao_ctor(struct am_ao *ao, const struct am_hsm_state *state);

/**
 * Start an active object.
 *
 * The safest is to start active objects in the order of their priority,
 * beginning from the lowest priority active objects because they tend
 * to have the biggest event queues.
 *
 * @param ao          the active object to start
 * @param prio        priority level [0, AM_AO_PRIO_MAX]
 * @param queue       the active object's event queue
 * @param queue_size  the event queue size
 * @param stack       active object stack
 * @param stack_size  the stack size [bytes]
 * @param name        human readable name of active object
 * @param init_event  init event. Can be NULL. The event is not recycled.
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

void am_ao_stop(const struct am_ao *ao);

struct am_ao_cfg {
    /**
     * User callback for idle condition actions (e.g. sleep mode)
     * The interrupts are disabled when the callback is called.
     * The function must unlock interrupts internally, ideally
     * atomically with a transition to a sleep mode.
     */
    void (*on_idle)(void);
    /** Debug callback. */
    void (*debug)(const struct am_ao *ao, const struct am_event *e);
    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);
};

/**
 * AO state constructor.
 *
 * @param cfg  AO state configuration
 *             The AO module makes an internal copy of the configuration.
 */
void am_ao_state_ctor(const struct am_ao_state_cfg *cfg);

/** Destroys the internal state of AO */
void am_ao_state_dtor(void);

/**
 * Subscribes active object to event.
 *
 * @param ao     active object to subscribe
 * @param event  the event ID to subscribe to
 */
void am_ao_subscribe(const struct am_ao *ao, int event);

/**
 * Unsubscribes active object from the event.
 *
 * @param ao     active object to unsubscribe
 * @param event  the event ID to unsubscribe from
 */
void am_ao_unsubscribe(const struct am_ao *ao, int event);

/**
 * Unsubscribes active object from all events.
 *
 * @param ao  active object to unsubscribe
 */
void am_ao_unsubscribe_all(const struct am_ao *ao);

/**
 * Initialize the AO subscribe list.
 *
 * @param sub   the array of AO subscribe lists
 * @param nsub  the number of elements in sub array
 */
void am_ao_init_subscribe_list(struct am_ao_subscribe_list *sub, int nsub);

/**
 * Run all active objects.
 *
 * @param loop  run active objects in loop (true: do not return)
 *
 * @retval true   processed at least one event
 * @retval false  processed no events
 */
bool am_ao_run_all(bool loop);

/**
 * Log the content of the first num events in each event queue of every AO.
 *
 * @param num  the number of events to log
 * @param log  the logging callback
 */
void am_ao_dump_event_queues(
    int num, void (*log)(const char *name, int i, int len, int cap, int event)
);

/**
 * Log last event of every AO.
 *
 * @param log  the logging callback
 */
void am_ao_log_last_events(void (*log)(const char *name, int event));

#ifdef __cplusplus
}
#endif

#endif /* AM_AO_H_INCLUDED */
