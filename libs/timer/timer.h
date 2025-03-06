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
 * Timer API declaration.
 */

#ifndef AM_TIMER_H_INCLUDED
#define AM_TIMER_H_INCLUDED

#include <stdbool.h>

#include "common/macros.h"
#include "event/event.h"
#include "slist/slist.h"
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

/**
 * Timer module state configuration.
 *
 * Either post or publish callback must be non-NULL.
 */
struct am_timer_state_cfg {
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
     * Update the content of the given timer.
     *
     * Optional, can be NULL.
     *
     * @param timer  the timer to update
     *
     * @return the updated timer
     */
    struct am_timer *(*update)(struct am_timer *timer);

    /** Enter critical section. */
    void (*crit_enter)(void);

    /** Exit critical section. */
    void (*crit_exit)(void);
};

/** Timer. */
struct am_timer {
    /** event descriptor */
    struct am_event event;

    /** to link timers together */
    struct am_slist_item item;

    /** the object, which receives the timer event */
    void *owner;

    /** the timer event is sent after this many ticks */
    int shot_in_ticks;

    /** the timer event is re-sent after this many ticks */
    int interval_ticks;

    /** the timer was disarmed and pending removal from timer list */
    unsigned disarm_pending : 1;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Timer state constructor.
 *
 * @param cfg  timer state configuration.
 *             The timer module makes an internal copy of the configuration.
 */
void am_timer_state_ctor(const struct am_timer_state_cfg *cfg);

/**
 * Timer constructor.
 *
 * @param timer   the timer to construct
 * @param id      the timer event identifier
 * @param domain  tick clock domain the timer belongs to.
 *                The valid range is [0-#AM_PAL_TICK_DOMAIN_MAX[.
 * @param owner   the timer's owner, which receives the posted event.
 *                Can be NULL, in which case the timer event is published.
 */
void am_timer_ctor(struct am_timer *timer, int id, int domain, void *owner);

/**
 * Allocate and construct timer.
 *
 * Cannot fail. Cannot be freed. Never garbage collected.
 * The returned timer is fully constructed.
 * No need to call am_timer_ctor() for it.
 *
 * Provides an alternative way to reserve memory for timers in
 * addition to static allocation in user code.
 *
 * Allocation of timer using this API is preferred as it
 * improves cache locality of timer structures.
 *
 * @param id      the timer event id
 * @param size    the timer size [bytes]
 * @param domain  the clock domain.
 *                The valid range is [0-#AM_PAL_TICK_DOMAIN_MAX[.
 * @param owner   the timer's owner, which receives the posted event.
 *                Can be NULL, in which case the timer event is published.
 */
struct am_timer *am_timer_allocate(int id, int size, int domain, void *owner);

/**
 * Tick timer.
 *
 * Update all armed timers and fire expired timers.
 * Must be called every tick.
 *
 * @param domain  only tick timers in this tick domain.
 *                The valid range is [0-#AM_PAL_TICK_DOMAIN_MAX[.
 */
void am_timer_tick(int domain);

/**
 * Arm timer.
 *
 * Sends timer event to owner in specified number of ticks.
 *
 * It is fine to arm an already armed timer.
 *
 * The owner is set in am_timer_ctor() or am_timer_allocate() calls.
 *
 * @param timer     the timer to arm
 * @param ticks     the timer event is to be sent in these many ticks
 * @param interval  the timer event is to be re-sent in these many ticks
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the timer is one shot.
 */
void am_timer_arm_ticks(struct am_timer *timer, int ticks, int interval);

/**
 * Arm timer.
 *
 * Sends timer event to owner in specified number of milliseconds.
 *
 * It is fine to arm an already armed timer.
 *
 * The owner is set in am_timer_ctor() or am_timer_allocate() calls.
 *
 * @param timer     the timer to arm
 * @param ms        the timer event is to be sent in these many milliseconds
 * @param interval  the timer event is to be re-sent in these many milliseconds
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the timer is one shot.
 */
void am_timer_arm_ms(struct am_timer *timer, int ms, int interval);

/**
 * Disarm timer.
 *
 * It is fine to disarm an already disarmed timer.
 *
 * @param timer  the timer to disarm
 *
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool am_timer_disarm(struct am_timer *timer);

/**
 * Check if timer is armed.
 *
 * @param timer  the timer to check
 *
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool am_timer_is_armed(const struct am_timer *timer);

/**
 * Check if timer domain has armed timers.
 *
 * The function is to be called from a critical section.
 * So, it can be called to check if there are any pending timers
 * just before going to sleep mode.
 *
 * Please read the article called
 * "Use an MCU's low-power modes in foreground/background systems"
 * by Miro Samek for more information about the reasoning of the approach.
 *
 * @param domain  the domain to check.
 *                The valid range is [0-#AM_PAL_TICK_DOMAIN_MAX[.
 *
 * @retval true   the timer domain is empty
 * @retval false  the timer domain has armed timers
 */
bool am_timer_domain_is_empty(int domain);

/**
 * Get number of ticks till timer event is sent.
 *
 * @param timer  the timer handler
 *
 * @return the timer event is sent in this number of ticks
 */
int am_timer_get_ticks(const struct am_timer *timer);

/**
 * Get timer interval.
 *
 * @param timer  the timer handler
 *
 * @return the timer event is sent with this interval [ticks]
 */
int am_timer_get_interval(const struct am_timer *timer);

#ifdef __cplusplus
}
#endif

#endif /* AM_TIMER_H_INCLUDED */
