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
 * Hierarchical State Machine (HSM) API implementation.
 */

#include <stdbool.h>
#include <stddef.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"

/** the maximum depth of HSM hierarchy */
#define HSM_HIERARCHY_DEPTH_MAX 16

struct a1hsm_path {
    a1hsm_state_fn fn[HSM_HIERARCHY_DEPTH_MAX];
    unsigned char instance[HSM_HIERARCHY_DEPTH_MAX];
    int len;
};

/** canned events */
static const struct event m_hsm_evt[] = {
    {.id = A1HSM_EVT_EMPTY},
    {.id = A1HSM_EVT_INIT},
    {.id = A1HSM_EVT_ENTRY},
    {.id = A1HSM_EVT_EXIT}
};

static void a1hsm_set_current(struct a1hsm *hsm, const struct a1hsm_state *s) {
    hsm->state = hsm->temp = s->fn;
    hsm->istate = hsm->itemp = s->instance;
}

static bool a1hsm_temp_is_eq(struct a1hsm *hsm, const struct a1hsm_state *s) {
    return (hsm->temp == s->fn) && (hsm->itemp == s->instance);
}

bool a1hsm_state_is_eq(struct a1hsm *hsm, const struct a1hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(state);
    ASSERT(state->fn);
    return (hsm->state == state->fn) && (hsm->istate == state->instance);
}

int a1hsm_get_state_instance(const struct a1hsm *hsm) {
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
static void a1hsm_build(
    struct a1hsm *hsm,
    struct a1hsm_path *path,
    const struct a1hsm_state *from,
    const struct a1hsm_state *until
) {
    struct a1hsm hsm_ = *hsm;
    a1hsm_set_current(hsm, from);
    path->fn[0] = from->fn;
    path->instance[0] = from->instance;
    path->len = 1;
    enum a1hsmrc rc = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EMPTY]);
    ASSERT(A1HSM_RC_SUPER == rc);
    while (!a1hsm_temp_is_eq(hsm, until)) {
        ASSERT(path->len < HSM_HIERARCHY_DEPTH_MAX);
        path->fn[path->len] = hsm->temp;
        path->instance[path->len] = hsm->itemp;
        path->len++;
        rc = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EMPTY]);
        ASSERT(A1HSM_RC_SUPER == rc);
    }
    *hsm = hsm_;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 * @param hsm   the path belongs to this HSM
 * @param path  the path to enter
 */
static void a1hsm_enter(struct a1hsm *hsm, const struct a1hsm_path *path) {
    for (int i = path->len - 1; i >= 0; --i) {
        a1hsm_set_current(hsm, &A1HSM_STATE(path->fn[i], path->instance[i]));
        enum a1hsmrc rc = hsm->state(hsm, &m_hsm_evt[A1HSM_EVT_ENTRY]);
        ASSERT((A1HSM_RC_SUPER == rc) || (A1HSM_RC_HANDLED == rc));
    }
}

/**
 * Exit states starting from current one and finishing with substate of #until.
 * @param hsm    exit the states of this HSM
 * @param until  stop the exit when reaching this state without exiting it
 */
static void a1hsm_exit(struct a1hsm *hsm, struct a1hsm_state *until) {
    while (!a1hsm_temp_is_eq(hsm, until)) {
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
        enum a1hsmrc rc = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EXIT]);
        ASSERT(rc != A1HSM_RC_TRAN);
        if (A1HSM_RC_HANDLED == rc) {
            rc = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EMPTY]);
        }
        ASSERT(A1HSM_RC_SUPER == rc);
    }
}

/**
 * Recursively enter and init destination state.
 * 1. enter all states in #path[]
 * 2. init destination state stored in #path->fn[0] and #path->instance[0]
 * 3. if destination state requested the initial transition, then build
 *    new #path[] and go to 1
 * @param hsm   enter and init the states of this HSM
 * @param path  the path to enter
 *              Initially is path from LCA substate to destination state
 *              (both inclusive), then reused.
 */
static void a1hsm_enter_and_init(struct a1hsm *hsm, struct a1hsm_path *path) {
    a1hsm_enter(hsm, path);
    a1hsm_set_current(hsm, &A1HSM_STATE(path->fn[0], path->instance[0]));
    while (hsm->state(hsm, &m_hsm_evt[A1HSM_EVT_INIT]) == A1HSM_RC_TRAN) {
        struct a1hsm_state from = A1HSM_STATE(hsm->temp, hsm->itemp);
        struct a1hsm_state until = A1HSM_STATE(path->fn[0], path->instance[0]);
        a1hsm_build(hsm, path, &from, &until);
        a1hsm_enter(hsm, path);
        a1hsm_set_current(hsm, &A1HSM_STATE(path->fn[0], path->instance[0]));
    }
    a1hsm_set_current(hsm, &A1HSM_STATE(path->fn[0], path->instance[0]));
}

static enum a1hsmrc a1_dispatch_(struct a1hsm *hsm, const struct event *evt) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(hsm->state == hsm->temp);
    ASSERT(hsm->istate == hsm->itemp);
    ASSERT(evt);

    struct a1hsm_state src = {.fn = NULL, .instance = 0};
    enum a1hsmrc rc = A1HSM_RC_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or ignored or triggers transition
     */
    do {
        src.fn = hsm->temp;
        src.instance = hsm->itemp;
        rc = hsm->temp(hsm, evt);
    } while (A1HSM_RC_SUPER == rc);

    {
        bool tran = (A1HSM_RC_TRAN == rc) || (A1HSM_RC_TRAN_REDISPATCH == rc);
        if (!tran) { /* event was handled or ignored */
            hsm->temp = hsm->state;
            hsm->itemp = hsm->istate;
            return rc;
        }
    }

    /* the event triggered state transition */

    struct a1hsm_state dst = {.fn = hsm->temp, .instance = hsm->itemp};
    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    if (!a1hsm_state_is_eq(hsm, &src)) {
        a1hsm_exit(hsm, /*until=*/&src);
        a1hsm_set_current(hsm, &src);
    }

    struct a1hsm_path path;

    if ((src.fn == dst.fn) && (src.instance == dst.instance)) {
        /* transition to itself */
        path.fn[0] = dst.fn;
        path.instance[0] = dst.instance;
        path.len = 1;
        enum a1hsmrc r = src.fn(hsm, &m_hsm_evt[A1HSM_EVT_EXIT]);
        ASSERT((A1HSM_RC_SUPER == r) || (A1HSM_RC_HANDLED == r));
        a1hsm_enter_and_init(hsm, &path);
        return rc;
    }

    a1hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&A1HSM_STATE(a1hsm_top));

    /*
     * Exit states from src till a1hsm_top() and search LCA along the way.
     * Once LCA is found, do not exit it. Enter all LCA substates down to dst.
     * If dst requests initial transition - enter and init the dst substates.
     */
    while (hsm->temp != a1hsm_top) {
        for (int i = path.len - 1; i >= 0; --i) {
            if ((path.fn[i] == hsm->temp) && (path.instance[i] == hsm->itemp)) {
                /* LCA is other than a1hsm_top() */
                path.len = i;
                a1hsm_enter_and_init(hsm, &path);
                return rc;
            }
        }
        enum a1hsmrc r = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EXIT]);
        if (A1HSM_RC_HANDLED == r) {
            r = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EMPTY]);
        }
        ASSERT(A1HSM_RC_SUPER == r);
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
    }
    /* LCA is a1hsm_top() */
    a1hsm_enter_and_init(hsm, &path);

    return rc;
}

void a1hsm_dispatch(struct a1hsm *hsm, const struct event *evt) {
    enum a1hsmrc rc = a1_dispatch_(hsm, evt);
    if (A1HSM_RC_TRAN_REDISPATCH == rc) {
        rc = a1_dispatch_(hsm, evt);
        ASSERT(A1HSM_RC_TRAN_REDISPATCH != rc);
    }
}

bool a1hsm_is_in(struct a1hsm *hsm, const struct a1hsm_state *state) {
    ASSERT(hsm);
    ASSERT(hsm->state);
    ASSERT(state);

    struct a1hsm hsm_ = *hsm;

    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    while (!a1hsm_temp_is_eq(hsm, state) && (hsm->temp != a1hsm_top)) {
        enum a1hsmrc rc = hsm->temp(hsm, &m_hsm_evt[A1HSM_EVT_EMPTY]);
        ASSERT(A1HSM_RC_SUPER == rc);
    }
    bool in = a1hsm_temp_is_eq(hsm, state);

    *hsm = hsm_;

    return in;
}

void a1hsm_ctor(struct a1hsm *hsm, const struct a1hsm_state *state) {
    ASSERT(hsm);
    ASSERT(state);
    ASSERT(state->fn);
    hsm->state = a1hsm_top;
    hsm->istate = 0;
    hsm->temp = state->fn;
    hsm->itemp = state->instance;
}

void a1hsm_dtor(struct a1hsm *hsm) {
    ASSERT(hsm);
    a1hsm_exit(hsm, /*until=*/&A1HSM_STATE(a1hsm_top));
    a1hsm_set_current(hsm, &A1HSM_STATE(NULL));
}

void a1hsm_init(struct a1hsm *hsm, const struct event *init_event) {
    ASSERT(hsm);
    ASSERT(hsm->state == a1hsm_top); /* was a1hsm_ctor() called? */
    ASSERT(hsm->istate == 0);
    ASSERT(hsm->temp != NULL);

    hsm->state = hsm->temp;
    hsm->istate = hsm->itemp;
    enum a1hsmrc rc = hsm->temp(hsm, init_event);
    ASSERT(A1HSM_RC_TRAN == rc);

    struct a1hsm_state dst = {.fn = hsm->temp, .instance = hsm->itemp};
    struct a1hsm_path path;
    a1hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&A1HSM_STATE(a1hsm_top));
    a1hsm_enter_and_init(hsm, &path);
}

enum a1hsmrc a1hsm_top(struct a1hsm *hsm, const struct event *event) {
    (void)hsm;
    (void)event;
    return A1HSM_RC_HANDLED;
}
