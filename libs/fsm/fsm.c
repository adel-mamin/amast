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
 * Finite State Machine (FSM) API implementation.
 */

#include <stdbool.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "fsm/fsm.h"

am_fsm_state_fn am_fsm_get_state(const struct am_fsm *fsm) {
    AM_ASSERT(fsm);
    return AM_FSM_STATE_CTOR(fsm->state);
}

/**
 * Enter state.
 *
 * @param fsm    FSM handler
 * @param state  the state to enter
 */
static void fsm_enter(struct am_fsm *fsm, const am_fsm_state_fn state) {
    fsm->state = state;
    struct am_event entry = {.id = AM_EVT_FSM_ENTRY};
    enum am_rc rc = fsm->state(fsm, &entry);
    AM_ASSERT(AM_RC_HANDLED == rc);
}

/**
 * Exit active state.
 *
 * @param fsm  FSM handler
 */
static void fsm_exit(struct am_fsm *fsm) {
    struct am_event exit = {.id = AM_EVT_FSM_EXIT};
    enum am_rc rc = fsm->state(fsm, &exit);
    AM_ASSERT(AM_RC_HANDLED == rc);
}

static enum am_rc fsm_dispatch(
    struct am_fsm *fsm, const struct am_event *event
) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state);
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    am_fsm_state_fn src = fsm->state;
    enum am_rc rc = fsm->state(fsm, event);
    if ((AM_RC_HANDLED == rc) || (AM_RC_HANDLED_ALIAS == rc)) {
        AM_ASSERT(fsm->state == src);
        return rc;
    }
    /* transition was triggered */
    am_fsm_state_fn dst = fsm->state;
    fsm->state = src;
    fsm_exit(fsm);
    fsm_enter(fsm, dst);

    return rc;
}

void am_fsm_dispatch(struct am_fsm *fsm, const struct am_event *event) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state);
    AM_ASSERT(fsm->init_called);
    AM_ASSERT(!fsm->dispatch_in_progress);
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    fsm->dispatch_in_progress = true;
    const int id = event->id;

#ifdef AM_FSM_SPY
    if (fsm->spy) {
        fsm->spy(fsm, event);
    }
#endif
    enum am_rc rc = fsm_dispatch(fsm, event);
    if (AM_RC_TRAN_REDISPATCH == rc) {
        rc = fsm_dispatch(fsm, event);
        AM_ASSERT(AM_RC_TRAN_REDISPATCH != rc);
    }

    fsm->dispatch_in_progress = false;

    /*
     * Event was freed / corrupted ?
     *
     * One possible reason could be the following usage scenario:
     *
     *  const struct am_event *e = am_event_allocate(id, size);
     *  am_event_inc_ref_cnt(e); <-- THIS IS MISSING
     *  am_fsm_dispatch(fsm, e);
     *      am_event_push_XXX(queue, e) & am_event_pop_front(queue, ...)
     *      OR
     *      am_event_inc_ref_cnt(e) & am_event_dec_ref_cnt(e)
     *  am_event_free(&e);
     */
    AM_ASSERT(id == event->id); /* cppcheck-suppress knownArgument */
}

bool am_fsm_is_in(const struct am_fsm *fsm, const am_fsm_state_fn state) {
    AM_ASSERT(fsm);
    return fsm->state == state;
}

void am_fsm_ctor(struct am_fsm *fsm, const am_fsm_state_fn state) {
    AM_ASSERT(fsm);
    AM_ASSERT(state);
    memset(fsm, 0, sizeof(*fsm));
    fsm->state = state;
}

void am_fsm_dtor(struct am_fsm *fsm) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state); /* was am_fsm_ctor() called? */
    fsm_exit(fsm);
    fsm->state = NULL;
    fsm->init_called = false;
}

void am_fsm_init(struct am_fsm *fsm, const struct am_event *init_event) {
    AM_ASSERT(fsm);
    AM_ASSERT(fsm->state); /* was am_fsm_ctor() called? */

    enum am_rc rc = fsm->state(fsm, init_event);
    AM_ASSERT(AM_RC_TRAN == rc);
    fsm_enter(fsm, fsm->state);
    fsm->init_called = true;
}

#ifdef AM_FSM_SPY
void am_fsm_set_spy(struct am_fsm *fsm, am_fsm_spy_fn spy) {
    AM_ASSERT(fsm);
    fsm->spy = spy;
}
#endif
