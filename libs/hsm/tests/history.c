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
#include <stdbool.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"

#define HSM_EVT_ON (AM_EVT_USER)
#define HSM_EVT_OFF (AM_EVT_USER + 1)
#define HSM_EVT_OPEN (AM_EVT_USER + 2)
#define HSM_EVT_CLOSE (AM_EVT_USER + 3)

struct oven_hsm {
    struct am_hsm hsm;
    struct am_hsm_state history;
};

static struct oven_hsm m_oven_hsm;

/* test transition to HSM history */

static enum am_hsm_rc oven_hsm_open(
    struct oven_hsm *me, const struct am_event *event
);
static enum am_hsm_rc oven_hsm_closed(
    struct oven_hsm *me, const struct am_event *event
);
static enum am_hsm_rc oven_hsm_on(
    struct oven_hsm *me, const struct am_event *event
);
static enum am_hsm_rc oven_hsm_off(
    struct oven_hsm *me, const struct am_event *event
);

static bool oven_hsm_is_open(void) { return false; }

static enum am_hsm_rc oven_hsm_open(
    struct oven_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case HSM_EVT_OFF:
        me->history = AM_HSM_STATE_CTOR(oven_hsm_off);
        return AM_HSM_HANDLED();

    case HSM_EVT_CLOSE:
        return AM_HSM_TRAN(oven_hsm_closed);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc oven_hsm_closed(
    struct oven_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_INIT:
        return AM_HSM_TRAN(me->history.fn);

    case HSM_EVT_OPEN:
        return AM_HSM_TRAN(oven_hsm_open);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc oven_hsm_on(
    struct oven_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        me->history = am_hsm_get_state(&me->hsm);
        return AM_HSM_HANDLED();

    case HSM_EVT_OFF:
        return AM_HSM_TRAN(oven_hsm_off);

    default:
        break;
    }
    return AM_HSM_SUPER(oven_hsm_closed);
}

static enum am_hsm_rc oven_hsm_off(
    struct oven_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        me->history = am_hsm_get_state(&me->hsm);
        return AM_HSM_HANDLED();

    case HSM_EVT_ON:
        return AM_HSM_TRAN(oven_hsm_on);

    default:
        break;
    }
    return AM_HSM_SUPER(oven_hsm_closed);
}

static enum am_hsm_rc oven_hsm_init(
    struct oven_hsm *me, const struct am_event *event
) {
    (void)event;
    me->history = AM_HSM_STATE_CTOR(oven_hsm_off);
    return AM_HSM_TRAN(oven_hsm_is_open() ? oven_hsm_open : oven_hsm_closed);
}

static void test_oven_hsm(void) {
    struct oven_hsm *me = &m_oven_hsm;
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(oven_hsm_init));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(oven_hsm_off)));

    struct am_event e1 = {.id = HSM_EVT_ON};
    am_hsm_dispatch(&me->hsm, &e1);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(oven_hsm_on)));

    struct am_event e2 = {.id = HSM_EVT_OPEN};
    am_hsm_dispatch(&me->hsm, &e2);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(oven_hsm_open)));

    struct am_event e3 = {.id = HSM_EVT_CLOSE};
    am_hsm_dispatch(&me->hsm, &e3);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(oven_hsm_on)));
}

int main(void) {
    test_oven_hsm();
    return 0;
}
