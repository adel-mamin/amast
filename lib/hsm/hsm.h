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
 * The event handlers must always return the A1HSM_SUPER() in response
 * to this event.
 */
#define A1HSM_EVT_EMPTY 0

/**
 * Init event.
 * Run initial transition from a given state.
 * Always follows the #A1HSM_EVT_ENTRY event.
 */
#define A1HSM_EVT_INIT 1

/**
 * Entry event.
 * Run entry action(s) for a given state.
 * Always precedes the #A1HSM_EVT_INIT event.
 * No state transition is allowed in response to this event.
 */
#define A1HSM_EVT_ENTRY 2

/**
 * Exit event.
 * Run exit action(s) for a given state.
 * No state transition is allowed in response to this event.
 */
#define A1HSM_EVT_EXIT 3

/** HSM event with maximum value. */
#define A1HSM_EVT_MAX A1HSM_EVT_EXIT

A1ASSERT_STATIC(EVT_USER > A1HSM_EVT_MAX);

/**
 * HSM state handler return codes.
 * These return codes are not used directly in user code.
 * Instead user code is expected to use as return values the macros
 * listed in descriptions to each of the constants.
 */
enum a1hsmrc {
    /* Returned by A1HSM_HANDLED() */
    A1HSM_RC_HANDLED = 0,
    /* Returned by A1HSM_TRAN() */
    A1HSM_RC_TRAN,
    /* Returned by A1HSM_TRAN_REDISPATCH() */
    A1HSM_RC_TRAN_REDISPATCH,
    /* Returned by A1HSM_SUPER() */
    A1HSM_RC_SUPER
};

/** forward declaration */
struct a1hsm;

/**
 * A state handler.
 * One should not assume that a state handler would be invoked only for
 * processing event IDs enlisted in the case statement of internal
 * switch statement. Event handlers should avoid using any code outside
 * of the switch statement, especially code that has side effects.
 * @param hsm    the state machine
 * @param event  the event to handle
 * @return One of A1HSM_RC_... constants.
 */
typedef enum a1hsmrc (*a1hsm_state_fn)(
    struct a1hsm *hsm, const struct event *evt
);

/** HSM state */
struct a1hsm_state {
    /** HSM state function  */
    a1hsm_state_fn fn;
    /** HSM state function instance. Used for submachines. Default is 0. */
    unsigned char instance;
};

/** Helper macro. Not to be used directly. */
#define A1STATE1_(f) \
    (struct a1hsm_state) { .fn = (a1hsm_state_fn)f, .instance = 0 }

/** Helper macro. Not to be used directly. */
#define A1STATE2_(f, i) \
    (struct a1hsm_state) { .fn = (a1hsm_state_fn)f, .instance = i }

/**
 * Get HSM state from event handler and optionally the event handler instance.
 *
 * A1HSM_STATE(fn)    is converted to
 *                    (struct a1hsm_state){.fn = fn, .instance = 0}
 * A1HSM_STATE(fn, i) is converted to
 *                    (struct a1hsm_state){.fn = fn, .instance = i}
 *
 * @param fn  HSM event handler
 * @param i   HSM event handler instance. Used by submachines. Default is 0.
 * @return HSM state structure
 */
#define A1HSM_STATE(...) \
    A1GET_MACRO_2_(__VA_ARGS__, A1STATE2_, A1STATE1_, _)(__VA_ARGS__)

/** HSM state */
struct a1hsm {
    /** current state */
    a1hsm_state_fn state;
    /** temp state during transitions and event processing */
    a1hsm_state_fn temp;
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
#define A1HSM_HANDLED() A1HSM_RC_HANDLED

/** Helper macro. Not to be used directly. */
#define A1SET_TEMP_(s, i)                              \
    (((struct a1hsm *)me)->temp = (a1hsm_state_fn)(s), \
     ((struct a1hsm *)me)->itemp = (i))

/** Helper macro. Not to be used directly. */
#define A1TRAN1_(s) (A1SET_TEMP_(s, 0), A1HSM_RC_TRAN)
/** Helper macro. Not to be used directly. */
#define A1TRAN2_(s, i) (A1SET_TEMP_(s, i), A1HSM_RC_TRAN)

/**
 * Event processing is over. Transition is taken.
 * It should never be returned for entry or exit events.
 * Conversely, the response to init event can optionally use
 * this macro as a return value to designate transition to
 * the provided state. The target state in this case must be
 * a substate of the current state.
 * @param s  the new state of type #a1hsm_state_fn (mandatory)
 * @param i  the new state submachine instance (optional, default is 0)
 */
#define A1HSM_TRAN(...) \
    A1GET_MACRO_2_(__VA_ARGS__, A1TRAN2_, A1TRAN1_, _)(__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define A1TRAN_REDISPATCH1_(s) (A1SET_TEMP_(s, 0), A1HSM_RC_TRAN_REDISPATCH)
/** Helper macro. Not to be used directly. */
#define A1TRAN_REDISPATCH2_(s, i) (A1SET_TEMP_(s, i), A1HSM_RC_TRAN_REDISPATCH)

/**
 * Event redispatch is requested. Transition is taken.
 * It should never be returned for entry, exit or init events.
 * Do not redispatch the same event more than once.
 * @param s  the new state of type #a1hsm_state_fn (mandatory)
 * @param i  the new state submachine instance (optional, default is 0)
 */
#define A1HSM_TRAN_REDISPATCH(...)                                           \
    A1GET_MACRO_2_(__VA_ARGS__, A1TRAN_REDISPATCH2_, A1TRAN_REDISPATCH1_, _) \
    (__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define A1SUPER1_(s) (A1SET_TEMP_(s, 0), A1HSM_RC_SUPER)
/** Helper macro. Not to be used directly. */
#define A1SUPER2_(s, i) (A1SET_TEMP_(s, i), A1HSM_RC_SUPER)

/**
 * Event processing is passed to superstate. No transition was taken.
 * If no explicit superstate exists, then the top (super)state a1hsm_top()
 * must be used.
 * @param s  the superstate of type #a1hsm_state_fn (mandatory)
 * @param i  the superstate submachine instance (optional, default is 0)
 */
#define A1HSM_SUPER(...) \
    A1GET_MACRO_2_(__VA_ARGS__, A1SUPER2_, A1SUPER1_, _)(__VA_ARGS__)

/**
 * Synchronous dispatch of event to the given HSM.
 * @param hsm    the HSM handler
 * @param event  the event to dispatch
 */
void a1hsm_dispatch(struct a1hsm *hsm, const struct event *event);

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
bool a1hsm_is_in(struct a1hsm *hsm, const struct a1hsm_state *state);

/**
 * Check if current state equals to #state (not in hierarchical sense).
 *
 * If current state of hsm is A, which is substate of B, then
 * a1hsm_state_is_eq(hsm, &A1HSM_STATE(A)) is true, but
 * a1hsm_state_is_eq(hsm, &A1HSM_STATE(B)) is false.
 *
 * @param hsm     the HSM handler
 * @param state   the state to compare against
 * @retval true   the current HSM state equals #state
 * @retval false  the current HSM state DOES NOT equal #state
 */
bool a1hsm_state_is_eq(struct a1hsm *hsm, const struct a1hsm_state *state);

/**
 * Get state instance.
 * Always returns the instance of the state that calls the API.
 * Calling the function in a state that is not part of any submachine
 * will always return 0.
 * @param hsm  the HSM handler
 * @return the instance
 */
int a1hsm_get_state_instance(const struct a1hsm *hsm);

/* const struct a1hsm_state *a1hsm_get_child_state(struct a1hsm *hsm); */
/* const struct a1hsm_state *a1hsm_get_super_state(struct a1hsm *hsm); */

/**
 * HSM constructor.
 * @param hsm    the HSM to construct
 * @param state  the initial state of the HSM object
 *               The initial state must return
 *               A1HSM_TRAN(s) or A1HSM_TRAN(s, i)
 */
void a1hsm_ctor(struct a1hsm *hsm, const struct a1hsm_state *state);

/**
 * HSM destructor.
 * @param hsm  the HSM to destruct
 */
void a1hsm_dtor(struct a1hsm *hsm);

/**
 * Perform HSM initial transition.
 * @param hsm         the HSM handler
 * @param init_event  the init event. Can be NULL. The event is not recycled.
 */
void a1hsm_init(struct a1hsm *hsm, const struct event *init_event);

/**
 * Every HSM has implicit top state, which surrounds all other elements
 * of the entire state machine.
 * One should never target top state in a state transition.
 */
enum a1hsmrc a1hsm_top(struct a1hsm *hsm, const struct event *event);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
