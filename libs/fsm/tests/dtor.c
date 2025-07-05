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

#include "event/event.h"
#include "fsm/fsm.h"

struct dtor_fsm {
    struct am_fsm fsm;
};

static struct dtor_fsm m_dtor_fsm;

/* test am_fsm_dtor() */

static enum am_rc dtor_fsm_s(
    struct dtor_fsm *me, const struct am_event *event
) {
    (void)me;
    (void)event;
    return AM_FSM_HANDLED();
}

static enum am_rc dtor_fsm_sinit(
    struct dtor_fsm *me, const struct am_event *event
) {
    (void)event;
    return AM_FSM_TRAN(dtor_fsm_s);
}

static void dtor_fsm(void) {
    struct dtor_fsm *me = &m_dtor_fsm;
    am_fsm_ctor(&me->fsm, AM_FSM_STATE_CTOR(dtor_fsm_sinit));
    am_fsm_init(&me->fsm, /*init_event=*/NULL);
    am_fsm_dtor(&me->fsm);
    AM_ASSERT(am_fsm_is_in(&me->fsm, NULL));
}

int main(void) {
    dtor_fsm();
    return 0;
}
