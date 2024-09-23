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
 * Behavioral tree API declaration.
 */

#ifndef BT_H_INCLUDED
#define BT_H_INCLUDED

#include <stdbool.h>
#include "dlist/dlist.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "timer/timer.h"

AM_ASSERT_STATIC(AM_HSM_EVT_MAX == 4);

/** BT events */
#define AM_BT_EVT_SUCCESS 5
#define AM_BT_EVT_FAILURE 6
#define AM_BT_EVT_DELAY 7
#define AM_BT_EVT_PARALLEL 8
#define AM_BT_EVT_COUNT 9
#define AM_BT_EVT_MAX AM_BT_EVT_COUNT

AM_ASSERT_STATIC(AM_EVT_USER > AM_BT_EVT_MAX);

/**
 * BT configuration.
 * BTs used by different HSMs may have different configurations.
 */
struct am_bt_cfg {
    struct am_dlist_item item;
    struct am_hsm *hsm;
    void (*post)(struct am_hsm *hsm, const struct am_event *event);
};

/** BT node */
struct am_bt_node {
    struct am_hsm_state super;
};

/** BT invert node state */
struct am_bt_invert {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

/** BT force success node state */
struct am_bt_force_success {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

/** BT force failure node state */
struct am_bt_force_failure {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

/** BT repeat node state */
struct am_bt_repeat {
    struct am_bt_node node;
    struct am_hsm_state substate;
    int total;
    int done;
};

/** am_bt_retry_until_success BT node state */
struct am_bt_retry_until_success {
    struct am_bt_node node;       /** super state */
    struct am_hsm_state substate; /** substate */
    int attempts_total; /** set to -1 for infinite number of attempts */
    int attempts_done;  /** number of attempts done so far */
};

/** am_bt_run_until_failure BT node state */
struct am_bt_run_until_failure {
    struct am_bt_node node;       /** super state */
    struct am_hsm_state substate; /** substate */
};

/** am_bt_delay BT node state */
struct am_bt_delay {
    struct am_bt_node node;
    struct am_hsm_state substate;
    struct am_event_timer delay; /** the delay timeout timer event */
    int delay_ticks;             /** the delay [ticks] */
    int domain;                  /** the delay timer tick domain */
};

/** BT count node state */
struct am_bt_count {
    struct am_bt_node node;
    struct am_hsm_state substate;
    /**
     * the total number of AM_BT_EVT_SUCCESS and AM_BT_EVT_FAILURE events
     * expected from substate
     */
    int ntotal;
    /**
     * wait for AM_BT_EVT_SUCCESS from at least this many times
     * before completing the count node
     */
    int success_min;
    int success_cnt; /** how many times substate reported success */
    int failure_cnt; /** how many times substate reported failure */
};

/** am_bt_fallback BT node state */
struct am_bt_fallback {
    struct am_bt_node node;
    const struct am_hsm_state *substates;
    int nsubstates;         /** number of substates */
    int isubstate;          /** currently running substate */
    unsigned init_done : 1; /** the fallback initialization status */
};

/** BT sequence node state */
struct am_bt_sequence {
    struct am_bt_node node;
    const struct am_hsm_state *substates;
    int nsubstates;         /** number of substates */
    int isubstate;          /** currently running substate */
    unsigned init_done : 1; /** the sequence initialization status */
};

/** BT sub-HSM. Used by BT parallel node. */
struct am_bt_subhsm {
    void (*ctor)(struct am_hsm *hsm, struct am_hsm *super);
    struct am_hsm *hsm;
};

/** BT parallel node state */
struct am_bt_parallel {
    struct am_bt_node node;
    const struct am_bt_subhsm *subhsms;
    int nsubhsms; /** number of substates */
    /**
     * wait for AM_BT_EVT_SUCCESS from at least this many sub-HSMs
     * before completing the parallel node
     */
    int success_min;
    int success_cnt; /** how many sub-HSMs completed with success */
    int failure_cnt; /** how many sub-HSMs completed with failure */
};

#ifdef __cplusplus
extern "C" {
#endif

/** canned events */
extern const struct am_event am_bt_evt_success;
extern const struct am_event am_bt_evt_failure;

/**
 * Invert the result of substate operation and send it to superstate.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_invert` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_invert(struct am_hsm *hsm, const struct am_event *event);

/**
 * Force AM_BT_EVT_SUCCESS of substate operation and send it to superstate.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_force_success` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_force_success(
    struct am_hsm *hsm, const struct am_event *event
);

/**
 * Force AM_BT_EVT_FAILURE of substate operation and send it to superstate.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_force_failure` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_force_failure(
    struct am_hsm *hsm, const struct am_event *event
);

/**
 * Run substate up to `struct am_bt_repeat::total` times as long
 * as the substate returns AM_BT_EVT_SUCCESS.
 * Interrupt the repetition if substate returns AM_BT_EVT_FAILURE.
 * Return AM_BT_EVT_SUCCESS if all repetitions were successful.
 * Otherwise return AM_BT_EVT_FAILURE.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once for each repetition. Otherwise behavior is undefined.
 * Configured with `struct am_bt_repeat` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_repeat(struct am_hsm *hsm, const struct am_event *event);

/**
 * Run substate up to `struct am_bt_retry_until_success::attempts_total` times
 * until the substate returns AM_BT_EVT_SUCCESS.
 * If `struct am_bt_retry_until_success::attempts_total` is -1, then
 * the number of attempts is unlimited.
 * Interrupt the retries if substate returns AM_BT_EVT_SUCCESS.
 * Return AM_BT_EVT_SUCCESS in this case.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once for each repetition. Otherwise behavior is undefined.
 * Configured with `struct am_bt_retry_until_success` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_retry_until_success(
    struct am_hsm *me, const struct am_event *event
);

/**
 * Keep running (re-entering) substate while it returns AM_BT_EVT_SUCCESS.
 * Stop once it returns AM_BT_EVT_FAILURE.
 * Return AM_BT_EVT_FAILURE in this case.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once for each repetition. Otherwise behavior is undefined.
 * Configured with `struct am_bt_run_until_failure` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_run_until_failure(
    struct am_hsm *me, const struct am_event *event
);

/**
 * Run substate once after a delay timeout.
 * The substate is expected to return AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE
 * only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_delay` instance.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_delay(struct am_hsm *me, const struct am_event *event);

/**
 * Run substate once and wait for a configured number of AM_BT_EVT_SUCCESS
 * events from it.
 * The total number of AM_BT_EVT_SUCCESS or AM_BT_EVT_FAILURE events
 * from substate is limited to a configured number. Otherwise the behavior is
 * undefined.
 * Configured with `struct am_bt_count` instance.
 * Returns AM_BT_EVT_SUCCESS if at least `struct am_bt_count::success_min`
 * is received. Otherwise AM_BT_EVT_FAILURE is returned.
 * It is a decorator node.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_count(struct am_hsm *me, const struct am_event *event);

/**
 * Run each substate once until a substate returns AM_BT_EVT_SUCCESS,
 * in which case AM_BT_EVT_SUCCESS is returned.
 * If all substates return AM_BT_EVT_FAILURE, then AM_BT_EVT_FAILURE
 * is returned.
 * Each substate, once activated is expected to return AM_BT_EVT_SUCCESS or
 * AM_BT_EVT_FAILURE only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_fallback` instance.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_fallback(struct am_hsm *me, const struct am_event *event);

/**
 * Run each substate once until a substate returns AM_BT_EVT_FAILURE,
 * in which case AM_BT_EVT_FAILURE is returned.
 * If all substates return AM_BT_EVT_SUCCESS, then AM_BT_EVT_SUCCESS
 * is returned.
 * Each substate, once activated is expected to return AM_BT_EVT_SUCCESS or
 * AM_BT_EVT_FAILURE only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_sequence` instance.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_sequence(struct am_hsm *me, const struct am_event *event);

/**
 * Run all sub-HSMs once in parallel and wait until a specified
 * number of them complete with AM_BT_EVT_SUCCESS,
 * In this case AM_BT_EVT_SUCCESS is returned.
 * If all sub-HSMs return AM_BT_EVT_FAILURE, then AM_BT_EVT_FAILURE is returned.
 * Each sub-HSM, once activated is expected to return AM_BT_EVT_SUCCESS or
 * AM_BT_EVT_FAILURE only once. Otherwise behavior is undefined.
 * Configured with `struct am_bt_parallel` instance.
 * Complies to am_hsm_state_fn type.
 */
enum am_hsm_rc am_bt_parallel(struct am_hsm *me, const struct am_event *event);

/** Add BT nodes */
void am_bt_add_invert(struct am_bt_invert *nodes, int num);
void am_bt_add_force_success(struct am_bt_force_success *nodes, int num);
void am_bt_add_force_failure(struct am_bt_force_failure *nodes, int num);
void am_bt_add_repeat(struct am_bt_repeat *nodes, int num);
void am_bt_add_retry_until_success(
    struct am_bt_retry_until_success *nodes, int num
);
void am_bt_add_run_until_failure(
    struct am_bt_run_until_failure *nodes, int num
);
void am_bt_add_delay(struct am_bt_delay *nodes, int num);
void am_bt_add_count(struct am_bt_count *nodes, int num);
void am_bt_add_fallback(struct am_bt_fallback *nodes, int num);
void am_bt_add_sequence(struct am_bt_sequence *nodes, int num);
void am_bt_add_parallel(struct am_bt_parallel *nodes, int num);

/**
 * Add BT configuration.
 * BT does not make a copy of the configuration.
 * So the caller must ensure the validity of the configuration
 * after the call.
 */
void am_bt_add_cfg(struct am_bt_cfg *cfg);

/** Construct BT module */
void am_bt_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
