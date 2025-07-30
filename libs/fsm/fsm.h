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
 * Finite State Machine (FSM) libary API declaration.
 * Configuration defines:
 * AM_FSM_SPY - enables FSM spy callback support for debugging
 */

#ifndef AM_FSM_H_INCLUDED
#define AM_FSM_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/compiler.h"
#include "common/types.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** FSM events. */
enum am_fsm_evt_id {
    /**
     * Entry event.
     *
     * Run entry action(s) for a given state.
     *
     * No state transition is allowed in response to this event.
     */
    AM_EVT_FSM_ENTRY = AM_EVT_RANGE_SM_BEGIN,

    /**
     * Exit event.
     *
     * Run exit action(s) for a given state.
     *
     * No state transition is allowed in response to this event.
     */
    AM_EVT_FSM_EXIT,
};

AM_ASSERT_STATIC(AM_EVT_FSM_EXIT <= AM_EVT_RANGE_SM_END);

/** Forward declaration. */
struct am_fsm;

/**
 * FSM state (event handler) function type.
 *
 * @param fsm    the FSM
 * @param event  the event to handle
 * @return return code
 */
typedef enum am_rc (*am_fsm_state_fn)(
    struct am_fsm *fsm, const struct am_event *event
);

/**
 * FSM spy user callback type.
 *
 * Used as one place to catch all events for the given FSM.
 *
 * Called on each user event BEFORE the event is processes by the FSM.
 *
 * Should only be used for debugging purposes.
 *
 * Set by am_fsm_set_spy().
 *
 * Only supported, if the FSM library is compiled with `AM_FSM_SPY` defined.
 *
 * @param fsm    the handler of the FSM to spy
 * @param event  the event to spy
 */
typedef void (*am_fsm_spy_fn)(struct am_fsm *fsm, const struct am_event *event);

/**
 * Construct FSM state from FSM event handler.
 *
 * @param s  the FSM event handler
 * @return constructed FSM state structure
 */
#define AM_FSM_STATE_CTOR(s) ((am_fsm_state_fn)(s))

/**
 * FSM descriptor.
 *
 * None of the fields of the descriptor are to be accessed directly
 * by user code. The only purpose of exposing it is to allow user
 * code to reserve memory for it.
 */
struct am_fsm {
    /** Active state. */
    am_fsm_state_fn state;
#ifdef AM_FSM_SPY
    /** FSM spy callback. */
    am_fsm_spy_fn spy;
#endif
    /** Safety net to catch missing am_fsm_init() call. */
    uint8_t init_called : 1;
    /** Safety net to catch erroneous reentrant am_fsm_dispatch() call. */
    uint8_t dispatch_in_progress : 1;
};

/**
 * Event processing is over.
 *
 * No transition is taken.
 *
 * Used as a default return value from FSM event handlers.
 */
#define AM_FSM_HANDLED() AM_RC_HANDLED

/**
 * Helper macro.
 *
 * Not to be used directly.
 */
#define AM_FSM_SET_(s) (((struct am_fsm *)me)->state = (am_fsm_state_fn)(s))

/**
 * Event processing is over. Transition is taken.
 *
 * It should never be returned in response to
 * #AM_EVT_FSM_ENTRY or #AM_EVT_FSM_EXIT events.
 *
 * AM_FSM_TRAN(s) is converted to
 *
 * @code{.c}
 * (((struct am_fsm *)me)->state = (am_fsm_state_fn)(s), AM_RC_TRAN)
 * @endcode
 *
 * @param s  the new state of type #am_fsm_state_fn
 */
#define AM_FSM_TRAN(s) (AM_FSM_SET_(s), AM_RC_TRAN)

/**
 * Same event redispatch is requested. Transition is taken.
 *
 * It should never be returned for #AM_EVT_FSM_ENTRY or #AM_EVT_FSM_EXIT events.
 *
 * Do not redispatch the same event more than once within same
 * am_fsm_dispatch() call.
 *
 * AM_FSM_TRAN_REDISPATCH(s) is converted to
 *
 * @code{.c}
 * (((struct am_fsm *)me)->state = (am_fsm_state_fn)(s), AM_RC_TRAN_REDISPATCH)
 * @endcode
 *
 * @param s  the new state of type #am_fsm_state_fn
 */
#define AM_FSM_TRAN_REDISPATCH(s) (AM_FSM_SET_(s), AM_RC_TRAN_REDISPATCH)

/**
 * Synchronous dispatch of event to a given FSM.
 *
 * @param fsm    the FSM
 * @param event  the event to dispatch
 */
void am_fsm_dispatch(struct am_fsm *fsm, const struct am_event *event);

/**
 * Check whether FSM is in a given state.
 *
 * Use sparingly to check states of other state machine(s) as
 * it breaks encapsulation.
 *
 * @param fsm     the FSM
 * @param state   the state to check
 *
 * @retval false  not in the state
 * @retval true   in the state
 */
bool am_fsm_is_in(const struct am_fsm *fsm, am_fsm_state_fn state);

/**
 * Get FSM's active state.
 *
 * @param fsm  the FSM
 *
 * @return the active state
 */
am_fsm_state_fn am_fsm_get_state(const struct am_fsm *fsm);

/**
 * FSM constructor.
 *
 * @param fsm    the FSM to construct
 *
 * @param state  the initial state of the FSM object.
 *               The initial state must return AM_FSM_TRAN(s).
 */
void am_fsm_ctor(struct am_fsm *fsm, am_fsm_state_fn state);

/**
 * FSM destructor.
 *
 * Exits any FSM state.
 *
 * The FSM is not usable after this call.
 * Call am_fsm_ctor() to construct the FSM again.
 *
 * @param fsm  the FSM to destruct
 */
void am_fsm_dtor(struct am_fsm *fsm);

/**
 * Perform FSM initial transition.
 *
 * Calls the initial state set by am_fsm_ctor() with the provided
 * optional init event and performs the initial transition.
 *
 * @param fsm         the FSM to init
 * @param init_event  the init event. Can be NULL.
 */
void am_fsm_init(struct am_fsm *fsm, const struct am_event *init_event);

/**
 * Set spy user callback as a one place to catch all events for the given FSM.
 *
 * Is only available, if the FSM library is compiled with `AM_FSM_SPY` defined.
 *
 * Should only be used for debugging purposes.
 *
 * Should only be called after calling am_fsm_ctor().
 *
 * @param fsm  the FSM to spy
 * @param spy  the spy callback. Use NULL to unset.
 */
void am_fsm_set_spy(struct am_fsm *fsm, am_fsm_spy_fn spy);

#ifdef __cplusplus
}
#endif

#endif /* AM_FSM_H_INCLUDED */
