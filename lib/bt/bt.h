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

#ifdef __cplusplus
extern "C" {
#endif

AM_ASSERT_STATIC(AM_HSM_EVT_MAX == 4);

#define AM_BT_EVT_SUCCESS 5
#define AM_BT_EVT_FAILURE 6
#define AM_BT_EVT_DELAY 7
#define AM_BT_EVT_MAX AM_BT_EVT_DELAY

AM_ASSERT_STATIC(AM_EVT_USER > AM_BT_EVT_MAX);

enum am_bt_type {
    AM_BT_TYPES_MIN = 0,
    AM_BT_INVERT = AM_BT_TYPES_MIN,
    AM_BT_FORCE_SUCCESS,
    AM_BT_FORCE_FAILURE,
    AM_BT_REPEAT,
    AM_BT_RETRY_UNTIL_SUCCESS,
    AM_BT_RUN_UNTIL_FAILURE,
    AM_BT_DELAY,
    AM_BT_FALLBACK,
    AM_BT_SEQUENCE,
    AM_BT_TYPES_NUM
};

struct am_bt_cfg {
    struct am_dlist_item item;
    struct am_hsm *hsm;
    void (*post)(struct am_hsm *hsm, const struct am_event *event);
};

struct am_bt_node {
    struct am_hsm_state super;
};

struct am_bt_invert {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

struct am_bt_force_success {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

struct am_bt_force_failure {
    struct am_bt_node node;
    struct am_hsm_state substate;
};

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

struct am_bt_fallback {
    struct am_bt_node node;
    const struct am_hsm_state *substates;
    int nsubstates;
    int isubstate;
    unsigned init_done : 1;
};

struct am_bt_sequence {
    struct am_bt_node node;
    const struct am_hsm_state *substates;
    int nsubstates;
    int isubstate;
    unsigned init_done : 1;
};

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

enum am_hsm_rc am_bt_fallback(struct am_hsm *me, const struct am_event *event);
enum am_hsm_rc am_bt_sequence(struct am_hsm *me, const struct am_event *event);

void am_bt_add_cfg(struct am_bt_cfg *cfg);

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
void am_bt_add_fallback(struct am_bt_fallback *nodes, int num);
void am_bt_add_sequence(struct am_bt_sequence *nodes, int num);

struct am_bt_cfg *am_bt_get_cfg(struct am_hsm *hsm);
void am_bt_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
