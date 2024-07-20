/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2024 Adel Mamin
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

struct hsm_path {
    hsm_state_fn fn[HSM_HIERARCHY_DEPTH_MAX];
    unsigned char instance[HSM_HIERARCHY_DEPTH_MAX];
    int len;
};

/** Canned events */
static const struct event m_hsm_evt[] = {
    {.id = HSM_EVT_EMPTY},
    {.id = HSM_EVT_INIT},
    {.id = HSM_EVT_ENTRY},
    {.id = HSM_EVT_EXIT}
};

static void hsm_set_current(struct hsm *hsm, const struct hsm_state *state) {
    hsm->state = hsm->temp = state->fn;
    hsm->istate = hsm->itemp = state->instance;
}

static bool hsm_temp_is_eq(struct hsm *hsm, const struct hsm_state *state) {
    return (hsm->temp == state->fn) && (hsm->itemp == state->instance);
}

bool hsm_state_is_eq(struct hsm *hsm, const struct hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(state);
    ASSERT(state->fn);
    return (hsm->state == state->fn) && (hsm->istate == state->instance);
}

int hsm_get_state_instance(const struct hsm *hsm) {
    ASSERT(hsm);
    return hsm->itemp;
}

/**
 * Build ancestor chain path.
 * The path starts with 'from' state and ends with substate of #until state.
 * @param hsm    HSM handler
 * @param path   the path is placed here
 * @param from   placed to path[0]
 * @param until  the substate of it is placed to the end of path[]
 */
static void hsm_build(
    struct hsm *hsm,
    struct hsm_path *path,
    const struct hsm_state *from,
    const struct hsm_state *until
) {
    struct hsm hsm_ = *hsm;
    hsm_set_current(hsm, from);
    path->fn[0] = from->fn;
    path->instance[0] = from->instance;
    path->len = 1;
    enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
    ASSERT(HSM_STATE_SUPER == rc);
    while (!hsm_temp_is_eq(hsm, until)) {
        ASSERT(path->len < HSM_HIERARCHY_DEPTH_MAX);
        path->fn[path->len] = hsm->temp;
        path->instance[path->len] = hsm->itemp;
        path->len++;
        rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        ASSERT(HSM_STATE_SUPER == rc);
    }
    *hsm = hsm_;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 * @param hsm   the path belongs to this HSM
 * @param path  the path to enter
 */
static void hsm_enter(struct hsm *hsm, const struct hsm_path *path) {
    for (int i = path->len - 1; i >= 0; --i) {
        hsm_set_current(hsm, &HSM_STATE(path->fn[i], path->instance[i]));
        enum hsm_rc rc = hsm->state(hsm, &m_hsm_evt[HSM_EVT_ENTRY]);
        ASSERT((HSM_STATE_SUPER == rc) || (HSM_STATE_HANDLED == rc));
    }
}

/**
 * Exit states starting from current one and finishing with substate of #until.
 * @param hsm    exit the states of this HSM
 * @param until  stop the exit when reaching this state without exiting it
 */
static void hsm_exit(struct hsm *hsm, struct hsm_state *until) {
    while (!hsm_temp_is_eq(hsm, until)) {
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
        enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        ASSERT(rc != HSM_STATE_TRAN);
        if (HSM_STATE_HANDLED == rc) {
            rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        }
        ASSERT(HSM_STATE_SUPER == rc);
    }
}

/**
 * Recursively enter and init destination state.
 * 1. enter all states in #path[]
 * 2. init destination state stored in #path->fn[0] and #path->instance[0]
 * 3. if destination state requested the initial transition, then build
 *    new #path[] and go to 1.
 * @param hsm   enter and init the states of this HSM
 * @param path  the path to enter.
 *              Initially is path from LCA substate to destination state
 *              (both inclusive), then reused.
 */
static void hsm_enter_and_init(struct hsm *hsm, struct hsm_path *path) {
    hsm_enter(hsm, path);
    hsm_set_current(hsm, &HSM_STATE(path->fn[0], path->instance[0]));
    while (hsm->state(hsm, &m_hsm_evt[HSM_EVT_INIT]) == HSM_STATE_TRAN) {
        struct hsm_state from = HSM_STATE(hsm->temp, hsm->itemp);
        struct hsm_state until = HSM_STATE(path->fn[0], path->instance[0]);
        hsm_build(hsm, path, &from, &until);
        hsm_enter(hsm, path);
        hsm_set_current(hsm, &HSM_STATE(path->fn[0], path->instance[0]));
    }
    hsm_set_current(hsm, &HSM_STATE(path->fn[0], path->instance[0]));
}

static enum hsm_rc hsm_dispatch_(struct hsm *hsm, const struct event *event) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(hsm->state == hsm->temp);
    ASSERT(hsm->istate == hsm->itemp);
    ASSERT(event);

    struct hsm_state src = {.fn = NULL, .instance = 0};
    enum hsm_rc rc = HSM_STATE_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or ignored or triggers transition
     */
    do {
        src.fn = hsm->temp;
        src.instance = hsm->itemp;
        rc = hsm->temp(hsm, event);
    } while (HSM_STATE_SUPER == rc);

    {
        int tran = (HSM_STATE_TRAN == rc) || (HSM_STATE_TRAN_REDISPATCH == rc);
        if (!tran) { /* event is handled or ignored */
            hsm->temp = hsm->state;
            hsm->itemp = hsm->istate;
            return rc;
        }
    }

    /* the event triggered state transition */

    struct hsm_state dst = {.fn = hsm->temp, .instance = hsm->itemp};
    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    if (!hsm_state_is_eq(hsm, &src)) {
        hsm_exit(hsm, /*until=*/&src);
        hsm_set_current(hsm, &src);
    }

    struct hsm_path path;

    if ((src.fn == dst.fn) && (src.instance == dst.instance)) {
        /* transition to itself */
        path.fn[0] = dst.fn;
        path.instance[0] = dst.instance;
        path.len = 1;
        enum hsm_rc r = src.fn(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        ASSERT((HSM_STATE_SUPER == r) || (HSM_STATE_HANDLED == r));
        hsm_enter_and_init(hsm, &path);
        return rc;
    }

    hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&HSM_STATE(hsm_top));

    /*
     * Exit states from src till hsm_top() and search LCA along the way.
     * Once LCA is found, do not exit it. Enter all LCA substates down to dst.
     * If dst requests initial transition - enter and init the dst substates.
     */
    while (hsm->temp != hsm_top) {
        for (int i = path.len - 1; i >= 0; --i) {
            if ((path.fn[i] == hsm->temp) && (path.instance[i] == hsm->itemp)) {
                /* LCA is other than hsm_top() */
                path.len = i;
                hsm_enter_and_init(hsm, &path);
                return rc;
            }
        }
        enum hsm_rc r = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EXIT]);
        if (HSM_STATE_HANDLED == r) {
            r = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        }
        ASSERT(HSM_STATE_SUPER == r);
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
    }
    /* LCA is hsm_top() */
    hsm_enter_and_init(hsm, &path);

    return rc;
}

void hsm_dispatch(struct hsm *hsm, const struct event *event) {
    enum hsm_rc rc = hsm_dispatch_(hsm, event);
    if (HSM_STATE_TRAN_REDISPATCH == rc) {
        rc = hsm_dispatch_(hsm, event);
        ASSERT(HSM_STATE_TRAN_REDISPATCH != rc);
    }
}

bool hsm_is_in(struct hsm *hsm, const struct hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(state);

    struct hsm hsm_ = *hsm;

    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    while (!hsm_temp_is_eq(hsm, state) && (hsm->temp != hsm_top)) {
        enum hsm_rc rc = hsm->temp(hsm, &m_hsm_evt[HSM_EVT_EMPTY]);
        ASSERT(HSM_STATE_SUPER == rc);
    }
    bool in = hsm_temp_is_eq(hsm, state);

    *hsm = hsm_;

    return in;
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
    hsm_exit(hsm, /*until=*/&HSM_STATE(hsm_top));
    hsm_set_current(hsm, &HSM_STATE(NULL));
}

void hsm_init(struct hsm *hsm, const struct event *init_event) {
    ASSERT(hsm);
    ASSERT(hsm->state == hsm_top); /* was hsm_ctor() called? */
    ASSERT(hsm->istate == 0);
    ASSERT(hsm->temp != NULL);

    hsm->state = hsm->temp;
    hsm->istate = hsm->itemp;
    enum hsm_rc rc = hsm->temp(hsm, init_event);
    ASSERT(HSM_STATE_TRAN == rc);

    struct hsm_state dst = {.fn = hsm->temp, .instance = hsm->itemp};
    struct hsm_path path;
    hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&HSM_STATE(hsm_top));
    hsm_enter_and_init(hsm, &path);
}

enum hsm_rc hsm_top(struct hsm *hsm, const struct event *event) {
    (void)hsm;
    (void)event;
    return HSM_IGNORED();
}
