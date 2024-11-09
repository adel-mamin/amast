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

/**
 * @file
 *
 * Finite State Machine (FSM) module API declaration.
 * Configuration defines:
 * AM_FSM_SPY - enables FSM spy callback support for debugging
 */

#ifndef FSM_H_INCLUDED
#define FSM_H_INCLUDED

#include <stdbool.h>

#include "common/compiler.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

enum am_fsm_evt {
    /**
     * Entry event.
     * Run entry action(s) for a given state.
     * No state transition is allowed in response to this event.
     */
    AM_FSM_EVT_ENTRY = AM_EVT_RANGE_FSM_BEGIN,

    /**
     * Exit event.
     * Run exit action(s) for a given state.
     * No state transition is allowed in response to this event.
     */
    AM_FSM_EVT_EXIT,
    /** FSM event with maximum value */
    AM_FSM_EVT_MAX = AM_FSM_EVT_EXIT
};

AM_ASSERT_STATIC(AM_FSM_EVT_MAX <= AM_EVT_RANGE_FSM_END);

/**
 * FSM state handler return codes.
 * These return codes are not used directly in user code.
 * Instead user code is expected to use as return values the macros
 * listed in descriptions to each of the constants.
 */
enum am_fsm_rc {
    /** Returned by AM_FSM_HANDLED() */
    AM_FSM_RC_HANDLED = 1,
    /** Returned by AM_FSM_TRAN() */
    AM_FSM_RC_TRAN,
    /** Returned by AM_FSM_TRAN_REDISPATCH() */
    AM_FSM_RC_TRAN_REDISPATCH,
};

/** forward declaration */
struct am_fsm;

/**
 * A state handler.
 *
 * @param fsm    the state machine
 * @param event  the event to handle
 * @return return code
 */
typedef enum am_fsm_rc (*am_fsm_state)(
    struct am_fsm *fsm, const struct am_event *event
);

/**
 * FSM spy callback type.
 *
 * Used as one place to catch all events for the given FSM.
 * Called on each user event BEFORE the event is processes by the FSM.
 * Should only be used for debugging purposes.
 * Set by am_fsm_set_spy().
 * Only supported if fsm.c is compiled with #AM_FSM_SPY defined.
 *
 * @param fsm    the handler of FSM to spy
 * @param event  the event to spy
 */
typedef void (*am_fsm_spy_fn)(struct am_fsm *fsm, const struct am_event *event);

/**
 * Get FSM state from event handler.
 *
 * @param s  FSM event handler
 * @return FSM state
 */
#define AM_FSM_STATE(s) ((am_fsm_state)(s))

/** FSM state */
struct am_fsm {
    /** active state */
    am_fsm_state state;
#ifdef AM_FSM_SPY
    /** FSM spy callback */
    am_fsm_spy_fn spy;
#endif
};

/**
 * Event processing is over. No transition was taken.
 * Used as a return value from an event handler that handled
 * an event and wants to prevent the event propagation to
 * superstate(s).
 */
#define AM_FSM_HANDLED() AM_FSM_RC_HANDLED

/** Helper macro. Not to be used directly. */
#define AM_SET_STATE_(s) (((struct am_fsm *)me)->state = (am_fsm_state)(s))

/**
 * Event processing is over. Transition is taken.
 *
 * It should never be returned for #AM_FSM_EVT_ENTRY or #AM_FSM_EVT_EXIT events.
 *
 * @param s  the new state of type #am_fsm_state
 */
#define AM_FSM_TRAN(s) (AM_SET_STATE_(s), AM_FSM_RC_TRAN)

/**
 * Event redispatch is requested. Transition is taken.
 *
 * It should never be returned for #AM_FSM_EVT_ENTRY or #AM_FSM_EVT_EXIT events.
 * Do not redispatch the same event more than once.
 *
 * @param s  the new state of type #am_fsm_state
 */
#define AM_FSM_TRAN_REDISPATCH(s) (AM_SET_STATE_(s), AM_FSM_RC_TRAN_REDISPATCH)

/**
 * Synchronous dispatch of event to the given FSM.
 *
 * @param fsm    the FSM handler
 * @param event  the event to dispatch
 */
void am_fsm_dispatch(struct am_fsm *fsm, const struct am_event *event);

/**
 * Test whether FSM is in a given state.
 *
 * @param fsm     the FSM handler
 * @param state   the state to check
 * @retval false  not in the state
 * @retval true   in the state
 */
bool am_fsm_is_in(struct am_fsm *fsm, const am_fsm_state state);

/**
 * Get the active state.
 *
 * @param fsm  the FSM handler
 * @return the active state
 */
am_fsm_state am_fsm_get_active_state(const struct am_fsm *fsm);

/**
 * FSM constructor.
 *
 * @param fsm    the FSM to construct
 * @param state  the initial state of the FSM object
 *               The initial state must return AM_FSM_TRAN(s)
 */
void am_fsm_ctor(struct am_fsm *fsm, const am_fsm_state state);

/**
 * FSM destructor.
 *
 * @param fsm  the FSM to destruct
 */
void am_fsm_dtor(struct am_fsm *fsm);

/**
 * Perform FSM initial transition.
 *
 * @param fsm         the FSM handler
 * @param init_event  the init event. Can be NULL.
 */
void am_fsm_init(struct am_fsm *fsm, const struct am_event *init_event);

/**
 * Set spy user callback as one place to catch all events for the given FSM.
 *
 * Is only available if fsm.c is compiled with #AM_FSM_SPY defined.
 * Should only be used for debugging purposes.
 * Should only be called after calling am_fsm_ctor() and not during ongoing
 * FSM event processing.
 *
 * @param fsm  the handler of FSM to spy
 * @param spy  the spy callback. Use NULL to unset.
 */
void am_fsm_set_spy(struct am_fsm *fsm, am_fsm_spy_fn spy);

#ifdef __cplusplus
}
#endif

#endif /* FSM_H_INCLUDED */
