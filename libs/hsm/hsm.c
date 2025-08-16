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
 * Hierarchical State Machine (HSM) API implementation.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"

/** Hierarchy path */
struct am_hsm_path {
    /**
     * states in superstate - substate relationship
     * state[i] is always a substate of state[i-1]
     */
    struct am_hsm_state state[AM_HSM_HIERARCHY_DEPTH_MAX];
    /** the actual length of \p state */
    int len;
};

/** canned event */
static const struct am_event m_hsm_evt_empty = {.id = AM_EVT_EMPTY};

static void hsm_set_state(struct am_hsm *hsm, struct am_hsm_state s) {
    hsm->state = s;
    hsm->smi = (uint8_t)s.smi;
}

/**
 * Build ancestor chain path.
 *
 * The path starts with \p from state and ends with substate of \p until state
 * OR with state \p till, if it is provided.
 *
 * @param hsm    HSM handler
 * @param path   the path is placed here
 * @param from   placed to path[0]
 * @param until  the substate of it is placed at the end of path[].
 * @param till   the state, if found, is placed at the end of path[].
 *               Can be NULL.
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
    enum am_rc rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
    AM_ASSERT(AM_RC_SUPER == rc);
    while (!am_hsm_state_is_eq(hsm, *until)) {
        AM_ASSERT(path->len < AM_COUNTOF(path->state));
        path->state[path->len] = hsm->state;
        ++path->len;
        if (till && am_hsm_state_is_eq(hsm, *till)) {
            break;
        }
        rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_RC_SUPER == rc);
    }
    *hsm = hsm_;
}

/**
 * Enter all states in the path starting from last and finishing with path[0].
 *
 * @param hsm   HSM handler
 * @param path  the path to enter
 */
static void hsm_enter(struct am_hsm *hsm, const struct am_hsm_path *path) {
    struct am_event entry = {.id = AM_EVT_ENTRY};
    for (int i = path->len; i > 0; --i) {
        hsm_set_state(hsm, path->state[i - 1]);
        enum am_rc rc = hsm->state.fn(hsm, &entry);
        AM_ASSERT((AM_RC_SUPER == rc) || (AM_RC_HANDLED == rc));
        hsm_set_state(hsm, path->state[i - 1]);
    }
    hsm->hierarchy_level = (unsigned)(hsm->hierarchy_level + path->len) &
                           AM_HSM_HIERARCHY_LEVEL_MASK;
    AM_ASSERT(hsm->hierarchy_level <= AM_COUNTOF(path->state));
}

/**
 * Exit current state.
 *
 * @param hsm  HSM handler
 */
static void hsm_exit_state(struct am_hsm *hsm) {
    struct am_event exit = {.id = AM_EVT_EXIT};
    enum am_rc rc = hsm->state.fn(hsm, &exit);
    if (AM_RC_HANDLED == rc) {
        rc = hsm->state.fn(hsm, &m_hsm_evt_empty);
    }
    AM_ASSERT(AM_RC_SUPER == rc);
    AM_ASSERT(hsm->hierarchy_level > 0);
    --hsm->hierarchy_level;
}

/**
 * Exit states.
 *
 * Start with current and end with immediate substate of \p until.
 *
 * @param hsm    HSM handler
 * @param until  stop the exit when reaching this state without exiting it
 */
static void hsm_exit(struct am_hsm *hsm, struct am_hsm_state until) {
    int cnt = AM_HSM_HIERARCHY_DEPTH_MAX;
    while (!am_hsm_state_is_eq(hsm, until)) {
        hsm_exit_state(hsm);
        --cnt;
        /* check if HSM hierarchy depth exceeds #AM_HSM_HIERARCHY_DEPTH_MAX */
        AM_ASSERT(cnt);
    }
}

/**
 * Recursively enter and init destination state.
 *
 * 1. enter all states in \p path
 * 2. init destination state stored in \p path[0]
 * 3. if destination state requested an initial transition, then build
 *    new \p path and go to step 1
 *
 * @param hsm   HSM handler
 * @param path  the path to enter
 *              Initially it is path from least common ancestor (LCA) substate
 *              to destination state (both inclusive), then reused.
 */
static void hsm_enter_and_init(struct am_hsm *hsm, struct am_hsm_path *path) {
    hsm_enter(hsm, path);
    hsm_set_state(hsm, path->state[0]);
    enum am_rc rc;
    struct am_event init = {.id = AM_EVT_INIT};
    while ((rc = hsm->state.fn(hsm, &init)) == AM_RC_TRAN) {
        struct am_hsm_state until = path->state[0];
        hsm_build(hsm, path, /*from=*/&hsm->state, &until, /*till=*/NULL);
        hsm_enter(hsm, path);
        hsm_set_state(hsm, path->state[0]);
    }
    AM_ASSERT((AM_RC_HANDLED == rc) || (AM_RC_SUPER == rc));
    hsm_set_state(hsm, path->state[0]);
}

/**
 * Transition from source to destination state.
 *
 * @param hsm  HSM handler
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
    int cnt = AM_HSM_HIERARCHY_DEPTH_MAX;
    while (hsm->state.fn != am_hsm_top) {
        --cnt;
        /* check if HSM hierarchy depth exceeds #AM_HSM_HIERARCHY_DEPTH_MAX */
        AM_ASSERT(cnt);
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

static enum am_rc hsm_dispatch(
    struct am_hsm *hsm, const struct am_event *event
) {
    struct am_hsm_state src = {.fn = NULL, .smi = 0};
    struct am_hsm_state state = hsm->state;
    enum am_rc rc = AM_RC_HANDLED;
    /*
     * propagate event up the ancestor chain till it is either
     * handled or ignored or triggers transition
     */
    int cnt = AM_HSM_HIERARCHY_DEPTH_MAX;
    do {
        src = hsm->state;
        hsm->state = state;
        /* preserve hsm->smi as the submachine instance of src.fn */
        rc = src.fn(hsm, event);
        --cnt;
        /* check if HSM hierarchy depth exceeds #AM_HSM_HIERARCHY_DEPTH_MAX */
        AM_ASSERT(cnt);
    } while (AM_RC_SUPER == rc);

    bool tran = (AM_RC_TRAN == rc) || (AM_RC_TRAN_REDISPATCH == rc);
    if (!tran) {
        AM_ASSERT(
            (AM_RC_HANDLED == rc) || (AM_RC_HANDLED_ALIAS == rc) ||
            (AM_RC_SUPER == rc)
        );
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
    const int id = event->id;

#ifdef AM_HSM_SPY
    if (hsm->spy) {
        hsm->spy(hsm, event);
    }
#endif
    enum am_rc rc = hsm_dispatch(hsm, event);
    if (AM_RC_TRAN_REDISPATCH == rc) {
        /* Event was freed / corrupted ? */
        AM_ASSERT(id == event->id);
        rc = hsm_dispatch(hsm, event);
        AM_ASSERT(AM_RC_TRAN_REDISPATCH != rc);
    }

    hsm->dispatch_in_progress = false;

    /* Event was freed / corrupted ? */
    AM_ASSERT(id == event->id); /* cppcheck-suppress knownArgument */
}

bool am_hsm_is_in(struct am_hsm *hsm, struct am_hsm_state state) {
    AM_ASSERT(hsm);

    if (NULL == state.fn) {
        return NULL == hsm->state.fn;
    }

    struct am_hsm hsm_ = *hsm;

    int cnt = AM_HSM_HIERARCHY_DEPTH_MAX;
    while (!am_hsm_state_is_eq(hsm, state) && (hsm->state.fn != am_hsm_top)) {
        struct am_hsm_state s = hsm->state;
        *hsm = hsm_;
        enum am_rc rc = s.fn(hsm, &m_hsm_evt_empty);
        AM_ASSERT(AM_RC_SUPER == rc);
        --cnt;
        /* check if HSM hierarchy depth exceeds #AM_HSM_HIERARCHY_DEPTH_MAX */
        AM_ASSERT(cnt);
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

int am_hsm_get_instance(const struct am_hsm *hsm) {
    AM_ASSERT(hsm);
    return (int)hsm->smi;
}

struct am_hsm_state am_hsm_get_state(const struct am_hsm *hsm) {
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
    AM_ASSERT(hsm->ctor_called); /* was am_hsm_ctor() called? */
    AM_ASSERT(hsm->state.fn);

    hsm_exit(hsm, /*until=*/AM_HSM_STATE_CTOR(am_hsm_top));
    hsm_set_state(hsm, AM_HSM_STATE_CTOR(NULL));
    hsm->ctor_called = hsm->init_called = false;
}

void am_hsm_init(struct am_hsm *hsm, const struct am_event *init_event) {
    AM_ASSERT(hsm);
    AM_ASSERT(hsm->ctor_called); /* was am_hsm_ctor() called? */

    struct am_hsm_state state = hsm->state;
    enum am_rc rc = hsm->state.fn(hsm, init_event);
    AM_ASSERT(AM_RC_TRAN == rc);

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

enum am_rc am_hsm_top(struct am_hsm *hsm, const struct am_event *event) {
    (void)hsm;
    (void)event;
    return AM_RC_HANDLED;
}
