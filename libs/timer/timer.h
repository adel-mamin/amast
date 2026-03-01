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
#include <stdint.h>

#include "common/alignment.h"
#include "event/event.h"
#include "slist/slist.h"

/** Timer state. */
struct am_timer {
    /** A list of armed timer events. */
    struct am_slist events;
    /**
     * List of armed pending timer events.
     * Each armed timer is first placed into this list
     * and then moved to the list of armed timers events
     * in next am_timer_tick() call.
     *
     * This is done to limit the armed timer events list operations
     * exclusively to am_timer_tick() call to avoid race conditions
     * between timer owners the ticker task/ISR.
     */
    struct am_slist events_pend;
    /** number of timer events */
    struct {
        int16_t pend;    /**< armed pending timer events count */
        int16_t running; /**< armed timer events count */
    } nevents;

    /** Armed events iterator. */
    struct am_slist_iterator it;

    void (*crit_enter)(void); /**< Enter critical section. */
    void (*crit_exit)(void);  /**< Exit critical section. */
};

/** Timer event. */
struct am_timer_event {
    /** event descriptor */
    struct am_event event;

    /** to link timers together */
    struct am_slist_item item;

    /** the timer event is sent after this many ticks */
    uint32_t oneshot_ticks;

    /** the timer event is re-sent after this many ticks */
    uint32_t interval_ticks : 31;

    /** the timer was disarmed and pending removal from timer list */
    unsigned disarm_pending : 1;
};

/** Time event with context. */
struct am_timer_event_x {
    /** Base event structure. */
    struct am_timer_event event;
    /** Event specific context. */
    void *ctx;
};

/** To use with AM_ALIGNOF() macro. */
typedef struct am_timer_event am_timer_event_t;

/** To use with AM_ALIGNOF() macro. */
AM_ALIGNOF_DEFINE(am_timer_event_t);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Timer state constructor.
 *
 * @param timer  the timer state
 */
void am_timer_ctor(struct am_timer *timer);

/**
 * Timer event constructor.
 *
 * @param id   the event ID
 *
 * @return the constructed event
 */
static inline struct am_timer_event am_timer_event_ctor(uint16_t id) {
    return (struct am_timer_event){.event = {.id = id}};
}

/**
 * Extended timer event constructor.
 *
 * @param id   the event ID
 * @param ctx  the event context
 *
 * @return the constructed event
 */
static inline struct am_timer_event_x am_timer_event_ctor_x(
    uint16_t id, void *ctx
) {
    struct am_timer_event_x e = {0};
    e.event.event.id = id;
    e.ctx = ctx;
    return e;
}

/**
 * Register callbacks with timer state.
 *
 * @param timer  the timer state
 * @param crit_enter  enter critical section
 * @param crit_exit  exit critical section
 */
void am_timer_register_cbs(
    struct am_timer *timer, void (*crit_enter)(void), void (*crit_exit)(void)
);

/**
 * Initialize tick iterator.
 *
 * Must be called exactly once every ticks and must precede
 * am_timer_tick_iterator_next() calls.
 *
 * @param timer  timer state
 */
void am_timer_tick_iterator_init(struct am_timer *timer);

/**
 * Iterate tick to next timer event.
 *
 * Must follow am_timer_tick_iterator_init() call.
 * Must be called every tick until it retuns a non-null value.
 *
 * @param timer  tick timers in this timer state
 *
 * @return Fired timer event or NULL.
 */
struct am_timer_event *am_timer_tick_iterator_next(struct am_timer *timer);

/**
 * Arm timer.
 *
 * It is fine to arm an already armed timer. The timer is re-armed in this case.
 * @param timer     the timer to arm
 * @param event     the timer event
 * @param ticks     the timer event is to be sent in these many ticks
 * @param interval  the timer event is to be re-sent in these many ticks
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the timer is one shot.
 */
void am_timer_arm(
    struct am_timer *timer,
    struct am_timer_event *event,
    uint32_t ticks,
    uint32_t interval
);

/**
 * Disarm timer.
 *
 * It is fine to disarm an already disarmed timer.
 *
 * @param timer   the timer state
 * @param event   the timer event
 *
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool am_timer_disarm(struct am_timer *timer, struct am_timer_event *event);

/**
 * Check if timer is armed.
 *
 * @param timer  the timer state
 * @param event  the timer event
 *
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool am_timer_is_armed(
    const struct am_timer *timer, const struct am_timer_event *event
);

/**
 * Check if timer state has armed timers.
 *
 * The function is to be called from a critical section.
 *
 * Can be used to check if there are any pending timers
 * just before going to sleep mode while in critical section.
 * As it does not enter critical section internally there is no
 * risk of recursive critical section entry.
 *
 * Please read the article called
 * "Use an MCU's low-power modes in foreground/background systems"
 * by Miro Samek for more information about the reasoning why
 * critical section is not used in the function implementation.
 *
 * @param timer  the timer state
 *
 * @retval true   the timer state has no armed timers
 * @retval false  the timer state has armed timers
 */
bool am_timer_is_empty_unsafe(const struct am_timer *timer);

/**
 * Get number of ticks till timer event is sent.
 *
 * @param timer  the timer handler
 * @param event  the timer event
 *
 * @return the timer event will be sent in this number of ticks
 */
uint32_t am_timer_get_ticks(
    const struct am_timer *timer, const struct am_timer_event *event
);

#ifdef __cplusplus
}
#endif

#endif /* AM_TIMER_H_INCLUDED */
