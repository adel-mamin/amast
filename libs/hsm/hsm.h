/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Ryan Hartlage (documentation)
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
 * Hierarchical State Machine (HSM) module API declaration.
 * Configuration defines:
 * AM_HSM_SPY - enables HSM spy callback support for debugging
 */

#ifndef AM_HSM_H_INCLUDED
#define AM_HSM_H_INCLUDED

#include <stdbool.h>

#include "common/compiler.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** HSM events. */
enum am_hsm_evt {
    /**
     * Empty event.
     * Should not cause any side effects in event handlers.
     * The event handlers must always return the AM_HSM_SUPER() in response
     * to this event.
     */
    AM_EVT_HSM_EMPTY = AM_EVT_RANGE_HSM_BEGIN,

    /**
     * Init event.
     * Run initial transition from a given state.
     * Always follows the #AM_EVT_HSM_ENTRY event.
     */
    AM_EVT_HSM_INIT,

    /**
     * Entry event.
     * Run entry action(s) for a given state.
     * Always precedes the #AM_EVT_HSM_INIT event.
     * No state transition is allowed in response to this event.
     */
    AM_EVT_HSM_ENTRY,

    /**
     * Exit event.
     * Run exit action(s) for a given state.
     * No state transition is allowed in response to this event.
     */
    AM_EVT_HSM_EXIT,

    /** HSM event with maximum value. */
    AM_EVT_HSM_MAX = AM_EVT_HSM_EXIT
};

AM_ASSERT_STATIC(AM_EVT_HSM_MAX <= AM_EVT_RANGE_HSM_END);

/**
 * HSM state handler return codes.
 *
 * These return codes are not used directly in user code.
 * Instead user code is expected to use as return values the macros
 * listed in descriptions to each of the constants.
 */
enum am_hsm_rc {
    /** Returned by AM_HSM_HANDLED() */
    AM_HSM_RC_HANDLED = 1,
    /** Returned by AM_HSM_TRAN() */
    AM_HSM_RC_TRAN,
    /** Returned by AM_HSM_TRAN_REDISPATCH() */
    AM_HSM_RC_TRAN_REDISPATCH,
    /** Returned by AM_HSM_SUPER() */
    AM_HSM_RC_SUPER
};

/** forward declaration */
struct am_hsm;

/**
 * HSM state (event handler) function type.
 *
 * One should not assume that a state handler would be invoked only for
 * processing event IDs enlisted in the case statement of internal
 * switch statement. Event handlers should avoid using any code outside
 * of the switch statement, especially code that has side effects.
 *
 * @param hsm    the HSM
 * @param event  the event to handle
 *
 * @return return code
 */
typedef enum am_hsm_rc (*am_hsm_state_fn)(
    struct am_hsm *hsm, const struct am_event *event
);

/**
 * HSM spy callback type.
 *
 * Used as one place to catch all events for the given HSM.
 * Called on each user event BEFORE the event is processes by the HSM.
 * Should only be used for debugging purposes.
 * Set by am_hsm_set_spy().
 * Only supported if hsm.c is compiled with AM_HSM_SPY defined.
 *
 * @param hsm    the handler of HSM to spy
 * @param event  the event to spy
 */
typedef void (*am_hsm_spy_fn)(struct am_hsm *hsm, const struct am_event *event);

/** HSM state */
struct am_hsm_state {
    /** HSM state (event handler) function */
    am_hsm_state_fn fn;
    /**
     * HSM submachine instance.
     * Default is 0. Valid range [0,127].
     */
    char smi;
};

/** Helper macro. Not to be used directly. */
#define AM_STATE1_(s) \
    (struct am_hsm_state){.fn = (am_hsm_state_fn)(s), .smi = 0}

/** Helper macro. Not to be used directly. */
#define AM_STATE2_(s, i) \
    (struct am_hsm_state) { .fn = (am_hsm_state_fn)(s), .smi = (char)(i) }

/**
 * Get HSM state from HSM event handler and optionally HSM submachine instance.
 *
 * AM_HSM_STATE_CTOR(s)     is converted to
 *                          (struct am_hsm_state){.fn = s, .smi = 0}
 * AM_HSM_STATE_CTOR(s, i)  is converted to
 *                          (struct am_hsm_state){.fn = s, .smi = i}
 *
 * @param s  HSM event handler
 * @param i  HSM submachine instance. Default is 0.
 *
 * @return HSM state structure
 */
#define AM_HSM_STATE_CTOR(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_STATE2_, AM_STATE1_, _)(__VA_ARGS__)

/** HSM hierarchy maximum depth */
#ifndef HSM_HIERARCHY_DEPTH_MAX
#define HSM_HIERARCHY_DEPTH_MAX 16
#endif

/** HSM hierarchy level representation bits number */
#define AM_HSM_HIERARCHY_LEVEL_BITS 5

/** HSM hierarchy level representation bits mask */
#define AM_HSM_HIERARCHY_LEVEL_MASK ((1U << AM_HSM_HIERARCHY_LEVEL_BITS) - 1)

AM_ASSERT_STATIC(HSM_HIERARCHY_DEPTH_MAX <= AM_HSM_HIERARCHY_LEVEL_MASK);

/** HSM state */
struct am_hsm {
    /** active state */
    struct am_hsm_state state;
    /**
     * While am_hsm::state::smi maintains submachine instance of active state,
     * am_hsm::smi maintains the transitive submachine instance that may differ
     * from am_hsm::state::smi, when an event is propagated up
     * from substates to superstates.
     * Returned by am_hsm_get_instance().
     */
    unsigned char smi;
    /**
     * Active state hierarchy level [0,HSM_HIERARCHY_DEPTH_MAX]
     * (level 0 is assigned to am_hsm_top()).
     * Used internally to speed up state transitions.
     */
    unsigned char hierarchy_level : AM_HSM_HIERARCHY_LEVEL_BITS;
    /** safety net to catch missing am_hsm_ctor() call */
    unsigned char ctor_called : 1;
    /** safety net to catch missing am_hsm_init() call */
    unsigned char init_called : 1;
    /** safety net to catch erroneous reentrant am_hsm_dispatch() call */
    unsigned char dispatch_in_progress : 1;
#ifdef AM_HSM_SPY
    /** HSM spy callback */
    am_hsm_spy_fn spy;
#endif
};

/**
 * Event processing is over. No state transition is triggered.
 *
 * Used as a return value from the event handler that handled
 * an event and wants to prevent the event propagation to
 * superstate(s).
 */
#define AM_HSM_HANDLED() AM_HSM_RC_HANDLED

/** Helper macro. Not to be used directly. */
#define AM_HSM_SET_(s, i)                                    \
    (((struct am_hsm *)me)->state = AM_HSM_STATE_CTOR(s, i), \
     ((struct am_hsm *)me)->smi = (unsigned char)i)

/** Helper macro. Not to be used directly. */
#define AM_TRAN1_(s) (AM_HSM_SET_(s, 0), AM_HSM_RC_TRAN)
/** Helper macro. Not to be used directly. */
#define AM_TRAN2_(s, i) (AM_HSM_SET_(s, i), AM_HSM_RC_TRAN)

/**
 * Event processing is over. Transition is triggered.
 *
 * It should never be returned for #AM_EVT_HSM_ENTRY or #AM_EVT_HSM_EXIT events.
 * Conversely, the response to #AM_EVT_HSM_INIT event can optionally use
 * this macro as a return value to designate transition to
 * the provided state. The target state in this case must be
 * a substate of the current state.
 *
 * @param s  the new state of type #am_hsm_state_fn (mandatory)
 * @param i  the new state submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN2_, AM_TRAN1_, _)(__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP1_(s) (AM_HSM_SET_(s, 0), AM_HSM_RC_TRAN_REDISPATCH)
/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP2_(s, i) (AM_HSM_SET_(s, i), AM_HSM_RC_TRAN_REDISPATCH)

/**
 * Event redispatch is requested. Transition is triggered.
 *
 * It should never be returned for #AM_EVT_HSM_ENTRY, #AM_EVT_HSM_EXIT or
 * #AM_EVT_HSM_INIT events.
 * Do not redispatch the same event more than once within same
 * am_hsm_dispatch() call.
 *
 * @param s  the new HSM state of type #am_hsm_state_fn (mandatory)
 * @param i  the new HSM state submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN_REDISPATCH(...)                                     \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN_REDISP2_, AM_TRAN_REDISP1_, _) \
    (__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_SUPER1_(s) (AM_HSM_SET_(s, 0), AM_HSM_RC_SUPER)
/** Helper macro. Not to be used directly. */
#define AM_SUPER2_(s, i) (AM_HSM_SET_(s, i), AM_HSM_RC_SUPER)

/**
 * Event processing is passed to superstate. No transition was triggered.
 *
 * If no explicit superstate exists, then the top (super)state am_hsm_top()
 * must be used.
 *
 * @param s  the superstate of type #am_hsm_state_fn (mandatory)
 * @param i  the superstate submachine instance (optional, default is 0)
 */
#define AM_HSM_SUPER(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_SUPER2_, AM_SUPER1_, _)(__VA_ARGS__)

/**
 * Synchronous dispatch of event to a given HSM.
 *
 * Does not free the event - this is caller's responsibility.
 *
 * @param hsm    the HSM
 * @param event  the event to dispatch
 */
void am_hsm_dispatch(struct am_hsm *hsm, const struct am_event *event);

/**
 * Test whether HSM is in a given state.
 *
 * Note that an HSM is in all superstates of the active state.
 * Use sparingly to test the active state of other state machine as
 * it breaks encapsulation.
 *
 * @param hsm    the HSM
 * @param state  the state to check
 *
 * @retval false  not in the state in the hierarchical sense
 * @retval true   in the state
 */
bool am_hsm_is_in(struct am_hsm *hsm, struct am_hsm_state state);

/**
 * Check if HSM's active state equals to \p state (not in hierarchical sense).
 *
 * If active state of hsm is S1, which is substate of S, then
 * am_hsm_state_is_eq(hsm, AM_HSM_STATE_CTOR(S1)) is true, but
 * am_hsm_state_is_eq(hsm, AM_HSM_STATE_CTOR(S)) is false.
 *
 * @param hsm    the HSM
 * @param state  the state to compare against
 *
 * @retval true   the active HSM state equals \p state
 * @retval false  the active HSM state DOES NOT equal \p state
 */
bool am_hsm_state_is_eq(const struct am_hsm *hsm, struct am_hsm_state state);

/**
 * Get HSM submachine instance.
 *
 * Always returns the submachine instance of the calling state function.
 * Calling the function from a state that is not part of any submachine
 * will always return 0.
 *
 * @param hsm  the HSM
 *
 * @return the submachine instance
 */
int am_hsm_get_instance(const struct am_hsm *hsm);

/**
 * Get HSM's active state.
 *
 * E.g., assume HSM is in state S11,
 * which is a substate of S1, which is in turn a substate of S.
 * In this case this function always returns S11.
 *
 * @param hsm  the HSM
 *
 * @return the active state
 */
struct am_hsm_state am_hsm_get_state(const struct am_hsm *hsm);

/**
 * HSM constructor.
 *
 * @param hsm    the HSM to construct
 *
 * @param state  the initial state of the HSM object
 *               The initial state must return
 *               AM_HSM_TRAN(s) or AM_HSM_TRAN(s, i)
 */
void am_hsm_ctor(struct am_hsm *hsm, struct am_hsm_state state);

/**
 * HSM destructor.
 *
 * @param hsm  the HSM to destruct
 */
void am_hsm_dtor(struct am_hsm *hsm);

/**
 * Perform HSM initial transition.
 *
 * Call the initial state set by am_hsm_ctor() with provided
 * optional initial event.
 *
 * @param hsm         the HSM to init
 * @param init_event  the init event. Can be NULL.
 */
void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event);

/**
 * Set spy user callback as one place to catch all events for the given HSM.
 *
 * Is only available if hsm.c is compiled with AM_HSM_SPY defined.
 * Should only be used for debugging purposes.
 * Should only be called after calling am_hsm_ctor() and not during ongoing
 * HSM event processing.
 *
 * @param hsm  the HSM to spy
 * @param spy  the spy callback. Use NULL to unset.
 */
void am_hsm_set_spy(struct am_hsm *hsm, am_hsm_spy_fn spy);

/**
 * Ultimate top state of any HSM.
 *
 * Every HSM has the same explicit top state, which surrounds all other elements
 * of the entire state machine.
 * One should never target the top state in a state transition.
 *
 * Has the same signature as #am_hsm_state_fn
 */
enum am_hsm_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event);

#ifdef __cplusplus
}
#endif

#endif /* AM_HSM_H_INCLUDED */
