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
struct timer_cfg {
    /** either post or publish callback must be non-NULL */
    /**
     * Expired events are posted using this callback.
     * Posting is one-to-one event delivery mechanism.
     */
    void (*post)(void *owner, const struct event *event);
    /**
     * Expired events are published using this callback.
     * Publishing is one-to-many event delivery mechanism.
     */
    void (*publish)(const struct event *event);
    /* optional, can be NULL */
    struct event_timer *(*update)(struct event_timer *event);
};

/** Timer event. */
struct event_timer {
    /** event descriptor */
    struct event event;
    /** to link time events together */
    struct am_dlist_item item;
    /** the object, who receives the event */
    void *owner;
    /** the event is sent after this many ticks */
    int shot_in_ticks;
    /** the event is re-sent after this many ticks */
    int interval_ticks;
};

#ifndef TICK_DOMAIN_MAX
/** total number of tick domains */
#define TICK_DOMAIN_MAX 1
#endif

AM_ASSERT_STATIC(TICK_DOMAIN_MAX < (1U << EVENT_TICK_DOMAIN_BITS));

/** Timer module descriptor. */
struct timer {
    /**
     * Timer event domains.
     * Each domain comprises a list of the timer events,
     * which belong to this domain.
     */
    struct am_dlist domains[TICK_DOMAIN_MAX];
    /** timer module configuration */
    struct timer_cfg cfg;
};

/**
 * Timer constructor.
 * @param hnd  the timer module
 * @param cfg  timer module configuration
 */
void timer_ctor(struct timer *hnd, const struct timer_cfg *cfg);

/**
 * Timer event constructor.
 * @param event   the timer event to construct
 * @param id      the timer event identifier
 * @param domain  tick domain the event belongs to
 */
void timer_event_ctor(struct event_timer *event, int id, int domain);

/**
 * Tick timers.
 * Update all armed timers and fire expired timer events.
 * @param hnd    the timer module
 * @param domain only tick timers in this tick domain
 */
void timer_tick(struct timer *hnd, int domain);

/**
 * Post timer event to owner in specified number of ticks.
 * @param hnd    the timer module
 * @param event  the timer event to post
 * @param owner  the timer event's owner that gets the posted event
 * @param ticks  the timer event is to be posted in these many ticks
 */
void timer_post_in_ticks(
    struct timer *hnd, struct event_timer *event, void *owner, int ticks
);

/**
 * Publish timer event in specified number of ticks.
 * The timer event is then re-published every ticks time period.
 * @param hnd    the timer module
 * @param event  the timer event to publish
 * @param ticks  the timer event is to be published in these many ticks
 *               and then re-published every ticks
 */
void timer_publish_in_ticks(
    struct timer *hnd, struct event_timer *event, int ticks
);

/**
 * Post timer event to owner in specified number of ticks.
 * The timer event is then re-posted to owner every ticks time period.
 * @param hnd    the timer module
 * @param event  the timer event to post
 * @param owner  the timer event's owner that gets the posted event
 * @param ticks  the timer event is to be posted in these many ticks
 *               and then re-posted every ticks
 */
void timer_post_every_ticks(
    struct timer *hnd, struct event_timer *event, void *owner, int ticks
);

/**
 * Publish timer event in specified number of ticks.
 * The timer event is then re-published every ticks time period.
 * @param hnd    the timer module
 * @param event  the timer event to publish
 * @param ticks  the timer event is to be published in these many ticks
 *               and then re-published every ticks
 */
void timer_publish_every_ticks(
    struct timer *hnd, struct event_timer *event, int ticks
);

/**
 * Disarm timer.
 * @param event   the timer to disarm
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool timer_disarm(struct event_timer *event);

/**
 * Check if timer is armed.
 * @param event   the timer to check
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool timer_is_armed(const struct event_timer *event);

/**
 * Check if any timer is armed.
 * @param hnd     the timer module
 * @param domain  check if any timer is armed in this tick domain
 *                If set to TICK_DOMAIN_MAX, then check if any timer
 *                is armed in any tick domain.
 * @retval true   there are armed timers
 * @retval false  there are no armed timers
 */
bool timer_any_armed(const struct timer *hnd, int domain);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_INCLUDED */
