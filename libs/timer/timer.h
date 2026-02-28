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

/** Timer configuration. */
struct am_timer_cbs {
    /** Enter critical section. */
    void (*crit_enter)(void);

    /** Exit critical section. */
    void (*crit_exit)(void);
};

/** Timer state. */
struct am_timer {
    /** Running timers. */
    uint32_t timers_running;

    /** Allocated timers. */
    uint32_t timers_allocated;

    void (*crit_enter)(void); /**< Enter critical section. */
    void (*crit_exit)(void);  /**< Exit critical section. */

    void *events;   /**< Timer events */
    int events_num; /**< The number of timer events */
    int event_size; /**< Size of each event [bytes] */
};

/** Timer event. */
struct am_timer_event {
    /** event descriptor */
    struct am_event base;

    /** the timer event is sent after this many ticks */
    uint32_t oneshot_ticks;

    /** the timer event is re-sent after this many ticks */
    uint32_t interval_ticks;
};

/** Timer event with context. */
struct am_timer_event_x {
    struct am_timer_event base; /**< timer event */
    void *ctx;                  /**< the context */
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
 * @param timer       the timer state
 * @param events      the timer event pool
 * @param events_num  number of events in the timer event pool
 * @param event_size  the event size [bytes]
 */
void am_timer_ctor(
    struct am_timer *timer, void *events, int events_num, int event_size
);

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
 * Allocate and construct timer.
 *
 * Cannot fail. Cannot be freed. Never garbage collected.
 *
 * @param timer     the timer state
 * @param event_id  the timer event id
 *
 * @return timer index (tix)
 */
int am_timer_allocate(struct am_timer *timer, int event_id);

/**
 * Allocate and construct timer with context.
 *
 * Cannot fail. Cannot be freed. Never garbage collected.
 *
 * @param timer     the timer state
 * @param event_id  the timer event id
 * @param ctx       the context
 *
 * @return timer index (tix)
 */
int am_timer_allocate_x(struct am_timer *timer, int event_id, void *ctx);

/**
 * Tick timer state.
 *
 * Update all armed timers in the given timer state
 * and fire expired timers.
 *
 * Must be called every tick.
 *
 * @param timer  only tick timers in this timer state
 *
 * @return Bit map of timer indices
 */
uint32_t am_timer_tick(struct am_timer *timer);

/**
 * Get timer event reference from timer index.
 * @param timer  the timer state
 * @param tix    the timer event index as returned by
 *               am_timer_allocate() or am_timer_allocate_x()
 * @return the timer event reference
 */
struct am_timer_event *am_timer_from_tix(const struct am_timer *timer, int tix);

/**
 * Arm timer.
 *
 * It is fine to arm an already armed timer. The timer is re-armed in this case.
 * @param timer     the timer to arm
 * @param tix       the timer event index as returned by
 *                  am_timer_allocate() or am_timer_allocate_x()
 * @param ticks     the timer event is to be sent in these many ticks
 * @param interval  the timer event is to be re-sent in these many ticks
 *                  after the event is sent for the fist time.
 *                  Can be 0, in which case the timer is one shot.
 */
void am_timer_arm(
    struct am_timer *timer, int tix, uint32_t ticks, uint32_t interval
);

/**
 * Disarm timer.
 *
 * It is fine to disarm an already disarmed timer.
 *
 * @param timer   the timer state
 * @param tix     the index of the timer event to disarm as returned by
 *                am_timer_allocate() or am_timer_allocate_x()
 *
 * @retval true   the timer was armed
 * @retval false  the timer was not armed
 */
bool am_timer_disarm(struct am_timer *timer, int tix);

/**
 * Check if timer is armed.
 *
 * @param timer  the timer state
 * @param tix    the index of the timer event to check as returned by
 *               am_timer_allocate() or am_timer_allocate_x()
 *
 * @retval true   the timer is armed
 * @retval false  the timer is not armed
 */
bool am_timer_is_armed(const struct am_timer *timer, int tix);

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
 * @param tix    the index of the timer event to check as returned by
 *               am_timer_allocate() or am_timer_allocate_x()
 *
 * @return the timer event will be sent in this number of ticks
 */
uint32_t am_timer_get_ticks(const struct am_timer *timer, int tix);

/**
 * Get timer interval.
 *
 * @param timer  the timer handler
 * @param tix    the index of the timer event to check as returned by
 *               am_timer_allocate() or am_timer_allocate_x()
 *
 * @return the timer event is sent with this interval [ticks]
 */
uint32_t am_timer_get_interval(const struct am_timer *timer, int tix);

#ifdef __cplusplus
}
#endif

#endif /* AM_TIMER_H_INCLUDED */
