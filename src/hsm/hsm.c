/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Adel Mamin
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
 * Hierarchical State Machine (HSM) framework API implementation.
 */

#include <stdbool.h>
#include <stddef.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"

/** The maximum depth of HSM hierarchy. */
#define HSM_HIERARCHY_DEPTH_MAX 16

/** Canned events */
static const struct event m_hsm_evt[] = {
    {.id = HSM_EVT_EMPTY},
    {.id = HSM_EVT_INIT},
    {.id = HSM_EVT_ENTRY},
    {.id = HSM_EVT_EXIT}
};

/**
 * Build ancestor chain path.
 * The path starts with 'from' state and ends with substate of #until state.
 * @param path   the path is placed here
 * @param from   placed to path[0]
 * @param until  the substate of it is placed to the end of path[]
 * @return the path length
 */
static int hsm_build(
    struct hsm *hsm,
    hsm_state_fn (*path)[HSM_HIERARCHY_DEPTH_MAX],
    hsm_state_fn from,
    hsm_state_fn until
) {
    struct hsm hsm_ = *hsm;
    hsm->state = hsm->temp = from;
    int len = 0;
    (*path)[len++] = from;
    enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
    ASSERT(HSM_STATE_SUPER == rc);
    while (hsm->temp != until) {
        ASSERT(len < HSM_HIERARCHY_DEPTH_MAX);
        (*path)[len++] = hsm->temp;
        rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        ASSERT(HSM_STATE_SUPER == rc);
    }
    *hsm = hsm_;
    return len;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 * @param hsm   the path belongs to this HSM
 * @param path  the path to enter
 * @param len   the path[] length
 */
static void hsm_enter(struct hsm *hsm, const hsm_state_fn *path, int len) {
    for (int i = len - 1; i >= 0; --i) {
        hsm->state = hsm->temp = path[i];
        enum hsm_rc rc = path[i](hsm, &m_hsm_evt[HSM_EVT_ENTRY]);
        ASSERT((HSM_STATE_SUPER == rc) || (HSM_STATE_HANDLED == rc));
    }
}

/**
 * Exit states starting from current one and finishing with substate of #until.
 * @param hsm    exit the states of this HSM
 * @param until  stop the exit when reaching this state (not exited)
 */
static void hsm_exit(struct hsm *hsm, hsm_state_fn until) {
    while (hsm->temp != until) {
        hsm->state = hsm->temp;
        enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        ASSERT(rc != HSM_STATE_TRAN);
        if (HSM_STATE_HANDLED == rc) {
            rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
            ASSERT(HSM_STATE_SUPER == rc);
        }
    }
}

/**
 * Recursively enter and init destination state.
 * 1. enter all states in #path[]
 * 2. init destination state
 * 3. if destination state requested the initial transition, then build #path[]
 *    and go to 1.
 * @param hsm   enter and init the states of this HSM
 * @param path  the path to enter.
 *              Initially is path from LCA substate to dst (both inclusive),
 *              then reused.
 * @param len   the #path[] length
 * @param dst   the destination state to enter and init
 */
static void hsm_enter_and_init(
    struct hsm *hsm,
    hsm_state_fn (*path)[HSM_HIERARCHY_DEPTH_MAX],
    int len,
    hsm_state_fn dst
) {
    hsm_enter(hsm, &(*path)[0], len);
    hsm->state = hsm->temp = dst;
    while (dst(hsm, &m_hsm_evt[HSM_EVT_INIT]) == HSM_STATE_TRAN) {
        len = hsm_build(hsm, path, /*from=*/hsm->temp, /*until=*/dst);
        hsm_enter(hsm, &(*path)[0], len);
        hsm->state = hsm->temp = dst = (*path)[0];
    }
    hsm->state = hsm->temp = dst;
}

void hsm_dispatch(struct hsm *hsm, const struct event *event) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(hsm->state == hsm->temp);
    ASSERT(event);

    hsm_state_fn src = NULL;
    enum hsm_rc rc = HSM_STATE_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or triggered transition or ignored
     */
    do {
        src = hsm->temp;
        hsm->temp = hsm->state;
        rc = src(hsm, event);
    } while (HSM_STATE_SUPER == rc);

    if (rc != HSM_STATE_TRAN) { /* event is handled or ignored */
        return;
    }

    /* the event triggered transition */

    hsm_state_fn dst = hsm->temp;
    hsm->temp = hsm->state;

    if (hsm->state != src) {
        hsm_exit(hsm, /*until=*/src);
    }

    hsm_state_fn path[HSM_HIERARCHY_DEPTH_MAX];

    if (src == dst) { /* transition to itself */
        path[0] = dst;
        hsm->state = hsm->temp = src;
        rc = src(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        ASSERT((HSM_STATE_SUPER == rc) || (HSM_STATE_HANDLED == rc));
        hsm_enter_and_init(hsm, &path, /*len=*/1, dst);
        return;
    }

    int len = hsm_build(hsm, &path, /*from=*/dst, /*until=*/hsm_top);

    /*
     * Exit states from src till hsm_top() and search LCA along the way.
     * Once LCA is found, do not exit it. Enter all LCA substates down to dst.
     * If dst requests initial transition - enter and init the dst substates.
     */
    hsm->temp = hsm->state = src;
    while (hsm->temp != hsm_top) {
        for (int i = len - 1; i >= 0; --i) {
            if (path[i] == hsm->temp) { /* LCA is other than hsm_top() */
                hsm_enter_and_init(hsm, &path, /*len=*/i, dst);
                return;
            }
        }
        hsm->state = hsm->temp;
        rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        if (HSM_STATE_HANDLED == rc) {
            rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
            ASSERT(HSM_STATE_SUPER == rc);
            continue;
        }
        ASSERT(HSM_STATE_SUPER == rc);
    }
    /* LCA is hsm_top() */
    hsm_enter_and_init(hsm, &path, len, dst);
}

bool hsm_is_in(struct hsm *hsm, const struct hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(hsm->temp == hsm->state);
    ASSERT(state);

    struct hsm hsm_ = *hsm;

    while ((hsm->temp != state->fn) && (hsm->temp != hsm_top)) {
        enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        ASSERT(HSM_STATE_SUPER == rc);
    }
    int in = (hsm->temp == state->fn);

    *hsm = hsm_;

    return in;
}

bool hsm_state_is_eq(struct hsm *hsm, const struct hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(state);
    ASSERT(state->fn);
    return (hsm->state == state->fn) && (hsm->istate == state->instance);
}

void hsm_ctor(struct hsm *hsm, const struct hsm_state *state) {
    ASSERT(hsm);
    ASSERT(state);
    ASSERT(state->fn);
    hsm->state = hsm_top;
    hsm->istate = 0;
    hsm->temp = state->fn;
    hsm->itemp = state->instance;
}

void hsm_dtor(struct hsm *hsm) {
    ASSERT(hsm);
    hsm_exit(hsm, /*until=*/HSM_STATE_FN(hsm_top));
    hsm->state = hsm->temp = hsm_top;
}

void hsm_init(struct hsm *hsm, const struct event *init_event) {
    ASSERT(hsm);
    ASSERT(hsm->state == hsm_top); /* was hsm_ctor() called? */
    ASSERT(hsm->temp != NULL);

    hsm->state = hsm->temp;
    enum hsm_rc rc = hsm->temp(hsm, init_event);
    ASSERT(HSM_STATE_TRAN == rc);

    hsm_state_fn dst = hsm->temp;
    hsm_state_fn path[HSM_HIERARCHY_DEPTH_MAX];
    int len = hsm_build(hsm, &path, /*from=*/dst, /*until=*/hsm_top);
    hsm_enter_and_init(hsm, &path, len, dst);
}

enum hsm_rc hsm_top(struct hsm *hsm, const struct event *event) {
    (void)hsm;
    (void)event;
    return HSM_IGNORED();
}
