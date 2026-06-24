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

#include <stddef.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "fsm/fsm.h"

#define FSM_EVT_A (AM_EVT_USER)
#define FSM_EVT_B (AM_EVT_USER + 1)

struct redisp_fsm {
    struct am_fsm fsm;
    int a_handled;
    int b_handled;
};

static struct redisp_fsm m_redisp_fsm;

/* test am_fsm_tran_redispatch() */

static enum am_rc redisp_fsm_s1(
    struct am_fsm* fsm, const struct am_event* event
);
static enum am_rc redisp_fsm_s2(
    struct am_fsm* fsm, const struct am_event* event
);

static enum am_rc redisp_fsm_s1(
    struct am_fsm* fsm, const struct am_event* event
) {
    struct redisp_fsm* me = AM_CONTAINER_OF(fsm, struct redisp_fsm, fsm);
    switch (event->id) {
    case FSM_EVT_A:
        return am_fsm_tran_redispatch(fsm, redisp_fsm_s2);
    case FSM_EVT_B:
        me->b_handled = 1;
        return am_fsm_handled(fsm);
    default:
        break;
    }
    return am_fsm_handled(fsm);
}

static enum am_rc redisp_fsm_s2(
    struct am_fsm* fsm, const struct am_event* event
) {
    struct redisp_fsm* me = AM_CONTAINER_OF(fsm, struct redisp_fsm, fsm);
    switch (event->id) {
    case FSM_EVT_A:
        me->a_handled = 1;
        return am_fsm_handled(fsm);
    case FSM_EVT_B:
        return am_fsm_tran_redispatch(fsm, redisp_fsm_s1);
    default:
        break;
    }
    return am_fsm_handled(fsm);
}

static enum am_rc redisp_fsm_sinit(
    struct am_fsm* fsm, const struct am_event* event
) {
    (void)event;
    struct redisp_fsm* me = AM_CONTAINER_OF(fsm, struct redisp_fsm, fsm);
    me->a_handled = 0;
    me->b_handled = 0;
    return am_fsm_tran(fsm, redisp_fsm_s1);
}

static void redispatch_fsm(void) {
    struct redisp_fsm* me = &m_redisp_fsm;
    am_fsm_create(&me->fsm, redisp_fsm_sinit);

    am_fsm_init(&me->fsm, /*init_event=*/NULL);
    AM_ASSERT(0 == me->a_handled);
    AM_ASSERT(0 == me->b_handled);

    static const struct am_event e1 = {.id = FSM_EVT_A};
    am_fsm_dispatch(&me->fsm, &e1);
    AM_ASSERT(1 == me->a_handled);
    AM_ASSERT(am_fsm_is_in(&me->fsm, redisp_fsm_s2));

    static const struct am_event e2 = {.id = FSM_EVT_B};
    am_fsm_dispatch(&me->fsm, &e2);
    AM_ASSERT(1 == me->b_handled);
    AM_ASSERT(am_fsm_is_in(&me->fsm, redisp_fsm_s1));
}

int main(void) {
    redispatch_fsm();
    return 0;
}
