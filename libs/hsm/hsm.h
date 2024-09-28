/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Ryan Hartlage (documentation)
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
 * Hierarchical State Machine (HSM) framework API declaration.
 * Configuration defines:
 * AM_HSM_SPY - enables HSM spy callback support for debugging
 */

#ifndef HSM_H_INCLUDED
#define HSM_H_INCLUDED

#include <stdbool.h>
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Empty event.
 * Should not cause any side effects in event handlers.
 * The event handlers must always return the AM_HSM_SUPER() in response
 * to this event.
 */
#define AM_HSM_EVT_EMPTY 1

/**
 * Init event.
 * Run initial transition from a given state.
 * Always follows the #AM_HSM_EVT_ENTRY event.
 */
#define AM_HSM_EVT_INIT 2

/**
 * Entry event.
 * Run entry action(s) for a given state.
 * Always precedes the #AM_HSM_EVT_INIT event.
 * No state transition is allowed in response to this event.
 */
#define AM_HSM_EVT_ENTRY 3

/**
 * Exit event.
 * Run exit action(s) for a given state.
 * No state transition is allowed in response to this event.
 */
#define AM_HSM_EVT_EXIT 4

/** HSM event with maximum value. */
#define AM_HSM_EVT_MAX AM_HSM_EVT_EXIT

AM_ASSERT_STATIC(AM_EVT_USER > AM_HSM_EVT_MAX);

/**
 * HSM state handler return codes.
 * These return codes are not used directly in user code.
 * Instead user code is expected to use as return values the macros
 * listed in descriptions to each of the constants.
 */
enum am_hsm_rc {
    /* Returned by AM_HSM_HANDLED() */
    AM_HSM_RC_HANDLED = 1,
    /* Returned by AM_HSM_TRAN() */
    AM_HSM_RC_TRAN,
    /* Returned by AM_HSM_TRAN_REDISPATCH() */
    AM_HSM_RC_TRAN_REDISPATCH,
    /* Returned by AM_HSM_SUPER() */
    AM_HSM_RC_SUPER
};

/** forward declaration */
struct am_hsm;

/**
 * A state handler.
 * One should not assume that a state handler would be invoked only for
 * processing event IDs enlisted in the case statement of internal
 * switch statement. Event handlers should avoid using any code outside
 * of the switch statement, especially code that has side effects.
 * @param hsm    the state machine
 * @param event  the event to handle
 * @return One of AM_HSM_RC_... constants.
 */
typedef enum am_hsm_rc (*am_hsm_state_fn)(
    struct am_hsm *hsm, const struct am_event *event
);

/**
 * HSM spy callback type.
 * Used as one place to catch all events for the given HSM.
 * Called on each user event BEFORE the event is processes by the HSM.
 * Should only be used for debugging purposes.
 * Set by am_hsm_set_spy().
 * Only supported if hsm.c is compiled with #AM_HSM_SPY defined.
 * @param hsm    the handler of HSM to spy
 * @param event  the event to spy
 */
typedef void (*am_hsm_spy_fn)(struct am_hsm *hsm, const struct am_event *event);

/** HSM state */
struct am_hsm_state {
    /** HSM state function  */
    am_hsm_state_fn fn;
    /** HSM state function instance. Used for submachines. Default is 0. */
    unsigned char ifn;
};

/** Helper macro. Not to be used directly. */
#define AM_STATE1_(s) \
    (struct am_hsm_state) { .fn = (am_hsm_state_fn)s, .ifn = 0 }

/** Helper macro. Not to be used directly. */
#define AM_STATE2_(s, i) \
    (struct am_hsm_state) { .fn = (am_hsm_state_fn)s, .ifn = i }

/**
 * Get HSM state from event handler and optionally the event handler instance.
 *
 * AM_HSM_STATE(s)     is converted to
 *                     (struct am_hsm_state){.fn = s, .ifn = 0}
 * AM_HSM_STATE(s, i)  is converted to
 *                     (struct am_hsm_state){.fn = s, .ifn = i}
 *
 * @param s  HSM event handler
 * @param i  HSM event handler instance. Used by submachines. Default is 0.
 * @return HSM state structure
 */
#define AM_HSM_STATE(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_STATE2_, AM_STATE1_, _)(__VA_ARGS__)

/** HSM state */
struct am_hsm {
    /** current state */
    am_hsm_state_fn state;
    /** temp state during transitions and event processing */
    am_hsm_state_fn temp;
#ifdef AM_HSM_SPY
    /** HSM spy callack */
    am_hsm_spy_fn spy;
#endif
    /** instance of current state */
    unsigned istate : 8;
    /** instance of temporary state during transitions & event processing */
    unsigned itemp : 8;
};

/**
 * Event processing is over. No transition was taken.
 * Used as a return value from an event handler that handled
 * an event and wants to prevent the event propagation to
 * superstate(s).
 */
#define AM_HSM_HANDLED() AM_HSM_RC_HANDLED

/** Helper macro. Not to be used directly. */
#define AM_SET_TEMP_(s, i)                               \
    (((struct am_hsm *)me)->temp = (am_hsm_state_fn)(s), \
     ((struct am_hsm *)me)->itemp = (i))

/** Helper macro. Not to be used directly. */
#define AM_TRAN1_(s) (AM_SET_TEMP_(s, 0), AM_HSM_RC_TRAN)
/** Helper macro. Not to be used directly. */
#define AM_TRAN2_(s, i) (AM_SET_TEMP_(s, i), AM_HSM_RC_TRAN)

/**
 * Event processing is over. Transition is taken.
 * It should never be returned for entry or exit events.
 * Conversely, the response to init event can optionally use
 * this macro as a return value to designate transition to
 * the provided state. The target state in this case must be
 * a substate of the current state.
 * @param s  the new state of type #am_hsm_state_fn (mandatory)
 * @param i  the new state submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN2_, AM_TRAN1_, _)(__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP1_(s) (AM_SET_TEMP_(s, 0), AM_HSM_RC_TRAN_REDISPATCH)
/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP2_(s, i) (AM_SET_TEMP_(s, i), AM_HSM_RC_TRAN_REDISPATCH)

/**
 * Event redispatch is requested. Transition is taken.
 * It should never be returned for entry, exit or init events.
 * Do not redispatch the same event more than once.
 * @param s  the new state of type #am_hsm_state_fn (mandatory)
 * @param i  the new state submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN_REDISPATCH(...)                                     \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN_REDISP2_, AM_TRAN_REDISP1_, _) \
    (__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_SUPER1_(s) (AM_SET_TEMP_(s, 0), AM_HSM_RC_SUPER)
/** Helper macro. Not to be used directly. */
#define AM_SUPER2_(s, i) (AM_SET_TEMP_(s, i), AM_HSM_RC_SUPER)

/**
 * Event processing is passed to superstate. No transition was taken.
 * If no explicit superstate exists, then the top (super)state am_hsm_top()
 * must be used.
 * @param s  the superstate of type #am_hsm_state_fn (mandatory)
 * @param i  the superstate submachine instance (optional, default is 0)
 */
#define AM_HSM_SUPER(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_SUPER2_, AM_SUPER1_, _)(__VA_ARGS__)

/**
 * Synchronous dispatch of event to the given HSM.
 * @param hsm    the HSM handler
 * @param event  the event to dispatch
 */
void am_hsm_dispatch(struct am_hsm *hsm, const struct am_event *event);

/**
 * Test whether HSM is in a given state.
 * Note that an HSM is in all superstates of the currently active state.
 * Use sparingly to test the current state of other state machine as
 * it breaks encapsulation.
 * @param hsm     the HSM handler
 * @param state   the state to check
 * @retval false  not in the state in the hierarchical sense
 * @retval true   in the state
 */
bool am_hsm_is_in(struct am_hsm *hsm, const struct am_hsm_state *state);

/**
 * Check if current state equals to #state (not in hierarchical sense).
 *
 * If current state of hsm is A, which is substate of B, then
 * am_hsm_state_is_eq(hsm, &AM_HSM_STATE(A)) is true, but
 * am_hsm_state_is_eq(hsm, &AM_HSM_STATE(B)) is false.
 *
 * @param hsm     the HSM handler
 * @param state   the state to compare against
 * @retval true   the current HSM state equals #state
 * @retval false  the current HSM state DOES NOT equal #state
 */
bool am_hsm_state_is_eq(struct am_hsm *hsm, const struct am_hsm_state *state);

/**
 * Get state instance.
 * Always returns the instance of the calling state function.
 * Calling the function in a state that is not part of any submachine
 * will always return 0.
 * @param hsm  the HSM handler
 * @return the instance
 */
int am_hsm_get_state_instance(const struct am_hsm *hsm);

/**
 * HSM constructor.
 * @param hsm    the HSM to construct
 * @param state  the initial state of the HSM object
 *               The initial state must return
 *               AM_HSM_TRAN(s) or AM_HSM_TRAN(s, i)
 */
void am_hsm_ctor(struct am_hsm *hsm, const struct am_hsm_state *state);

/**
 * HSM destructor.
 * @param hsm  the HSM to destruct
 */
void am_hsm_dtor(struct am_hsm *hsm);

/**
 * Perform HSM initial transition.
 * @param hsm         the HSM handler
 * @param init_event  the init event. Can be NULL. The event is not recycled.
 */
void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event);

/**
 * Set spy user callback as one place to catch all events for the given HSM.
 * Is only available if hsm.c is compiled with #AM_HSM_SPY defined.
 * Should only be used for debugging purposes.
 * Should only be called after calling am_hsm_ctor() and not during ongoing
 * HSM event processing.
 * @param hsm  the handler of HSM to spy
 * @param spy  the spy callback. Use NULL to unset.
 */
void am_hsm_set_spy(struct am_hsm *hsm, am_hsm_spy_fn spy);

/**
 * Every HSM has implicit top state, which surrounds all other elements
 * of the entire state machine.
 * One should never target top state in a state transition.
 */
enum am_hsm_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
