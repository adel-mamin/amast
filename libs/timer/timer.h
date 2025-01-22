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

#ifndef AM_TIMER_H_INCLUDED
#define AM_TIMER_H_INCLUDED

#include <stdbool.h>

#include "common/macros.h"
#include "event/event.h"
#include "dlist/dlist.h"
#include "pal/pal.h"

AM_ASSERT_STATIC(AM_EVENT_TICK_DOMAIN_MASK >= AM_PAL_TICK_DOMAIN_MAX);

/**
 * Expired timer events are posted using this callback.
 * Posting is a one-to-one event delivery mechanism.
 */
typedef void (*am_timer_post_fn)(void *owner, const struct am_event *event);

/**
 * Expired timer events are published using this callback.
 * Publishing is a one-to-many event delivery mechanism.
 */
typedef void (*am_timer_publish_fn)(const struct am_event *event);

/** Timer module state configuration. */
struct am_timer_state_cfg {
    /** either post or publish callback must be non-NULL */

    /**
     * Expired timer events are posted using this callback.
     * Posting is a one-to-one event delivery mechanism.
     */
    am_timer_post_fn post;

    /**
     * Expired timer events are published using this callback.
     * Publishing is a one-to-many event delivery mechanism.
     */
    am_timer_publish_fn publish;

    /**
     * Update the content of the given timer event.
     *
     * Optional, can be NULL.
     *
     * @param event  the timer event to update
     *
     * @return the updated timer event
     */
    struct am_event_timer *(*update)(struct am_event_timer *event);

    /** Enter critical section. */
    void (*crit_enter)(void);

    /** Exit critical section. */
    void (*crit_exit)(void);
};

/** Timer event. */
struct am_event_timer {
    /** event descriptor */
    struct am_event event;

    /** to link timer events together */
    struct am_dlist_item item;

    /** the object, who receives the timer event */
    void *owner;

    /** the timer event is sent after this many ticks */
    int shot_in_ticks;

    /** the timer event is re-sent after this many ticks */
    int interval_ticks;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Timer state constructor.
 *
 * @param cfg  timer state configuration
 *             The timer module makes an internal copy of the configuration.
 */
void am_timer_state_ctor(const struct am_timer_state_cfg *cfg);

/**
 * Timer event constructor.
 *
 * @param event   the timer event to construct
 * @param id      the timer event identifier
 * @param domain  tick domain the event belongs to
 * @param owner   the timer event's owner that gets the posted event.
 *                Can be NULL, in which case the timer event is published.
 */
void am_timer_event_ctor(
    struct am_event_timer *event, int id, int domain, void *owner
);

/**
 * Allocate and construct timer event.
 *
 * Cannot fail. Cannot be freed. Never garbage collected.
 * The returned timer event is fully constructed.
 * No need to call am_timer_event_ctor() for it.
 * Provides an alternative way to reserve memory for timer events in
 * addition to the static allocation in user code.
 * Allocation of timer event using this API is preferred as it
 * improves cache locality of timer event structures.
 *
 * @param id      the timer event id
 * @param size    the timer event size [bytes]
 * @param domain  the clock domain [0-AM_PAL_TICK_DOMAIN_MAX[
 * @param owner   the timer event's owner that gets the posted event.
 *                Can be NULL, in which case the timer event is published.
 */
struct am_event_timer *am_timer_event_allocate(
    int id, int size, int domain, void *owner
);

/**
 * Tick timer.
 *
 * Update all armed timers and fire expired timer events.
 * Must be called every tick.
 *
 * @param domain  only tick timers in this tick domain
 */
void am_timer_tick(int domain);

/**
 * Send timer event to owner in specified number of ticks.
 *
 * The owner is set in am_timer_event_ctor() or am_timer_event_allocate() calls.
 *
 * @param event     the timer event to arm
 * @param ticks     the timer event is to be sent in these many ticks
 * @param interval  the timer event is to be re-sent in these many ticks
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the timer event is one shot.
 */
void am_timer_arm(struct am_event_timer *event, int ticks, int interval);

/**
 * Disarm timer event.
 *
 * @param event   the timer event to disarm
 *
 * @retval true   the timer event was armed
 * @retval false  the timer event was not armed
 */
bool am_timer_disarm(struct am_event_timer *event);

/**
 * Check if timer event is armed.
 *
 * @param event   the timer event to check
 *
 * @retval true   the timer event is armed
 * @retval false  the timer event is not armed
 */
bool am_timer_is_armed(const struct am_event_timer *event);

/**
 * Check if timer domain has armed timer events.
 *
 * @param domain  the domain to check
 *
 * @retval true   the timer domain is empty
 * @retval false  the timer domain has armed timer events
 */
bool am_timer_domain_is_empty(int domain);

#ifdef __cplusplus
}
#endif

#endif /* AM_TIMER_H_INCLUDED */
