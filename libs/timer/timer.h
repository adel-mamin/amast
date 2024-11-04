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

#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "common/compiler.h"
#include "event/event.h"
#include "dlist/dlist.h"
#include "pal/pal.h"

AM_ASSERT_STATIC(AM_EVENT_TICK_DOMAIN_MASK >= AM_PAL_TICK_DOMAIN_MAX);

/** Timer module configuration. */
struct am_timer_cfg {
    /** either post or publish callback must be non-NULL */

    /**
     * Expired events are posted using this callback.
     * Posting is one-to-one event delivery mechanism.
     */
    void (*post)(void *owner, const struct am_event *event);
    /**
     * Expired events are published using this callback.
     * Publishing is one-to-many event delivery mechanism.
     */
    void (*publish)(const struct am_event *event);
    /**
     * Custom update the content of the event.
     * Optional, can be NULL.
     * @param event  the event to update
     * @return the updated event
     */
    struct am_event_timer *(*update)(struct am_event_timer *event);
};

/** Timer event. */
struct am_event_timer {
    /** event descriptor */
    struct am_event event;
    /** to link time events together */
    struct am_dlist_item item;
    /** the object, who receives the event */
    void *owner;
    /** the event is sent after this many ticks */
    int shot_in_ticks;
    /** the event is re-sent after this many ticks */
    int interval_ticks;
};

/**
 * Timer constructor.
 *
 * @param cfg  timer module configuration
 *             The timer module makes an internal copy of the configuration.
 */
void am_timer_ctor(const struct am_timer_cfg *cfg);

/**
 * Timer event constructor.
 *
 * @param event   the timer event to construct
 * @param id      the timer event identifier
 * @param domain  tick domain the event belongs to
 */
void am_timer_event_ctor(struct am_event_timer *event, int id, int domain);

/**
 * Allocate and construct timer event.
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
 */
struct am_event_timer *am_timer_event_allocate(int id, int size, int domain);

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
 * @param event     the timer event to arm
 * @param owner     the timer event's owner that gets the posted event.
 *                  Can be NULL, in which case the time event is published.
 * @param ticks     the timer event is to be sent in these many ticks
 * @param interval  the timer event is to be re-sent in these many ticks
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the event is one shot.
 */
void am_timer_arm(
    struct am_event_timer *event, void *owner, int ticks, int interval
);

/**
 * Disarm timer.
 *
 * @param event   the timer to disarm
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool am_timer_disarm(struct am_event_timer *event);

/**
 * Check if timer is armed.
 *
 * @param event   the timer to check
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool am_timer_is_armed(const struct am_event_timer *event);

/**
 * Check if timer domain has armed timers.
 *
 * @param domain  the domain to check
 * @retval true   the timer domain is empty
 * @retval false  the timer domain has armed timers
 */
bool am_timer_domain_is_empty(int domain);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_INCLUDED */
