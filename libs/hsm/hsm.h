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
 *
 * @par Configuration
 * - #AM_HSM_HIERARCHY_DEPTH_MAX: HSM hierarchy maximum depth.
 */

#ifndef AM_HSM_H_INCLUDED
#define AM_HSM_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"

/** Forward declaration of HSM descriptor. */
struct am_hsm;

/**
 * HSM state/event handler function type.
 *
 * Do not assume that a state handler is invoked only to process event IDs
 * listed in the state handler's internal switch statement.
 *
 * Event handlers should avoid code outside the switch statement, especially
 * code with side effects.
 *
 * @param hsm    HSM that receives @p event.
 * @param event  Event to handle.
 *
 * @retval AM_RC_SUPER             Propagate @p event to the superstate.
 * @retval AM_RC_CORO_BUSY         Event was handled; do not propagate it.
 * @retval AM_RC_CORO_DONE         Event was handled; do not propagate it.
 * @retval AM_RC_HANDLED           Event was handled; do not propagate it.
 * @retval AM_RC_TRAN              Event caused a state transition.
 * @retval AM_RC_TRAN_REDISPATCH   Event caused a state transition and the same
 *                                 event shall be redispatched.
 */
typedef enum am_rc (*am_hsm_state_fn)(
    struct am_hsm* hsm, const struct am_event* event
);

/**
 * HSM state descriptor.
 *
 * The descriptor identifies a state handler and, when the state belongs to a
 * submachine, the submachine instance of that state.
 */
struct am_hsm_state {
    /** HSM state/event handler function. */
    am_hsm_state_fn fn;
    /**
     * HSM submachine instance.
     *
     * Use 0 for states that do not belong to a submachine, or for the default
     * submachine instance.
     */
    uint8_t instance;
};

/**
 * HSM hierarchy maximum depth.
 *
 * Deeper HSM hierarchies increase state transition execution time overhead.
 */
#ifndef AM_HSM_HIERARCHY_DEPTH_MAX
#define AM_HSM_HIERARCHY_DEPTH_MAX 16
#endif

/** HSM hierarchy level representation bit count. */
#define AM_HSM_HIERARCHY_LEVEL_BITS 5

/** HSM hierarchy level representation bit mask. */
#define AM_HSM_HIERARCHY_LEVEL_MASK \
    ((1U << (unsigned)AM_HSM_HIERARCHY_LEVEL_BITS) - 1)

AM_ASSERT_STATIC(AM_HSM_HIERARCHY_DEPTH_MAX <= AM_HSM_HIERARCHY_LEVEL_MASK);

/**
 * HSM descriptor.
 *
 * None of the fields of the descriptor should be accessed directly by user
 * code. The descriptor is exposed only so user code can reserve memory for it.
 */
struct am_hsm {
    /** Active HSM state/event handler function. */
    am_hsm_state_fn state_fn;
    /** Active HSM state submachine instance. */
    uint8_t state_instance;
    /**
     * Transitive submachine instance.
     *
     * While am_hsm::state_instance stores the submachine instance of the active
     * state, am_hsm::instance stores the transitive submachine instance visible
     * to the currently executing state handler. The two values may differ when
     * an event is propagated from substates to superstates.
     *
     * This value is returned by am_hsm_get_instance().
     */
    uint8_t instance;
    /**
     * Active state hierarchy level in the range [0,
     * #AM_HSM_HIERARCHY_DEPTH_MAX].
     *
     * Level 0 is assigned to am_hsm_top(). Used internally to speed up state
     * transitions.
     */
    uint8_t hierarchy_level : AM_HSM_HIERARCHY_LEVEL_BITS;
    /** Safety net to catch a missing am_hsm_init() call. */
    uint8_t init_called : 1;
    /** Safety net to catch a missing am_hsm_start() call. */
    uint8_t start_called : 1;
    /** Safety net to catch an erroneous reentrant am_hsm_dispatch() call. */
    uint8_t dispatch_in_progress : 1;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Make an HSM state descriptor for the default submachine instance.
 *
 * Equivalent to am_hsm_state_i(fn, 0).
 *
 * @param fn  HSM state/event handler function.
 *
 * @return HSM state descriptor with submachine instance 0.
 */
static inline struct am_hsm_state am_hsm_state_make(am_hsm_state_fn fn) {
    return (struct am_hsm_state){.fn = fn, .instance = 0};
}

/**
 * Make an HSM state descriptor for a specific submachine instance.
 *
 * @param fn        HSM state/event handler function.
 * @param instance  HSM submachine instance.
 *
 * @return HSM state descriptor with submachine instance @p instance.
 */
static inline struct am_hsm_state am_hsm_state_make_i(
    am_hsm_state_fn fn, uint8_t instance
) {
    return (struct am_hsm_state){.fn = fn, .instance = instance};
}

/**
 * Finish event processing without triggering a state transition.
 *
 * Use this as a return value from a state handler that handled an event and
 * wants to prevent propagation to superstates.
 *
 * @param hsm  HSM that is processing the event.
 *
 * @retval AM_RC_HANDLED  Event was handled; do not propagate it.
 */
static inline enum am_rc am_hsm_handled(struct am_hsm* hsm) {
    (void)hsm;
    return AM_RC_HANDLED;
}

/**
 * Trigger a transition to a state in the default submachine instance.
 *
 * Equivalent to am_hsm_tran_i(@p hsm, @p fn, 0).
 *
 * It should never be returned in response to #AM_EVT_ENTRY or #AM_EVT_EXIT
 * events.
 *
 * In response to #AM_EVT_INIT, this function may be used to designate a
 * transition to @p fn. In that case, @p fn must be a substate of the current
 * state.
 *
 * @param hsm  HSM that is processing the event.
 * @param fn   Target HSM state/event handler function.
 *
 * @retval AM_RC_TRAN  State transition was triggered.
 */
static inline enum am_rc am_hsm_tran(struct am_hsm* hsm, am_hsm_state_fn fn) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = 0;
    return AM_RC_TRAN;
}

/**
 * Trigger a transition to a state in a specific submachine instance.
 *
 * It should never be returned in response to #AM_EVT_ENTRY or #AM_EVT_EXIT
 * events.
 *
 * In response to #AM_EVT_INIT, this function may be used to designate a
 * transition to @p fn. In that case, @p fn must be a substate of the current
 * state.
 *
 * @param hsm       HSM that is processing the event.
 * @param fn        Target HSM state/event handler function.
 * @param instance  Target HSM submachine instance.
 *
 * @retval AM_RC_TRAN  State transition was triggered.
 */
static inline enum am_rc am_hsm_tran_i(
    struct am_hsm* hsm, am_hsm_state_fn fn, uint8_t instance
) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = instance;
    return AM_RC_TRAN;
}

/**
 * Trigger a transition and request redispatch of the same event.
 *
 * Equivalent to am_hsm_tran_redispatch_i(@p hsm, @p fn, 0).
 *
 * It should never be returned in response to #AM_EVT_ENTRY, #AM_EVT_EXIT or
 * #AM_EVT_INIT events.
 *
 * Do not redispatch the same event more than once within the same
 * am_hsm_dispatch() call.
 *
 * @param hsm  HSM that is processing the event.
 * @param fn   Target HSM state/event handler function.
 *
 * @retval AM_RC_TRAN_REDISPATCH  State transition was triggered and the same
 *                                event shall be redispatched.
 */
static inline enum am_rc am_hsm_tran_redispatch(
    struct am_hsm* hsm, am_hsm_state_fn fn
) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = 0;
    return AM_RC_TRAN_REDISPATCH;
}

/**
 * Trigger a transition and request redispatch of the same event.
 *
 * It should never be returned in response to #AM_EVT_ENTRY, #AM_EVT_EXIT or
 * #AM_EVT_INIT events.
 *
 * Do not redispatch the same event more than once within the same
 * am_hsm_dispatch() call.
 *
 * @param hsm       HSM that is processing the event.
 * @param fn        Target HSM state/event handler function.
 * @param instance  Target HSM submachine instance.
 *
 * @retval AM_RC_TRAN_REDISPATCH  State transition was triggered and the same
 *                                event shall be redispatched.
 */
static inline enum am_rc am_hsm_tran_redispatch_i(
    struct am_hsm* hsm, am_hsm_state_fn fn, uint8_t instance
) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = instance;
    return AM_RC_TRAN_REDISPATCH;
}

/**
 * Propagate event processing to a superstate.
 *
 * Equivalent to am_hsm_super_i(@p hsm, @p fn, 0).
 *
 * Use this when the current state did not handle the event and no transition
 * was triggered. If no explicit superstate exists, use am_hsm_top().
 *
 * @param hsm  HSM that is processing the event.
 * @param fn   Superstate HSM state/event handler function.
 *
 * @retval AM_RC_SUPER  Propagate the event to @p fn.
 */
static inline enum am_rc am_hsm_super(struct am_hsm* hsm, am_hsm_state_fn fn) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = 0;
    return AM_RC_SUPER;
}

/**
 * Propagate event processing to a superstate in a specific submachine instance.
 *
 * Use this when the current state did not handle the event and no transition
 * was triggered. If no explicit superstate exists, use am_hsm_top().
 *
 * @param hsm       HSM that is processing the event.
 * @param fn        Superstate HSM state/event handler function.
 * @param instance  Superstate HSM submachine instance.
 *
 * @retval AM_RC_SUPER  Propagate the event to @p fn.
 */
static inline enum am_rc am_hsm_super_i(
    struct am_hsm* hsm, am_hsm_state_fn fn, uint8_t instance
) {
    hsm->state_fn = fn;
    hsm->state_instance = hsm->instance = instance;
    return AM_RC_SUPER;
}

/**
 * Synchronously dispatch an event to an HSM.
 *
 * The caller retains ownership of @p event. Do not free the event from user
 * state handlers.
 *
 * @param hsm    HSM to dispatch @p event to.
 * @param event  Event to dispatch.
 */
void am_hsm_dispatch(struct am_hsm* hsm, const struct am_event* event);

/**
 * Check whether an HSM is in a given state.
 *
 * This check is hierarchical: an HSM is considered to be in its active state
 * and in all superstates of the active state.
 *
 * Use sparingly to check states of other state machines, because it breaks
 * encapsulation.
 *
 * @param hsm    HSM to query.
 * @param state  State to check.
 *
 * @retval true   @p hsm is in @p state in the hierarchical sense.
 * @retval false  @p hsm is not in @p state in the hierarchical sense.
 */
bool am_hsm_is_in(struct am_hsm* hsm, struct am_hsm_state state);

/**
 * Check whether an HSM's active state is exactly equal to a state.
 *
 * This check is not hierarchical. For example, if the active state is S1, and
 * S1 is a substate of S, then am_hsm_state_is_eq(hsm, am_hsm_state_make(S1)) is
 * true, but am_hsm_state_is_eq(hsm, am_hsm_state_make(S)) is false.
 *
 * @param hsm    HSM to query.
 * @param state  State to compare against the active state.
 *
 * @retval true   The active HSM state equals @p state.
 * @retval false  The active HSM state does not equal @p state.
 */
bool am_hsm_state_is_eq(const struct am_hsm* hsm, struct am_hsm_state state);

/**
 * Get the current HSM submachine instance.
 *
 * Returns the submachine instance visible to the calling state handler.
 *
 * Calling this function from a state handler that is not part of any
 * submachine returns 0.
 *
 * @param hsm  HSM to query.
 *
 * @return Current HSM submachine instance.
 */
uint8_t am_hsm_get_instance(const struct am_hsm* hsm);

/**
 * Get an HSM's active state.
 *
 * For example, if the HSM is in state S11, and S11 is a substate of S1, which
 * is in turn a substate of S, this function returns S11.
 *
 * @param hsm  HSM to query.
 *
 * @return Active HSM state.
 */
struct am_hsm_state am_hsm_get_state(const struct am_hsm* hsm);

/**
 * Construct an HSM object.
 *
 * The initial state handler must return am_hsm_tran() or am_hsm_tran_i().
 *
 * @param hsm    HSM object to construct.
 * @param state  Initial state descriptor of @p hsm.
 */
void am_hsm_init(struct am_hsm* hsm, struct am_hsm_state state);

/**
 * Destruct an HSM object.
 *
 * Exits the current state and all of its superstates up to am_hsm_top().
 *
 * The HSM is not usable after this call. Call am_hsm_init() before using the
 * HSM again.
 *
 * @param hsm  HSM object to destruct.
 */
void am_hsm_deinit(struct am_hsm* hsm);

/**
 * Perform an HSM initial transition.
 *
 * Calls the initial state handler set by am_hsm_init() with @p init_event and
 * performs the initial transition, including all recursive initial transitions.
 *
 * @param hsm         HSM to initialize.
 * @param init_event  Initial event, or NULL.
 */
void am_hsm_start(struct am_hsm* hsm, const struct am_event* init_event);

/**
 * Ultimate top superstate of every HSM.
 *
 * Every HSM has the same explicit top superstate, which surrounds all other
 * elements of the state machine.
 *
 * User code should never target the top superstate in a state transition.
 *
 * @param hsm    HSM that is processing @p event.
 * @param event  Event being processed.
 *
 * @retval AM_RC_SUPER    The event was not handled by the top superstate.
 * @retval AM_RC_HANDLED  The event was handled by the top superstate.
 */
enum am_rc am_hsm_top(struct am_hsm* hsm, const struct am_event* event);

#ifdef __cplusplus
}
#endif

#endif /* AM_HSM_H_INCLUDED */
