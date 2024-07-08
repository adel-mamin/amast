/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Adel Mamin
 * Copyright (c) 2019 Ryan Hartlage (documentation)
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Should not cause any side effects in event handlers.
 * The event handlers must always return the HSM_SUPER() in response.
 */
#define HSM_EVT_EMPTY 0

/**
 * Run initial transition from a given state.
 * Always follows the #HSM_EVT_ENTRY event.
 */
#define HSM_EVT_INIT 1

/**
 * Run entry action(s) for a given state.
 * Always precedes the #HSM_EVT_INIT event.
 */
#define HSM_EVT_ENTRY 2

/** Run exit action(s) for a given state. */
#define HSM_EVT_EXIT 3

/** User event IDs start with this ID (inclusive). */
#define HSM_EVT_USER 4

/** HSM state handler return codes */
enum hsm_rc {
    HSM_STATE_HANDLED = 0,
    HSM_STATE_IGNORED = HSM_STATE_HANDLED,
    HSM_STATE_TRAN,
    HSM_STATE_SUPER
};

/** forward declaration */
struct hsm;

/**
 * A state handler.
 * One should not assume that a state handler would be invoked only for
 * processing event IDs enlisted in the case statement of internal
 * switch statement. You should avoid any code outside of the switch
 * statement, especially code that would have side effects.
 * @param hsm    the state machine
 * @param event  the event to handle
 * @return One of HSM_STATE_... constants.
 */
typedef int (*hsm_state_fn)(struct hsm *hsm, const struct event *event);

/**
 * Get HSM state from HSM handler.
 * @param h HSM handler
 * @return HSM state handler
 */
#define HSM_STATE(h) ((hsm_state_fn)(h))

/** HSM state */
struct hsm {
    /** current state */
    hsm_state_fn state;
    /** temp state during transitions and event processing */
    hsm_state_fn temp;
    /** submachine instance index */
    unsigned istate : 8;
    /** temp submachine instance index during transitions & event processing */
    unsigned itemp : 8;
};

/** Event processing is over. No transition was taken. */
#define HSM_HANDLED() HSM_STATE_HANDLED

/** Event was ignored. No transition was taken. */
#define HSM_IGNORED() HSM_STATE_IGNORED

#define HSM_GET_MACRO_(_1, _2, NAME, ...) NAME

#define HSM_SET_TEMP_(s, i) \
    (((struct hsm *)me)->temp = HSM_STATE(s), ((struct hsm *)me)->itemp = (i))

#define HSM_TRAN_1_(s) (HSM_SET_TEMP_(s, 0), HSM_STATE_TRAN)
#define HSM_TRAN_2_(s, i) (HSM_SET_TEMP_(s, i), HSM_STATE_TRAN)

/**
 * Event processing is over. Transition is taken.
 * It should never be returned for entry or exit events.
 * Conversely, the response to init event can optionally include
 * this macro to designate transition to the provided substate
 * of the current state.
 * @param s  the new state of type #hsm_state_fn (mandatory)
 * @param i  the new state submachine instance index (optional, default is 0)
 */
#define HSM_TRAN(...) \
    HSM_GET_MACRO_(__VA_ARGS__, HSM_TRAN_2_, HSM_TRAN_1_)(__VA_ARGS__)

#define HSM_SUPER_1_(s) (HSM_SET_TEMP_(s, 0), HSM_STATE_SUPER)
#define HSM_SUPER_2_(s, i) (HSM_SET_TEMP_(s, i), HSM_STATE_SUPER)

/**
 * Event processing is passed to superstate. No transition was taken.
 * If no explicit superstate exists, then the top (super)state hsm_top()
 * must be used.
 * @param s  the superstate of type #hsm_state_fn (mandatory)
 * @param i  the superstate submachine instance index (optional, default is 0)
 */
#define HSM_SUPER(...) \
    HSM_GET_MACRO_(__VA_ARGS__, HSM_SUPER_2_, HSM_SUPER_1_)(__VA_ARGS__)

/**
 * Synchronous dispatching of event to the given HSM.
 * @param hsm        the hierarchical state machine handler
 * @param event      the event to dispatch
 */
void hsm_dispatch(struct hsm *hsm, const struct event *event);

/**
 * Test whether HSM is in a given state.
 * Note that an HSM is in all superstates of the currently active state.
 * Use sparingly to test the current state of other state machine as
 * it breaks encapsulation.
 * @param hsm        the hierarchical state machine handler
 * @param state      the state to check
 * @retval false     not in the state in the hierarchical sense
 * @retval true      in the state
 */
bool hsm_is_in(struct hsm *hsm, hsm_state_fn state);

/**
 * Return current HSM state.
 * @param hsm        the hierarchical state machine handler
 * @return           the current HSM state
 */
hsm_state_fn hsm_state(struct hsm *hsm);

/**
 * Hierarchical state machine constructor.
 * @param hsm        the hierarchical state machine to construct
 * @param state      the initial state of the hierarchical state machine object
 *                   The initial state must return HSM_TRAN(s).
 */
void hsm_ctor(struct hsm *hsm, hsm_state_fn state);

/**
 * Hierarchical state machine destructor.
 * @param hsm        the hierarchical state machine to destruct
 */
void hsm_dtor(struct hsm *hsm);

/**
 * Performs HSM initial transition.
 * @param hsm        the hierarchical state machine handler
 * @param init_event the init event. Can be NULL. The event is not recycled.
 */
void hsm_init(struct hsm *hsm, const struct event *init_event);

/**
 * Every HSM has implicit top state, which surrounds all other elements
 * of the entire state machine.
 * One should never target top state in a state transition.
 */
int hsm_top(struct hsm *hsm, const struct event *event);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
