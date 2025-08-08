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
 * Hierarchical State Machine (HSM) library API declaration.
 * Configuration defines:
 * AM_HSM_SPY - enables HSM spy callback support for debugging
 * AM_HSM_HIERARCHY_DEPTH_MAX - HSM hierarchy maximum depth
 */

#ifndef AM_HSM_H_INCLUDED
#define AM_HSM_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"

/** HSM events. */
enum am_hsm_evt_id {
    /**
     * Empty event.
     *
     * User event handlers should take care of not causing any side effects
     * when called with this event.
     *
     * The event handlers must always return the AM_HSM_SUPER() in response
     * to this event.
     */
    AM_EVT_HSM_EMPTY = AM_EVT_RANGE_SM_BEGIN,

    /**
     * Init event.
     *
     * Run optional initial transition from a given state.
     *
     * Always follows the #AM_EVT_HSM_ENTRY event.
     */
    AM_EVT_HSM_INIT,

    /**
     * Entry event.
     *
     * Run entry action(s) for a given state.
     *
     * Always precedes the #AM_EVT_HSM_INIT event.
     *
     * No state transition is allowed in response to this event.
     */
    AM_EVT_HSM_ENTRY,

    /**
     * Exit event.
     *
     * Run exit action(s) for a given state.
     *
     * No state transition is allowed in response to this event.
     */
    AM_EVT_HSM_EXIT,
};

AM_ASSERT_STATIC(AM_EVT_HSM_EXIT <= AM_EVT_RANGE_SM_END);

/** forward declaration of HSM descriptor */
struct am_hsm;

/**
 * HSM state (event handler) function type.
 *
 * Do not assume that a state handler is invoked only for
 * processing event IDs enlisted in case statements of internal
 * switch statement.
 *
 * Event handlers should avoid using any code outside
 * of the switch statement, especially the code, which has side effects.
 *
 * @param hsm    the HSM
 * @param event  the event to handle
 *
 * @return return code
 */
typedef enum am_rc (*am_hsm_state_fn)(
    struct am_hsm *hsm, const struct am_event *event
);

/**
 * HSM spy user callback type.
 *
 * Used as one place to catch all events for a given HSM.
 *
 * Called on each user event BEFORE the event is processes by the HSM.
 *
 * Should only be used for debugging purposes.
 *
 * Set by am_hsm_set_spy().
 *
 * Only supported, if the HSM library is compiled with `AM_HSM_SPY` defined.
 *
 * @param hsm    the handler of the HSM to spy
 * @param event  the event to spy
 */
typedef void (*am_hsm_spy_fn)(struct am_hsm *hsm, const struct am_event *event);

/** HSM state. */
struct am_hsm_state {
    /** HSM state (event handler) function */
    am_hsm_state_fn fn;
    /**
     * HSM submachine instance.
     *
     * Default is 0. Valid range is [0,255].
     */
    uint8_t smi;
};

/** Helper macro. Not to be used directly. */
#define AM_STATE1_(s) \
    (struct am_hsm_state){.fn = (am_hsm_state_fn)(s), .smi = 0}

/** Helper macro. Not to be used directly. */
#define AM_STATE2_(s, i) \
    (struct am_hsm_state) { .fn = (am_hsm_state_fn)(s), .smi = (uint8_t)(i) }

/**
 * Construct HSM state from HSM event handler and optionally
 * HSM submachine instance.
 *
 * Examples:
 *
 * AM_HSM_STATE_CTOR(s) is converted to
 *
 * @code{.c}
 * (struct am_hsm_state){.fn = s, .smi = 0}
 * @endcode
 *
 * AM_HSM_STATE_CTOR(s, i) is converted to
 *
 * @code{.c}
 * (struct am_hsm_state){.fn = s, .smi = i}
 * @endcode
 *
 * @def AM_HSM_STATE_CTOR(s, i)
 *
 * \a s  is the HSM event handler (mandatory)
 *
 * \a i  is the HSM submachine instance (optional, default is 0)
 *
 * @return constructed HSM state structure
 */
#define AM_HSM_STATE_CTOR(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_STATE2_, AM_STATE1_, _)(__VA_ARGS__)

/**
 * HSM hierarchy maximum depth.
 *
 * Deep HSM hierarchy increases state switch execution time overhead.
 */
#ifndef AM_HSM_HIERARCHY_DEPTH_MAX
#define AM_HSM_HIERARCHY_DEPTH_MAX 16
#endif

/** HSM hierarchy level representation bits number */
#define AM_HSM_HIERARCHY_LEVEL_BITS 5

/** HSM hierarchy level representation bits mask */
#define AM_HSM_HIERARCHY_LEVEL_MASK ((1U << AM_HSM_HIERARCHY_LEVEL_BITS) - 1)

AM_ASSERT_STATIC(AM_HSM_HIERARCHY_DEPTH_MAX <= AM_HSM_HIERARCHY_LEVEL_MASK);

/**
 * HSM descriptor.
 *
 * None of the fields of the descriptor are to be accessed directly
 * by user code. The only purpose of exposing it is to allow user
 * code to reserve memory for it.
 */
struct am_hsm {
    /** Active state. */
    struct am_hsm_state state;
    /**
     * While am_hsm::state::smi maintains submachine instance of active state,
     * am_hsm::smi maintains the transitive submachine instance that may differ
     * from am_hsm::state::smi, when an event is propagated up
     * from substates to superstates.
     * Returned by am_hsm_get_instance().
     */
    uint8_t smi;
    /**
     * Active state hierarchy level [0,#AM_HSM_HIERARCHY_DEPTH_MAX]
     * (level 0 is assigned to am_hsm_top()).
     * Used internally to speed up state transitions.
     */
    uint8_t hierarchy_level : AM_HSM_HIERARCHY_LEVEL_BITS;
    /** safety net to catch missing am_hsm_ctor() call */
    uint8_t ctor_called : 1;
    /** safety net to catch missing am_hsm_init() call */
    uint8_t init_called : 1;
    /** safety net to catch erroneous reentrant am_hsm_dispatch() call */
    uint8_t dispatch_in_progress : 1;
#ifdef AM_HSM_SPY
    /** HSM spy callback */
    am_hsm_spy_fn spy;
#endif
};

/**
 * Event processing is over. No state transition is triggered.
 *
 * Used as a return value from the event handler, which handled
 * an event and wants to prevent the event propagation to
 * superstate(s).
 */
#define AM_HSM_HANDLED() AM_RC_HANDLED

/** Helper macro. Not to be used directly. */
#define AM_HSM_SET_(s, i)                                    \
    (((struct am_hsm *)me)->state = AM_HSM_STATE_CTOR(s, i), \
     ((struct am_hsm *)me)->smi = (uint8_t)i)

/** Helper macro. Not to be used directly. */
#define AM_TRAN1_(s) (AM_HSM_SET_(s, 0), AM_RC_TRAN)
/** Helper macro. Not to be used directly. */
#define AM_TRAN2_(s, i) (AM_HSM_SET_(s, i), AM_RC_TRAN)

/**
 * Event processing is over. Transition is triggered.
 *
 * It should never be returned in response to
 * #AM_EVT_HSM_ENTRY or #AM_EVT_HSM_EXIT events.
 *
 * Conversely, the response to #AM_EVT_HSM_INIT event can optionally use
 * this macro as a return value to designate transition to
 * the provided state. The target state in this case must be
 * a substate of the current state.
 *
 * Examples:
 *
 * AM_HSM_TRAN(s) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = 0},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_TRAN)
 * @endcode
 *
 * AM_HSM_TRAN(s, i) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = i},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_TRAN)
 * @endcode
 *
 * Below are the parameters to the macro:
 *
 * @def AM_HSM_TRAN(s, i)
 *
 * \a s  is the new state of type #am_hsm_state_fn (mandatory)
 *
 * \a i  is HSM submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN2_, AM_TRAN1_, _)(__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP1_(s) (AM_HSM_SET_(s, 0), AM_RC_TRAN_REDISPATCH)
/** Helper macro. Not to be used directly. */
#define AM_TRAN_REDISP2_(s, i) (AM_HSM_SET_(s, i), AM_RC_TRAN_REDISPATCH)

/**
 * Same event redispatch is requested. Transition is triggered.
 *
 * It should never be returned for #AM_EVT_HSM_ENTRY, #AM_EVT_HSM_EXIT or
 * #AM_EVT_HSM_INIT events.
 * Do not redispatch the same event more than once within same
 * am_hsm_dispatch() call.
 *
 * Examples:
 *
 * AM_HSM_TRAN_REDISPATCH(s) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = 0},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_TRAN_REDISPATCH)
 * @endcode
 *
 * AM_HSM_TRAN_REDISPATCH(s, i) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = i},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_TRAN_REDISPATCH)
 * @endcode
 *
 * Below are the parameters to the macro:
 *
 * @def AM_HSM_TRAN_REDISPATCH(s, i)
 *
 * \a s  is the new HSM state of type #am_hsm_state_fn (mandatory)
 *
 * \a i  is the new HSM state submachine instance (optional, default is 0)
 */
#define AM_HSM_TRAN_REDISPATCH(...)                                     \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_TRAN_REDISP2_, AM_TRAN_REDISP1_, _) \
    (__VA_ARGS__)

/** Helper macro. Not to be used directly. */
#define AM_SUPER1_(s) (AM_HSM_SET_(s, 0), AM_RC_SUPER)
/** Helper macro. Not to be used directly. */
#define AM_SUPER2_(s, i) (AM_HSM_SET_(s, i), AM_RC_SUPER)

/**
 * Event processing is passed to superstate. No transition was triggered.
 *
 * If no explicit superstate exists, then the top superstate am_hsm_top()
 * must be used.
 *
 * Examples:
 *
 * AM_HSM_SUPER(s) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = 0},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_SUPER)
 * @endcode
 *
 * AM_HSM_SUPER(s, i) is converted to
 *
 * @code{.c}
 * (((struct am_hsm *)me)->state = (struct am_hsm_state){.fn = s, .smi = i},
 *  ((struct am_hsm *)me)->smi = (uint8_t)i, AM_RC_SUPER)
 * @endcode
 *
 * Below are the parameters to the macro:
 *
 * @def AM_HSM_SUPER(s, i)
 *
 * \a s  is the superstate of type #am_hsm_state_fn (mandatory)
 *
 * \a i  is the superstate submachine instance (optional, default is 0)
 */
#define AM_HSM_SUPER(...) \
    AM_GET_MACRO_2_(__VA_ARGS__, AM_SUPER2_, AM_SUPER1_, _)(__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Synchronous dispatch of event to a given HSM.
 *
 * Do not free the event in user event handlers -
 * this is caller's responsibility.
 *
 * @param hsm    the HSM
 * @param event  the event to dispatch
 */
void am_hsm_dispatch(struct am_hsm *hsm, const struct am_event *event);

/**
 * Check whether HSM is in a given state.
 *
 * Note that an HSM is simultaneously in all superstates of
 * current active state.
 *
 * Use sparingly to check states of other state machine(s) as
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
 * `am_hsm_state_is_eq``(hsm, AM_HSM_STATE_CTOR(S1))` is true, but
 * `am_hsm_state_is_eq``(hsm, AM_HSM_STATE_CTOR(S))` is false.
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
 * Always returns the submachine instance of the calling event handler
 * state function.
 *
 * Calling the function from an event handler state that is not part
 * of any submachine always returns 0.
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
 * Exits all HSM states.
 *
 * The HSM is not usable after this call.
 * Call am_hsm_ctor() to construct HSM again.
 *
 * @param hsm  the HSM to destruct
 */
void am_hsm_dtor(struct am_hsm *hsm);

/**
 * Perform HSM initial transition.
 *
 * Calls the initial state event handler set by am_hsm_ctor() with the provided
 * optional initial event and performs the initial transition including
 * all recursive initial transitions, if any.
 *
 * @param hsm         the HSM to init
 * @param init_event  the initial event. Can be NULL.
 */
void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event);

/**
 * Set spy user callback as a one place to catch all events for the given HSM.
 *
 * Is only available, if the HSM library is compiled with `AM_HSM_SPY` defined.
 *
 * Should only be used for debugging purposes.
 *
 * Should only be called after calling am_hsm_ctor().
 *
 * @param hsm  the HSM to spy
 * @param spy  the spy callback. Use NULL to unset.
 */
void am_hsm_set_spy(struct am_hsm *hsm, am_hsm_spy_fn spy);

/**
 * Ultimate top superstate of any HSM.
 *
 * Every HSM has the same explicit top superstate, which surrounds
 * all other elements of the entire state machine.
 *
 * Users should never target the top superstate in a state transition.
 *
 * Has the same signature as #am_hsm_state_fn.
 */
enum am_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event);

#ifdef __cplusplus
}
#endif

#endif /* AM_HSM_H_INCLUDED */
