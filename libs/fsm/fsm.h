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
 * Finite State Machine (FSM) library API declaration.
 */

#ifndef AM_FSM_H_INCLUDED
#define AM_FSM_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/types.h"
#include "event/event_common.h"

/** Forward declaration of FSM descriptor. */
struct am_fsm;

/**
 * FSM state/event handler function type.
 *
 * @param fsm    FSM that receives @p event.
 * @param event  Event to handle.
 *
 * @retval AM_RC_CORO_BUSY         Event was handled.
 * @retval AM_RC_CORO_DONE         Event was handled.
 * @retval AM_RC_HANDLED           Event was handled.
 * @retval AM_RC_TRAN              Event caused a state transition.
 * @retval AM_RC_TRAN_REDISPATCH   Event caused a state transition and the same
 *                                 event shall be redispatched.
 */
typedef enum am_rc (*am_fsm_state_fn)(
    struct am_fsm* fsm, const struct am_event* event
);

/**
 * FSM descriptor.
 *
 * None of the fields of the descriptor should be accessed directly by user
 * code. The descriptor is exposed only so user code can reserve memory for it.
 */
struct am_fsm {
    /** Active state/event handler function. */
    am_fsm_state_fn state;
    /** Safety net to catch a missing am_fsm_init() call. */
    uint8_t init_called : 1;
    /** Safety net to catch an erroneous reentrant am_fsm_dispatch() call. */
    uint8_t dispatch_in_progress : 1;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Finish event processing without triggering a state transition.
 *
 * Use this as a return value from a state handler that handled an event.
 *
 * @param fsm  FSM that is processing the event.
 *
 * @retval AM_RC_HANDLED  Event was handled.
 */
static inline enum am_rc am_fsm_handled(struct am_fsm* fsm) {
    (void)fsm;
    return AM_RC_HANDLED;
}

/**
 * Trigger a transition to another state.
 *
 * It should never be returned in response to #AM_EVT_ENTRY or #AM_EVT_EXIT
 * events.
 *
 * @param fsm  FSM that is processing the event.
 * @param fn   Target FSM state/event handler function.
 *
 * @retval AM_RC_TRAN  State transition was triggered.
 */
static inline enum am_rc am_fsm_tran(struct am_fsm* fsm, am_fsm_state_fn fn) {
    fsm->state = fn;
    return AM_RC_TRAN;
}

/**
 * Trigger a transition and request redispatch of the same event.
 *
 * It should never be returned in response to #AM_EVT_ENTRY or #AM_EVT_EXIT
 * events.
 *
 * Do not redispatch the same event more than once within the same
 * am_fsm_dispatch() call.
 *
 * @param fsm  FSM that is processing the event.
 * @param fn   Target FSM state/event handler function.
 *
 * @retval AM_RC_TRAN_REDISPATCH  State transition was triggered and the same
 *                                event shall be redispatched.
 */
static inline enum am_rc am_fsm_tran_redispatch(
    struct am_fsm* fsm, am_fsm_state_fn fn
) {
    fsm->state = fn;
    return AM_RC_TRAN_REDISPATCH;
}

/**
 * Synchronously dispatch an event to an FSM.
 *
 * The caller retains ownership of @p event. Do not free the event from user
 * state handlers.
 *
 * @param fsm    FSM to dispatch @p event to.
 * @param event  Event to dispatch.
 */
void am_fsm_dispatch(struct am_fsm* fsm, const struct am_event* event);

/**
 * Check whether an FSM is in a given state.
 *
 * Use sparingly to check states of other state machines, because it breaks
 * encapsulation.
 *
 * @param fsm    FSM to query.
 * @param state  State to check.
 *
 * @retval true   @p fsm is in @p state.
 * @retval false  @p fsm is not in @p state.
 */
bool am_fsm_is_in(const struct am_fsm* fsm, am_fsm_state_fn state);

/**
 * Get an FSM's active state.
 *
 * @param fsm  FSM to query.
 *
 * @return Active FSM state/event handler function.
 */
am_fsm_state_fn am_fsm_get_state(const struct am_fsm* fsm);

/**
 * Construct an FSM object.
 *
 * The initial state handler must return am_fsm_tran().
 *
 * @param fsm    FSM object to construct.
 * @param state  Initial state/event handler function of @p fsm.
 */
void am_fsm_create(struct am_fsm* fsm, am_fsm_state_fn state);

/**
 * Destruct an FSM object.
 *
 * Exits the current state.
 *
 * The FSM is not usable after this call. Call am_fsm_create() before using the
 * FSM again.
 *
 * @param fsm  FSM object to destruct.
 */
void am_fsm_destroy(struct am_fsm* fsm);

/**
 * Perform an FSM initial transition.
 *
 * Calls the initial state handler set by am_fsm_create() with @p init_event and
 * performs the initial transition.
 *
 * @param fsm         FSM to initialize.
 * @param init_event  Initial event, or NULL.
 */
void am_fsm_init(struct am_fsm* fsm, const struct am_event* init_event);

#ifdef __cplusplus
}
#endif

#endif /* AM_FSM_H_INCLUDED */
