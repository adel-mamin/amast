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

#include <stddef.h>
#include <stdbool.h>

#include "common/macros.h"
#include "event/event.h"
#include "fsm/fsm.h"

#define FSM_EVT_ON (AM_EVT_USER)
#define FSM_EVT_OFF (AM_EVT_USER + 1)
#define FSM_EVT_OPEN (AM_EVT_USER + 2)
#define FSM_EVT_CLOSE (AM_EVT_USER + 3)

struct oven_fsm {
    struct am_fsm fsm;
    am_fsm_state_fn history;
};

static struct oven_fsm m_oven_fsm;

/* test transition to FSM history */

static enum am_fsm_rc oven_fsm_open(
    struct oven_fsm *me, const struct am_event *event
);
static enum am_fsm_rc oven_fsm_on(
    struct oven_fsm *me, const struct am_event *event
);
static enum am_fsm_rc oven_fsm_off(
    struct oven_fsm *me, const struct am_event *event
);

static bool oven_fsm_is_open(void) { return false; }

static enum am_fsm_rc oven_fsm_open(
    struct oven_fsm *me, const struct am_event *event
) {
    switch (event->id) {
    case FSM_EVT_ON:
        me->history = am_fsm_state(&me->fsm);
        return AM_FSM_HANDLED();

    case FSM_EVT_OFF:
        me->history = am_fsm_state(&me->fsm);
        return AM_FSM_HANDLED();

    case FSM_EVT_CLOSE:
        return AM_FSM_TRAN(me->history);

    default:
        break;
    }
    return AM_FSM_HANDLED();
}

static enum am_fsm_rc oven_fsm_on(
    struct oven_fsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_FSM_EVT_ENTRY:
        me->history = am_fsm_state(&me->fsm);
        return AM_FSM_HANDLED();

    case FSM_EVT_OFF:
        return AM_FSM_TRAN(oven_fsm_off);

    case FSM_EVT_OPEN:
        return AM_FSM_TRAN(oven_fsm_open);

    default:
        break;
    }
    return AM_FSM_HANDLED();
}

static enum am_fsm_rc oven_fsm_off(
    struct oven_fsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_FSM_EVT_ENTRY:
        me->history = am_fsm_state(&me->fsm);
        return AM_FSM_HANDLED();

    case FSM_EVT_ON:
        return AM_FSM_TRAN(oven_fsm_on);

    case FSM_EVT_OPEN:
        return AM_FSM_TRAN(oven_fsm_open);

    default:
        break;
    }
    return AM_FSM_HANDLED();
}

static enum am_fsm_rc oven_fsm_init(
    struct oven_fsm *me, const struct am_event *event
) {
    (void)event;
    me->history = AM_FSM_STATE_CTOR(oven_fsm_off);
    return AM_FSM_TRAN(oven_fsm_is_open() ? oven_fsm_open : oven_fsm_off);
}

static void test_oven_fsm(void) {
    struct oven_fsm *me = &m_oven_fsm;
    am_fsm_ctor(&me->fsm, AM_FSM_STATE_CTOR(oven_fsm_init));

    am_fsm_init(&me->fsm, /*init_event=*/NULL);
    AM_ASSERT(am_fsm_is_in(&me->fsm, AM_FSM_STATE_CTOR(oven_fsm_off)));

    struct am_event e1 = {.id = FSM_EVT_ON};
    am_fsm_dispatch(&me->fsm, &e1);
    AM_ASSERT(am_fsm_is_in(&me->fsm, AM_FSM_STATE_CTOR(oven_fsm_on)));

    struct am_event e2 = {.id = FSM_EVT_OPEN};
    am_fsm_dispatch(&me->fsm, &e2);
    AM_ASSERT(am_fsm_is_in(&me->fsm, AM_FSM_STATE_CTOR(oven_fsm_open)));

    struct am_event e3 = {.id = FSM_EVT_CLOSE};
    am_fsm_dispatch(&me->fsm, &e3);
    AM_ASSERT(am_fsm_is_in(&me->fsm, AM_FSM_STATE_CTOR(oven_fsm_on)));
}

int main(void) {
    test_oven_fsm();
    return 0;
}
