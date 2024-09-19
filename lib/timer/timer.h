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

#include "common/compiler.h"
#include "event/event.h"
#include "common/macros.h"
#include "dlist/dlist.h"

/** Timer module configuration. */
struct am_timer_cfg {
    int (*ticks_to_ms)(int ticks);
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
    /* optional, can be NULL */
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

#ifndef AM_TICK_DOMAIN_MAX
/** total number of tick domains */
#define AM_TICK_DOMAIN_MAX 1
#endif

AM_ASSERT_STATIC(AM_TICK_DOMAIN_MAX < (1U << AM_EVENT_TICK_DOMAIN_BITS));

/**
 * Timer constructor.
 * @param cfg  timer module configuration
 */
void am_timer_ctor(const struct am_timer_cfg *cfg);

/**
 * Timer event constructor.
 * @param event   the timer event to construct
 * @param id      the timer event identifier
 * @param domain  tick domain the event belongs to
 */
void am_timer_event_ctor(struct am_event_timer *event, int id, int domain);

/**
 * Tick timer.
 * Update all armed timers and fire expired timer events.
 * @param domain only tick timers in this tick domain
 */
void am_timer_tick(int domain);

/**
 * Send timer event to owner in specified number of ticks.
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
 * @param event   the timer to disarm
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool am_timer_disarm(struct am_event_timer *event);

/**
 * Check if timer is armed.
 * @param event   the timer to check
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool am_timer_is_armed(const struct am_event_timer *event);

/**
 * Check if any timer is armed.
 * @param domain  check if any timer is armed in this tick domain
 *                If set to TICK_DOMAIN_MAX, then check if any timer
 *                is armed in any tick domain.
 * @retval true   there are armed timers
 * @retval false  there are no armed timers
 */
bool am_timer_any_armed(int domain);

/**
 * Convert ticks to milliseconds.
 * @param ticks  the ticks
 * @return the milliseconds
 */
int am_timer_ticks_to_ms(int ticks);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_INCLUDED */
