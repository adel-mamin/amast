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
 * Interpreted Hierarchical State Machine (IHSM) module API declaration.
 *
 * Configuration defines:
 *
 * - AM_IHSM_SPY                  - enables HSM spy callback for debugging
 * - AM_IHSM_STATE_NAME_SIZE_MAX  - state names max size [bytes]
 * - AM_IHSM_EVENT_NAME_SIZE_MAX  - event names max size [bytes]
 * - AM_IHSM_ACTION_NAME_SIZE_MAX - action names max size [bytes]
 */

#ifndef AM_IHSM_H_INCLUDED
#define AM_IHSM_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "event/event.h"
#include "hsm/hsm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Only these types of HSM JSON descriptions formats are supported. */
enum am_ihsm_json { AM_IHSM_JSON_SMCAT = 0 };

#ifndef AM_IHSM_STATE_NAME_SIZE_MAX
#define AM_IHSM_STATE_NAME_SIZE_MAX 16 /**< State names max size [bytes]. */
#endif

#ifndef AM_IHSM_EVENT_NAME_SIZE_MAX
#define AM_IHSM_EVENT_NAME_SIZE_MAX 16 /**< Event names max size [bytes]. */
#endif

#ifndef AM_IHSM_ACTION_NAME_SIZE_MAX
#define AM_IHSM_ACTION_NAME_SIZE_MAX 16 /**< Action names max size [bytes]. */
#endif

/** IHSM processing return codes. */
enum am_ihsm_rc {
    /* success */
    AM_IHSM_RC_OK = 0,
    /* not enough memory to process HSM JSON model */
    AM_IHSM_RC_ERR_NOMEM = -1,
    /* model type is not listed in enum am_ihsm_json */
    AM_IHSM_RC_ERR_UNKNOWN_MODEL = -2,
    /* invalid HSM JSON description */
    AM_IHSM_RC_ERR_MALFORMED_MODEL = -3,
    /* user action failure */
    AM_IHSM_RC_ERR_ACTION = -4,
};

/** IHSM transition descriptor. */
struct am_ihsm_tran {
    /** transition to this state */
    uint8_t to;
    /** pointer to next transition descriptor */
    uint8_t next_tran;
    /**
     * Internal transition.
     * Does not trigger exit & entry actions.
     */
    uint8_t internal : 1;

    /** event hash to speed up event lookup */
    uint32_t event_hash;
    /** event ID */
    int event_id;
    /** event name */
    char event_name[AM_IHSM_EVENT_NAME_SIZE_MAX];
    /** action name */
    char action_name[AM_IHSM_ACTION_NAME_SIZE_MAX];
};

/** IHSM state type */
enum am_ihsm_state_type {
    /** initial state */
    AM_IHSM_STATE_TYPE_INITIAL,
    /** regular state */
    AM_IHSM_STATE_TYPE_REGULAR,
    /** choice state */
    AM_IHSM_STATE_TYPE_CHOICE,
    /** final state */
    AM_IHSM_STATE_TYPE_FINAL,
};

/** IHSM state descriptor */
struct am_ihsm_state {
    /** super state of this state */
    uint8_t super;
    /** this substate */
    uint8_t state;
    /** history substate */
    uint8_t history;
    /**
     * If store_to_history == 1 and deep_history == 1,
     * then all substates should register themselves to
     * struct am_ihsm_state::history
     *
     * If store_to_history == 1 and deep_history == 0,
     * then only immediate substates should register themselves to
     * struct am_ihsm_state::history
     */
    uint8_t store_to_history : 1;
    /**
     * valid if struct am_ihsm_state::store_to_history is set
     * 1 - deep history, 0 - shallow history
     */
    uint8_t deep_history : 1;
    /** enum am_ihsm_state_type */
    uint8_t type : 2;
    /** pointer to struct am_ihsm_tran list */
    uint8_t tran_list;
    /** state name */
    char name[AM_IHSM_STATE_NAME_SIZE_MAX];
    /** entry action */
    char entry_action[AM_IHSM_ACTION_NAME_SIZE_MAX];
    /** exit action */
    char exit_action[AM_IHSM_ACTION_NAME_SIZE_MAX];
};

/** IHSM memory block. */
union am_ihsm_mem_block {
    /** IHSM state descriptor */
    struct am_ihsm_state state;
    /** IHSM transition descriptor */
    struct am_ihsm_tran tran;
};

/** IHSM event */
struct am_ihsm_event {
    /** Base structure. Must be first. */
    struct am_event event;
    /** event name */
    char name[AM_IHSM_EVENT_NAME_SIZE_MAX];
};

/** HSM model is accommodated in memory blocks of this size */
#define AM_IHSM_MEM_BLOCK_SIZE sizeof(union am_ihsm_mem_block)

/** forward declaration */
struct am_ihsm;

/**
 * All HSM errors are expected to be handled by this user callback.
 *
 * The IHSM APIs, which are allowed to be called from the callback:
 *
 * - am_ihsm_is_in()
 * - am_ihsm_state_is_eq()
 * - am_ihsm_get_state()
 * - am_ihsm_get_event()
 * - am_ihsm_get_action()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_spy()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_error_fn()
 * - am_ihsm_set_choice_fn()
 *
 * Should not block.
 *
 * @param ihsm    the IHSM
 * @param rc      the error
 *
 * @retval true   IHSM terminates HSM (calls am_ihsm_term()) after the call.
 *                This is also default IHSM behavior if the callback is not set.
 * @retval false  IHSM continues HSM execution in case of #AM_IHSM_RC_ERR_ACTION
 *                error
 */
typedef bool (*am_ihsm_error_fn)(struct am_ihsm *ihsm, enum am_ihsm_rc rc);

/**
 * All HSM actions are expected to be handled by this user callback.
 *
 * The IHSM APIs, which are allowed to be called from the callback:
 *
 * - am_ihsm_is_in()
 * - am_ihsm_state_is_eq()
 * - am_ihsm_get_state()
 * - am_ihsm_get_event()
 * - am_ihsm_get_action()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_spy()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_error_fn()
 * - am_ihsm_set_choice_fn()
 *
 * Should not block.
 *
 * @param ihsm    the IHSM
 * @param event   the event, which triggers action
 * @param action  action name as specified in HSM JSON model description
 *
 * @retval AM_IHSM_RC_OK          success
 * @retval AM_IHSM_RC_ERR_ACTION  failure.
 *                                IHSM calls am_ihsm_error_fn() callback if set
 *                                and optionally calls am_ihsm_term() for HSM.
 */
typedef enum am_ihsm_rc (*am_ihsm_action_fn)(
    struct am_ihsm *ihsm, const struct am_ihsm_event *event, const char *action
);

/**
 * All HSM choices and guards are expected to be handled by this user callback.
 *
 * The IHSM APIs, which are allowed to be called from the callback:
 *
 * - am_ihsm_is_in()
 * - am_ihsm_state_is_eq()
 * - am_ihsm_get_state()
 * - am_ihsm_get_event()
 * - am_ihsm_get_action()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_spy()
 * - am_ihsm_set_action_fn()
 * - am_ihsm_set_error_fn()
 * - am_ihsm_set_choice_fn()
 *
 * Should not block.
 *
 * @param ihsm     the IHSM
 * @param choice   the choice to make
 * @param options  the options to choose from. Terminated with NULL pointer.
 *
 * @return choice option.
 *         Must match one of the given choice options or set to NULL,
 *         if none was chosen. IHSM handles the NULL choice by not
 *         taking any transition and bubbling the event up to superstates.
 *         of the state.
 */
typedef const char *(*am_ihsm_choice_fn)(
    struct am_ihsm *ihsm, const char *choice, const char **options
);

/**
 * HSM spy callback type.
 *
 * Used as one place to catch all events for the given HSM.
 * Called on each user event BEFORE the event is processes by the HSM.
 * Should only be used for debugging purposes.
 * Set by am_ihsm_set_spy().
 * Only supported if "hsm.c" file is compiled with AM_IHSM_SPY defined.
 * Should not block.
 *
 * @param ihsm   the IHSM to spy
 * @param event  the event to spy
 */
typedef void (*am_ihsm_spy_fn)(
    struct am_ihsm *ihsm, const struct am_ihsm_event *event
);

/** IHSM state. */
struct am_ihsm {
    /** Base class. Must be first. */
    struct am_hsm hsm;
    /** the init state */
    unsigned init;
    /** currently processed (active) event */
    const struct am_ishm_event *active_event;
    /** currently executed (active) action */
    const char *active_action;
    /** IHSM spy callback */
    am_ihsm_spy_fn spy;
    /** IHSM action callback */
    am_ihsm_action_fn action;
    /** IHSM error callback */
    am_ihsm_error_fn error;
    /** IHSM choice callback */
    am_ihsm_choice_fn choice;
};

/**
 * Synchronous dispatch of event to IHSM.
 *
 * Does not free the event - this is caller's responsibility.
 *
 * @param ihsm   the IHSM
 * @param event  the event to dispatch
 */
void am_ihsm_dispatch(struct am_ihsm *ihsm, const struct am_ihsm_event *event);

/**
 * Test whether IHSM is in a given state.
 *
 * Note that an IHSM is in all superstates of the active state.
 * Use sparingly to test the active state of other state machine as
 * it breaks encapsulation.
 *
 * @param ihsm   the IHSM
 * @param state  the state to check
 *
 * @retval false  not in the state in the hierarchical sense
 * @retval true   in the state
 */
bool am_ihsm_is_in(struct am_ihsm *ihsm, const char *state);

/**
 * Check if IHSM's active state equals to \p state (not in hierarchical sense).
 *
 * If active state of ihsm is "S1", which is substate of "S", then
 * am_ihsm_state_is_eq(hsm, "S1") is true, but
 * am_ihsm_state_is_eq(hsm, "S") is false.
 *
 * @param ihsm   the IHSM
 * @param state  the state to compare against
 *
 * @retval true   the active IHSM state equals \p state
 * @retval false  the active IHSM state DOES NOT equal \p state
 */
bool am_ihsm_state_is_eq(const struct am_ihsm *ihsm, const char *state);

/**
 * Get IHSM's active state.
 *
 * E.g., assume IHSM is in state "S11",
 * which is a substate of "S1", which is in turn a substate of "S".
 * In this case this function always returns "S11".
 *
 * @param ihsm  the IHSM
 *
 * @return the active state
 */
const char *am_ihsm_get_state(const struct am_ihsm *ihsm);

/**
 * Get IHSM's active event.
 *
 * @param ihsm  the IHSM
 *
 * @return the active event. Owned by IHSM.
 */
const struct am_ihsm_event *am_ihsm_get_event(const struct am_ihsm *ihsm);

/**
 * Get IHSM's active action.
 *
 * @param ihsm  the IHSM
 *
 * @return the active action. Owned by IHSM.
 */
const char *am_ihsm_get_action(const struct am_ihsm *ihsm);

/**
 * IHSM constructor.
 *
 * @param ihsm  the IHSM to construct
 */
void am_ihsm_ctor(struct am_ihsm *ihsm);

/**
 * Set IHSM memory pool.
 *
 * Can only be called before am_ihsm_init() or after am_ihsm_term() calls.
 *
 * @param ihsm  add memory pool to this IHSM
 * @param pool  the memory pool pointer
 * @param size  the memory pool size [bytes]
 */
void am_ihsm_set_pool(struct am_ihsm *ihsm, void *pool, int size);

/**
 * IHSM destructor.
 *
 * Exits all HSM states.
 * Call am_ihsm_ctor() to construct IHSM again.
 *
 * @param ihsm  the IHSM to destruct
 */
void am_ihsm_dtor(struct am_ihsm *ihsm);

/**
 * Set action callback.
 *
 * @param ihsm    the IHSM
 * @param action  the callback
 */
void am_ihsm_set_action_fn(struct am_ihsm *ihsm, am_ihsm_action_fn action);

/**
 * Set error callback.
 *
 * @param ihsm   the IHSM
 * @param error  the callback
 */
void am_ihsm_set_error_fn(struct am_ihsm *ihsm, am_ihsm_error_fn error);

/**
 * Set choice callback.
 *
 * @param ihsm    the IHSM
 * @param choice  the callback
 */
void am_ihsm_set_choice_fn(struct am_ihsm *ihsm, am_ihsm_choice_fn choice);

/**
 * Load IHSM with HSM JSON description.
 *
 * IHSM extracts all necessary data from JSON description into
 * internal buffers and so caller can recycle the memory pointed to by \p json.
 *
 * Can be called without calling am_ihsm_set_pool() API first.
 * Called this way the API returns the number of memory blocks
 * union am_ihsm_mem_block required to accommodate the HSM model.
 *
 * @param ihsm  the IHSM
 * @param type  IHSM JSON type
 * @param json  HSM JSON description. Not used after the call.
 *
 * @retval 0   success
 * @retval >0  no memory failure.
 *             Number of required memory blocks of
 *             size sizeof(union am_ihsm_mem_block).
 *             Call am_ihsm_set_pool() to provide memory pool of
 *             the required size.
 * @retval <0  enum am_ihsm_rc type of error
 */
int am_ihsm_load(
    struct am_ihsm *ihsm, enum am_ihsm_json type, const char *json
);

/**
 * Perform IHSM initial transition.
 *
 * @param ihsm  the IHSM to init
 */
void am_ihsm_init(struct am_ihsm *ihsm);

/**
 * Terminate IHSM.
 *
 * Exists all HSM states.
 * Call am_ihsm_init() to initialize IHSM again.
 *
 * @param ihsm  the IHSM to terminate.
 */
void am_ihsm_term(struct am_ihsm *ihsm);

/**
 * Set spy user callback as one place to catch all events for the given IHSM.
 *
 * Should only be used for debugging purposes.
 * Should only be called after calling am_ihsm_ctor() and not during ongoing
 * IHSM event processing.
 *
 * @param ihsm  the IHSM to spy
 * @param spy   the spy callback. Use NULL to unset.
 */
void am_ihsm_set_spy(struct am_ihsm *ihsm, am_ihsm_spy_fn spy);

#ifdef __cplusplus
}
#endif

#endif /* AM_IHSM_H_INCLUDED */
