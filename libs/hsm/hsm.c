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
 * Hierarchical State Machine (HSM) API implementation.
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "common/macros.h"
#include "hsm/hsm.h"

/** Hierarchy path */
struct am_hsm_path {
    /**
     * states in superstate - substate relationship
     * state[i] is always a substate of state[i-1]
     */
    struct am_hsm_state state[HSM_HIERARCHY_DEPTH_MAX];
    /** the actual size of \p state */
    int len;
};

/** canned events */
static const struct am_event m_hsm_evt_empty = {.id = AM_HSM_EVT_EMPTY};
static const struct am_event m_hsm_evt_init = {.id = AM_HSM_EVT_INIT};
static const struct am_event m_hsm_evt_entry = {.id = AM_HSM_EVT_ENTRY};
static const struct am_event m_hsm_evt_exit = {.id = AM_HSM_EVT_EXIT};

static void hsm_set_state(struct am_hsm *hsm, struct am_hsm_state s) {
    hsm->state = s;
    hsm->smi = (unsigned char)s.smi;
}

/**
 * Build ancestor chain path.
 *
 * The path starts with #from state and ends with substate of #until state OR
 * with state #till, if it is provided.
 *
 * @param hsm    HSM handler
 * @param path   the path is placed here
 * @param from   placed to path[0]
 * @param until  the substate of it is placed at the end of path[].
 * @param till   the state if found is placed at the end of path[]. Can be NULL.
 */
static void hsm_build(
    struct am_hsm *hsm,
    struct am_hsm_path *path,
    const struct am_hsm_state *from,
    const struct am_hsm_state *until,
    const struct am_hsm_state *till
) {
    path->state[0] = *from;
    path->len = 1;
    if (till && (till->fn == from->fn) && (till->smi == from->smi)) {
        return;
    }
    struct am_hsm hsm_ = *hsm;
    hsm_set_state(hsm, *from);
    enum am_hsm_rc rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
    AM_ASSERT(AM_HSM_RC_SUPER == rc);
    while (!am_hsm_state_is_eq(hsm, *until)) {
        AM_ASSERT(path->len < AM_COUNTOF(path->state));
        path->state[path->len] = hsm->state;
        ++path->len;
        if (till && am_hsm_state_is_eq(hsm, *till)) {
            break;
        }
        rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_HSM_RC_SUPER == rc);
    }
    *hsm = hsm_;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 *
 * @param hsm   the path belongs to this HSM
 * @param path  the path to enter
 */
static void hsm_enter(struct am_hsm *hsm, const struct am_hsm_path *path) {
    for (int i = path->len; i > 0; --i) {
        hsm_set_state(hsm, path->state[i - 1]);
        enum am_hsm_rc rc = hsm->state.fn(hsm, &m_hsm_evt_entry);
        AM_ASSERT((AM_HSM_RC_SUPER == rc) || (AM_HSM_RC_HANDLED == rc));
        hsm_set_state(hsm, path->state[i - 1]);
    }
    hsm->hierarchy_level = (unsigned)(hsm->hierarchy_level + path->len) &
                           AM_HSM_HIERARCHY_LEVEL_MASK;
    AM_ASSERT(hsm->hierarchy_level <= AM_COUNTOF(path->state));
}

/**
 * Exit current state.
 *
 * @param hsm    exit the state of this HSM
 */
static void hsm_exit_state(struct am_hsm *hsm) {
    enum am_hsm_rc rc = hsm->state.fn(hsm, &m_hsm_evt_exit);
    AM_ASSERT(rc != AM_HSM_RC_TRAN);
    if (AM_HSM_RC_HANDLED == rc) {
        rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
    }
    AM_ASSERT(AM_HSM_RC_SUPER == rc);
    AM_ASSERT(hsm->hierarchy_level > 0);
    --hsm->hierarchy_level;
}

/**
 * Exit states.
 *
 * Start with current and end with immediate substate of #until.
 *
 * @param hsm    exit the states of this HSM
 * @param until  stop the exit when reaching this state without exiting it
 */
static void hsm_exit(struct am_hsm *hsm, struct am_hsm_state until) {
    while (!am_hsm_state_is_eq(hsm, until)) {
        hsm_exit_state(hsm);
    }
}

/**
 * Recursively enter and init destination state.
 *
 * 1. enter all states in #path
 * 2. init destination state stored in #path[0]
 * 3. if destination state requested an initial transition, then build
 *    new #path and go to step 1
 *
 * @param hsm   enter and init the states of this HSM
 * @param path  the path to enter
 *              Initially it is path from least common ancestor (LCA) substate
 *              to destination state (both inclusive), then reused.
 */
static void hsm_enter_and_init(struct am_hsm *hsm, struct am_hsm_path *path) {
    hsm_enter(hsm, path);
    hsm_set_state(hsm, path->state[0]);
    enum am_hsm_rc rc;
    while ((rc = hsm->state.fn(hsm, &m_hsm_evt_init)) == AM_HSM_RC_TRAN) {
        struct am_hsm_state until = path->state[0];
        hsm_build(hsm, path, /*from=*/&hsm->state, &until, /*till=*/NULL);
        hsm_enter(hsm, path);
        hsm_set_state(hsm, path->state[0]);
    }
    AM_ASSERT(rc != AM_HSM_RC_TRAN_REDISPATCH);
    hsm_set_state(hsm, path->state[0]);
}

/**
 * Transition from source to destination state.
 *
 * @param hsm  transition the state of this HSM
 * @param src  the source state
 * @param dst  the destination state
 */
static void hsm_transition(
    struct am_hsm *hsm, struct am_hsm_state src, struct am_hsm_state dst
) {
    if (!am_hsm_state_is_eq(hsm, src)) {
        hsm_exit(hsm, /*until=*/src);
        hsm_set_state(hsm, src);
    }

    struct am_hsm_path path;

    if ((src.fn == dst.fn) && (src.smi == dst.smi)) {
        /* transition to itself */
        path.state[0] = dst;
        path.len = 1;
        hsm_exit_state(hsm);
        hsm_enter_and_init(hsm, &path);
        return;
    }

    struct am_hsm_state until = AM_HSM_STATE_CTOR(am_hsm_top);
    hsm_build(hsm, &path, /*from=*/&dst, &until, /*till=*/&src);
    const struct am_hsm_state *end = &path.state[path.len - 1];
    if ((end->fn == src.fn) && (end->smi == src.smi)) {
        /* src is LCA */
        --path.len;
        hsm_enter_and_init(hsm, &path);
        return;
    }

    /*
     * Exit states from src till am_hsm_top() and search LCA along the way.
     * Once LCA is found, do not exit it. Enter all LCA substates down to dst.
     * If dst requests initial transition - enter and init the dst substates.
     */
    while (hsm->state.fn != am_hsm_top) {
        if (hsm->hierarchy_level <= path.len) {
            /*
             * path has higher hierarchy level states
             * at lower indices (reversed order)
             */
            int i = path.len - hsm->hierarchy_level;
            if (am_hsm_state_is_eq(hsm, path.state[i])) {
                /* LCA is found and it is not am_hsm_top() */
                path.len = i;
                hsm_enter_and_init(hsm, &path);
                return;
            }
        }
        hsm_exit_state(hsm);
    }
    /* LCA is am_hsm_top() */
    hsm_enter_and_init(hsm, &path);
}

static enum am_hsm_rc hsm_dispatch(
    struct am_hsm *hsm, const struct am_event *event
) {
    struct am_hsm_state src = {.fn = NULL, .smi = 0};
    struct am_hsm_state state = hsm->state;
    enum am_hsm_rc rc = AM_HSM_RC_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or ignored or triggers transition
     */
    int cnt = HSM_HIERARCHY_DEPTH_MAX;
    do {
        src = hsm->state;
        hsm->state = state;
        /* preserve hsm->smi as the submachine instance of src.fn */
        rc = src.fn(hsm, event);
        --cnt;
        AM_ASSERT(cnt); /* HSM hierarchy depth exceeds HSM_HIERARCHY_DEPTH_MAX*/
    } while (AM_HSM_RC_SUPER == rc);

    bool tran = (AM_HSM_RC_TRAN == rc) || (AM_HSM_RC_TRAN_REDISPATCH == rc);
    if (!tran) { /* event was handled or ignored */
        hsm_set_state(hsm, state);
        return rc;
    }

    /* the event triggered state transition */

    struct am_hsm_state dst = hsm->state;
    AM_ASSERT(dst.fn != am_hsm_top); /* transition to am_hsm_top() is invalid */
    hsm_set_state(hsm, state);

    hsm_transition(hsm, src, dst);

    return rc;
}

void am_hsm_dispatch(struct am_hsm *hsm, const struct am_event *event) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state.fn);
    AM_ASSERT(hsm->init_called);
    AM_ASSERT(!hsm->dispatch_in_progress);
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    hsm->dispatch_in_progress = true;

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

    hsm->dispatch_in_progress = false;
}

bool am_hsm_is_in(struct am_hsm *hsm, struct am_hsm_state state) {
    AM_ASSERT(hsm);

    if (NULL == state.fn) {
        return NULL == hsm->state.fn;
    }

    struct am_hsm hsm_ = *hsm;

    while (!am_hsm_state_is_eq(hsm, state) && (hsm->state.fn != am_hsm_top)) {
        struct am_hsm_state s = hsm->state;
        *hsm = hsm_;
        enum am_hsm_rc rc = s.fn(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_HSM_RC_SUPER == rc);
    }
    bool in = am_hsm_state_is_eq(hsm, state);

    *hsm = hsm_;

    return in;
}

bool am_hsm_state_is_eq(const struct am_hsm *hsm, struct am_hsm_state state) {
    AM_ASSERT(hsm);
    if (NULL == state.fn) {
        return NULL == hsm->state.fn;
    }
    return (hsm->state.fn == state.fn) && (hsm->state.smi == state.smi);
}

int am_hsm_instance(const struct am_hsm *hsm) {
    AM_ASSERT(hsm);
    return (int)hsm->smi;
}

struct am_hsm_state am_hsm_state(const struct am_hsm *hsm) {
    AM_ASSERT(hsm);
    return hsm->state;
}

void am_hsm_ctor(struct am_hsm *hsm, struct am_hsm_state state) {
    AM_ASSERT(hsm);
    AM_ASSERT(state.fn);
    memset(hsm, 0, sizeof(*hsm));
    hsm_set_state(hsm, state);
    hsm->ctor_called = true;
}

void am_hsm_dtor(struct am_hsm *hsm) {
    AM_ASSERT(hsm);
    hsm_exit(hsm, /*until=*/AM_HSM_STATE_CTOR(am_hsm_top));
    hsm_set_state(hsm, AM_HSM_STATE_CTOR(NULL));
    hsm->ctor_called = hsm->init_called = false;
}

void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->ctor_called); /* was am_hsm_ctor() called? */

    struct am_hsm_state state = hsm->state;
    enum am_hsm_rc rc = hsm->state.fn(hsm, init_event);
    AM_ASSERT(AM_HSM_RC_TRAN == rc);

    struct am_hsm_state dst = hsm->state;
    struct am_hsm_path path;
    struct am_hsm_state until = AM_HSM_STATE_CTOR(am_hsm_top);
    hsm_set_state(hsm, state);
    hsm_build(hsm, &path, /*from=*/&dst, &until, /*till=*/NULL);
    hsm_enter_and_init(hsm, &path);
    hsm->init_called = true;
}

#ifdef AM_HSM_SPY
void am_hsm_set_spy(struct am_hsm *hsm, am_hsm_spy_fn spy) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->state.fn); /* was am_hsm_ctor() called? */
    hsm->spy = spy;
}
#endif

enum am_hsm_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event) {
    (void)hsm;
    (void)event;
    return AM_HSM_RC_HANDLED;
}
