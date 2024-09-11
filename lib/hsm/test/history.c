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

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"

#define HSM_EVT_ON (EVT_USER)
#define HSM_EVT_OFF (EVT_USER + 1)
#define HSM_EVT_OPEN (EVT_USER + 2)
#define HSM_EVT_CLOSE (EVT_USER + 3)

struct oven {
    struct am_hsm hsm;
    struct am_hsm_state history;
};

static struct oven m_oven;

/* test transition to HSM history */

static enum am_hsm_rc oven_open(struct oven *me, const struct event *event);
static enum am_hsm_rc oven_closed(struct oven *me, const struct event *event);
static enum am_hsm_rc oven_on(struct oven *me, const struct event *event);
static enum am_hsm_rc oven_off(struct oven *me, const struct event *event);

static bool oven_is_open(void) { return false; }

static enum am_hsm_rc oven_open(struct oven *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ON:
        me->history = AM_HSM_STATE(oven_on);
        return AM_HSM_HANDLED();

    case HSM_EVT_OFF:
        me->history = AM_HSM_STATE(oven_off);
        return AM_HSM_HANDLED();

    case HSM_EVT_CLOSE:
        return AM_HSM_TRAN(me->history.fn, me->history.ifn);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc oven_closed(struct oven *me, const struct event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT:
        return AM_HSM_TRAN(oven_off);

    case HSM_EVT_OPEN:
        return AM_HSM_TRAN(oven_open);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc oven_on(struct oven *me, const struct event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->history = AM_HSM_STATE(oven_on);
        return AM_HSM_HANDLED();

    case HSM_EVT_ON:
        return AM_HSM_HANDLED();

    case HSM_EVT_OFF:
        return AM_HSM_TRAN(oven_off);

    default:
        break;
    }
    return AM_HSM_SUPER(oven_closed);
}

static enum am_hsm_rc oven_off(struct oven *me, const struct event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->history = AM_HSM_STATE(oven_off);
        return AM_HSM_HANDLED();

    case HSM_EVT_ON:
        return AM_HSM_TRAN(oven_on);

    case HSM_EVT_OFF:
        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(oven_closed);
}

static enum am_hsm_rc oven_init(struct oven *me, const struct event *event) {
    (void)event;
    me->history = AM_HSM_STATE(oven_off);
    return AM_HSM_TRAN(oven_is_open() ? oven_open : oven_closed);
}

static void test_oven(void) {
    struct oven *me = &m_oven;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(oven_init));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(oven_off)));

    {
        struct event e = {.id = HSM_EVT_ON};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(oven_on)));
    }
    {
        struct event e = {.id = HSM_EVT_OPEN};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(oven_open)));
    }
    {
        struct event e = {.id = HSM_EVT_CLOSE};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(oven_on)));
    }
}

int main(void) {
    test_oven();
    return 0;
}
