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
 * Finate State Machine (FSM) API implementation.
 */

#include <stdbool.h>
#include <stddef.h>

#include "common/macros.h"
#include "fsm/fsm.h"

/** canned events */
static const struct am_event m_fsm_evt_entry = {.id = AM_FSM_EVT_ENTRY};
static const struct am_event m_fsm_evt_exit = {.id = AM_FSM_EVT_EXIT};

am_fsm_state am_fsm_get_active_state(const struct am_fsm *fsm) {
    AM_ASSERT(fsm);
    return AM_FSM_STATE(fsm->state);
}

/**
 * Enter state.
 *
 * @param fsm    the FSM state
 * @param state  the state to enter
 */
static void fsm_enter(struct am_fsm *fsm, const am_fsm_state state) {
    fsm->state = state;
    enum am_fsm_rc rc = fsm->state(fsm, &m_fsm_evt_entry);
    AM_ASSERT(AM_FSM_RC_HANDLED == rc);
}

/**
 * Exit active state.
 *
 * @param fsm  exit the active state of this FSM
 */
static void fsm_exit(struct am_fsm *fsm) {
    enum am_fsm_rc rc = fsm->state(fsm, &m_fsm_evt_exit);
    AM_ASSERT(AM_FSM_RC_HANDLED == rc);
}

static enum am_fsm_rc fsm_dispatch(
    struct am_fsm *fsm, const struct am_event *event
) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state);
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    am_fsm_state src = fsm->state;
    enum am_fsm_rc rc = fsm->state(fsm, event);
    if (AM_FSM_RC_HANDLED == rc) {
        AM_ASSERT(fsm->state == src);
        return rc;
    }
    /* transition was taken */
    am_fsm_state dst = fsm->state;
    fsm->state = src;
    fsm_exit(fsm);
    fsm_enter(fsm, dst);

    return rc;
}

void am_fsm_dispatch(struct am_fsm *fsm, const struct am_event *event) {
#ifdef AM_FSM_SPY
    if (fsm->spy) {
        fsm->spy(fsm, event);
    }
#endif
    enum am_fsm_rc rc = fsm_dispatch(fsm, event);
    if (AM_FSM_RC_TRAN_REDISPATCH == rc) {
        rc = fsm_dispatch(fsm, event);
        AM_ASSERT(AM_FSM_RC_TRAN_REDISPATCH != rc);
    }
}

bool am_fsm_is_in(const struct am_fsm *fsm, const am_fsm_state state) {
    AM_ASSERT(fsm);
    return fsm->state == state;
}

void am_fsm_ctor(struct am_fsm *fsm, const am_fsm_state state) {
    AM_ASSERT(fsm);
    AM_ASSERT(state);
    fsm->state = state;
}

void am_fsm_dtor(struct am_fsm *fsm) {
    AM_ASSERT(fsm);
    fsm_exit(fsm);
    fsm->state = NULL;
}

void am_fsm_init(struct am_fsm *fsm, const struct am_event *init_event) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state); /* was am_fsm_ctor() called? */

    enum am_fsm_rc rc = fsm->state(fsm, init_event);
    AM_ASSERT(AM_FSM_RC_TRAN == rc);
    fsm_enter(fsm, fsm->state);
}

#ifdef AM_FSM_SPY
void am_fsm_set_spy(struct am_fsm *fsm, am_fsm_spy_fn spy) {
    AM_ASSERT(fsm);
    fsm->spy = spy;
}
#endif
