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

#ifdef HSM_EVT_USER
ASSERT_STATIC(HSM_EVT_USER == 4)
#else
#define HSM_EVT_USER 4 /**< user event IDs start with this ID (inclusive) */
#endif

#define HSM_STATE_HANDLED 0
#define HSM_STATE_IGNORED 0
#define HSM_STATE_TRAN 1
#define HSM_STATE_SUPER 2

/** Event descriptor */
struct hsm_event {
    /** event ID. */
    int id;
};

/** forward declaration */
struct hsm;

/**
 * A state handler.
 * One should not assume that a state handler would be invoked only for
 * processing event IDs enlisted in the case statement of internal
 * switch statement. You should avoid any code outside of the switch
 * statement, especially code that would have side effects.
 * @param hsm the state machine.
 * @param event the event to handle.
 * @return One of HSM_STATE_... constants.
 */
typedef int (*hsm_state_handler_func)(
    struct hsm *hsm, const struct hsm_event *event
);

/**
 * Get HSM state from HSM handler.
 * @param h HSM handler
 * @return HSM state handler
 */
#define HSM_STATE(h) ((hsm_state_handler_func)(h))

/** HSM state */
struct hsm {
    /** the current state */
    hsm_state_handler_func state;
    /** temp state during transitions and event processing */
    hsm_state_handler_func temp;
};

/** The event processing is over. No transition was taken. */
#define HSM_HANDLED() HSM_STATE_HANDLED

/** The event was ignored. No transition was taken. */
#define HSM_IGNORED() HSM_STATE_IGNORED

/**
 * The event processing is over. Transition is taken.
 * It should never be returned for entry or exit events.
 * Conversely, the response to init event can optionally include
 * this macro to designate transition to the the provided substate
 * of the current state.
 * The transition can only be done to substates of the state.
 * @param s          the new state of type #hsm_state_handler_func.
 */
#define HSM_TRAN(s) (((struct hsm *)me)->temp = HSM_STATE(s), HSM_STATE_TRAN)

/**
 * The event processing is passed to the superstate. No transition was taken.
 * If no explicit superstate exists, then the top (super)state hsm_top()
 * must be used.
 * @param s          the superstate of type #hsm_state_handler_func.
 */
#define HSM_SUPER(s) (((struct hsm *)me)->temp = HSM_STATE(s), HSM_STATE_SUPER)

/**
 * Synchronous dispatching of event to the given HSM.
 * @param hsm        the hierarchical state machine handler.
 * @param event      the event to dispatch.
 */
void hsm_dispatch(struct hsm *hsm, const struct hsm_event *event);

/**
 * Test whether the HSM is in a given state.
 * Note that an HSM is in all superstates of the currently active state.
 * Use sparingly to test the current state of other state machine as
 * it breaks encapsulation.
 * @param hsm        the hierarchical state machine handler.
 * @param state      the state to check.
 * @retval false     not in the state in the hierarchical sense.
 * @retval true      in the state.
 */
bool hsm_is_in(const struct hsm *hsm, hsm_state_handler_func state);

/**
 * Return the current HSM state.
 * @param hsm        the hierarchical state machine handler.
 * @return           The current HSM state.
 */
hsm_state_handler_func hsm_state(struct hsm *hsm);

/**
 * Hierarchical state machine constructor.
 * @param hsm        the hierarchical state machine to construct.
 * @param state      the initial state of the hierarchical state machine object.
 *                   The initial state must return HSM_TRAN(s).
 */
void hsm_ctor(struct hsm *hsm, hsm_state_handler_func state);

/**
 * Hierarchical state machine destructor.
 * @param hsm        the hierarchical state machine to destruct.
 */
void hsm_dtor(struct hsm *hsm);

/**
 * Performs HSM initial transition.
 * @param hsm        the hierarchical state machine handler.
 * @param init_event the init event. Can be NULL. The event is not recycled.
 */
void hsm_init(struct hsm *hsm, const struct hsm_event *init_event);

/**
 * Every HSM has the implicit top state, which surrounds all the other elements
 * of the entire state machine.
 * One should never target top state in a state transition.
 */
int hsm_top(struct hsm *hsm, const struct hsm_event *event);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
