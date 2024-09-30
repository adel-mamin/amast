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

struct am_hsm_path {
    am_hsm_state_fn fn[HSM_HIERARCHY_DEPTH_MAX];
    unsigned char ifn[HSM_HIERARCHY_DEPTH_MAX];
    int len;
};

/** canned events */
static const struct am_event m_hsm_evt_empty = {.id = AM_HSM_EVT_EMPTY};
static const struct am_event m_hsm_evt_init = {.id = AM_HSM_EVT_INIT};
static const struct am_event m_hsm_evt_entry = {.id = AM_HSM_EVT_ENTRY};
static const struct am_event m_hsm_evt_exit = {.id = AM_HSM_EVT_EXIT};

static void hsm_set_current(struct am_hsm *hsm, const struct am_hsm_state *s) {
    hsm->state = hsm->temp = s->fn;
    hsm->istate = hsm->itemp = s->ifn;
}

static bool hsm_temp_is_eq(struct am_hsm *hsm, const struct am_hsm_state *s) {
    return (hsm->temp == s->fn) && (hsm->itemp == s->ifn);
}

bool am_hsm_state_is_eq(struct am_hsm *hsm, const struct am_hsm_state *state) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state);
    AM_ASSERT(state);
    AM_ASSERT(state->fn);
    return (hsm->state == state->fn) && (hsm->istate == state->ifn);
}

int am_hsm_get_state_instance(const struct am_hsm *hsm) {
    AM_ASSERT(hsm);
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
    struct am_hsm *hsm,
    struct am_hsm_path *path,
    const struct am_hsm_state *from,
    const struct am_hsm_state *until
) {
    struct am_hsm hsm_ = *hsm;
    hsm_set_current(hsm, from);
    path->fn[0] = from->fn;
    path->ifn[0] = from->ifn;
    path->len = 1;
    enum am_hsm_rc rc = hsm->temp(hsm, &m_hsm_evt_empty);
    AM_ASSERT(AM_HSM_RC_SUPER == rc);
    while (!hsm_temp_is_eq(hsm, until)) {
        AM_ASSERT(path->len < HSM_HIERARCHY_DEPTH_MAX);
        path->fn[path->len] = hsm->temp;
        path->ifn[path->len] = hsm->itemp;
        path->len++;
        rc = hsm->temp(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_HSM_RC_SUPER == rc);
    }
    *hsm = hsm_;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 * @param hsm   the path belongs to this HSM
 * @param path  the path to enter
 */
static void hsm_enter(struct am_hsm *hsm, const struct am_hsm_path *path) {
    for (int i = path->len - 1; i >= 0; --i) {
        hsm_set_current(hsm, &AM_HSM_STATE(path->fn[i], path->ifn[i]));
        enum am_hsm_rc rc = hsm->state(hsm, &m_hsm_evt_entry);
        AM_ASSERT((AM_HSM_RC_SUPER == rc) || (AM_HSM_RC_HANDLED == rc));
    }
}

/**
 * Exit states starting from current one and finishing with substate of #until.
 * @param hsm    exit the states of this HSM
 * @param until  stop the exit when reaching this state without exiting it
 */
static void hsm_exit(struct am_hsm *hsm, struct am_hsm_state *until) {
    while (!hsm_temp_is_eq(hsm, until)) {
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
        enum am_hsm_rc rc = hsm->temp(hsm, &m_hsm_evt_exit);
        AM_ASSERT(rc != AM_HSM_RC_TRAN);
        if (AM_HSM_RC_HANDLED == rc) {
            rc = hsm->temp(hsm, &m_hsm_evt_empty);
        }
        AM_ASSERT(AM_HSM_RC_SUPER == rc);
    }
}

/**
 * Recursively enter and init destination state.
 * 1. enter all states in #path[]
 * 2. init destination state stored in #path->fn[0] and #path->ifn[0]
 * 3. if destination state requested an initial transition, then build
 *    new #path[] and go to step 1
 * @param hsm   enter and init the states of this HSM
 * @param path  the path to enter
 *              Initially it is path from LCA substate to destination state
 *              (both inclusive), then reused.
 */
static void hsm_enter_and_init(struct am_hsm *hsm, struct am_hsm_path *path) {
    hsm_enter(hsm, path);
    hsm_set_current(hsm, &AM_HSM_STATE(path->fn[0], path->ifn[0]));
    while (hsm->state(hsm, &m_hsm_evt_init) == AM_HSM_RC_TRAN) {
        struct am_hsm_state from = AM_HSM_STATE(hsm->temp, hsm->itemp);
        struct am_hsm_state until = AM_HSM_STATE(path->fn[0], path->ifn[0]);
        hsm_build(hsm, path, &from, &until);
        hsm_enter(hsm, path);
        hsm_set_current(hsm, &AM_HSM_STATE(path->fn[0], path->ifn[0]));
    }
    hsm_set_current(hsm, &AM_HSM_STATE(path->fn[0], path->ifn[0]));
}

static enum am_hsm_rc hsm_dispatch(
    struct am_hsm *hsm, const struct am_event *event
) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state);
    AM_ASSERT(hsm->state == hsm->temp);
    AM_ASSERT(hsm->istate == hsm->itemp);
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    struct am_hsm_state src = {.fn = NULL, .ifn = 0};
    enum am_hsm_rc rc = AM_HSM_RC_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or ignored or triggers transition
     */
    do {
        src.fn = hsm->temp;
        src.ifn = hsm->itemp;
        rc = hsm->temp(hsm, event);
    } while (AM_HSM_RC_SUPER == rc);

    {
        bool tran = (AM_HSM_RC_TRAN == rc) || (AM_HSM_RC_TRAN_REDISPATCH == rc);
        if (!tran) { /* event was handled or ignored */
            hsm->temp = hsm->state;
            hsm->itemp = hsm->istate;
            return rc;
        }
    }

    /* the event triggered state transition */

    struct am_hsm_state dst = {.fn = hsm->temp, .ifn = hsm->itemp};
    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    if (!am_hsm_state_is_eq(hsm, &src)) {
        hsm_exit(hsm, /*until=*/&src);
        hsm_set_current(hsm, &src);
    }

    struct am_hsm_path path;

    if ((src.fn == dst.fn) && (src.ifn == dst.ifn)) {
        /* transition to itself */
        path.fn[0] = dst.fn;
        path.ifn[0] = dst.ifn;
        path.len = 1;
        enum am_hsm_rc r = src.fn(hsm, &m_hsm_evt_exit);
        AM_ASSERT((AM_HSM_RC_SUPER == r) || (AM_HSM_RC_HANDLED == r));
        hsm_enter_and_init(hsm, &path);
        return rc;
    }

    hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&AM_HSM_STATE(am_hsm_top));

    /*
     * Exit states from src till am_hsm_top() and search LCA along the way.
     * Once LCA is found, do not exit it. Enter all LCA substates down to dst.
     * If dst requests initial transition - enter and init the dst substates.
     */
    while (hsm->temp != am_hsm_top) {
        for (int i = path.len - 1; i >= 0; --i) {
            if ((path.fn[i] == hsm->temp) && (path.ifn[i] == hsm->itemp)) {
                /* LCA is other than am_hsm_top() */
                path.len = i;
                hsm_enter_and_init(hsm, &path);
                return rc;
            }
        }
        enum am_hsm_rc r = hsm->temp(hsm, &m_hsm_evt_exit);
        if (AM_HSM_RC_HANDLED == r) {
            r = hsm->temp(hsm, &m_hsm_evt_empty);
        }
        AM_ASSERT(AM_HSM_RC_SUPER == r);
        hsm->state = hsm->temp;
        hsm->istate = hsm->itemp;
    }
    /* LCA is am_hsm_top() */
    hsm_enter_and_init(hsm, &path);

    return rc;
}

void am_hsm_dispatch(struct am_hsm *hsm, const struct am_event *event) {
#ifdef AM_HSM_SPY
    if (hsm->spy) {
        hsm->spy(hsm, event);
    }
#endif
    enum am_hsm_rc rc = hsm_dispatch(hsm, event);
    if (AM_HSM_RC_TRAN_REDISPATCH == rc) {
        rc = hsm_dispatch(hsm, event);
        AM_ASSERT(AM_HSM_RC_TRAN_REDISPATCH != rc);
    }
}

bool am_hsm_is_in(struct am_hsm *hsm, const struct am_hsm_state *state) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state);
    AM_ASSERT(state);

    struct am_hsm hsm_ = *hsm;

    hsm->temp = hsm->state;
    hsm->itemp = hsm->istate;

    while (!hsm_temp_is_eq(hsm, state) && (hsm->temp != am_hsm_top)) {
        enum am_hsm_rc rc = hsm->temp(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_HSM_RC_SUPER == rc);
    }
    bool in = hsm_temp_is_eq(hsm, state);

    *hsm = hsm_;

    return in;
}

void am_hsm_ctor(struct am_hsm *hsm, const struct am_hsm_state *state) {
    AM_ASSERT(hsm);
    AM_ASSERT(state);
    AM_ASSERT(state->fn);
    hsm->state = am_hsm_top;
    hsm->istate = 0;
    hsm->temp = state->fn;
    hsm->itemp = state->ifn;
}

void am_hsm_dtor(struct am_hsm *hsm) {
    AM_ASSERT(hsm);
    hsm_exit(hsm, /*until=*/&AM_HSM_STATE(am_hsm_top));
    hsm_set_current(hsm, &AM_HSM_STATE(NULL));
}

void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state == am_hsm_top); /* was am_hsm_ctor() called? */
    AM_ASSERT(hsm->istate == 0);
    AM_ASSERT(hsm->temp != NULL);

    hsm->state = hsm->temp;
    hsm->istate = hsm->itemp;
    enum am_hsm_rc rc = hsm->temp(hsm, init_event);
    AM_ASSERT(AM_HSM_RC_TRAN == rc);

    struct am_hsm_state dst = {.fn = hsm->temp, .ifn = hsm->itemp};
    struct am_hsm_path path;
    hsm_build(hsm, &path, /*from=*/&dst, /*until=*/&AM_HSM_STATE(am_hsm_top));
    hsm_enter_and_init(hsm, &path);
}

#ifdef AM_HSM_SPY
void am_hsm_set_spy(struct am_hsm *hsm, am_hsm_spy_fn spy) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state == am_hsm_top); /* was am_hsm_ctor() called? */
    hsm->spy = spy;
}
#endif

enum am_hsm_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event) {
    (void)hsm;
    (void)event;
    return AM_HSM_RC_HANDLED;
}
