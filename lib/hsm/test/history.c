/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
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

#define HSM_EVT_ON (HSM_EVT_USER)
#define HSM_EVT_OFF (HSM_EVT_USER + 1)
#define HSM_EVT_OPEN (HSM_EVT_USER + 2)
#define HSM_EVT_CLOSE (HSM_EVT_USER + 3)

struct oven {
    struct hsm hsm;
    struct hsm_state history;
};

static struct oven m_oven;

/* test transition to HSM history */

static enum hsm_rc oven_open(struct oven *me, const struct event *event);
static enum hsm_rc oven_closed(struct oven *me, const struct event *event);
static enum hsm_rc oven_on(struct oven *me, const struct event *event);
static enum hsm_rc oven_off(struct oven *me, const struct event *event);

static bool oven_is_open(void) { return false; }

static enum hsm_rc oven_closed(struct oven *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_INIT:
        return HSM_TRAN(oven_off);

    case HSM_EVT_OPEN:
        return HSM_TRAN(oven_open);

    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc oven_on(struct oven *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->history = HSM_STATE(oven_on);
        return HSM_HANDLED();

    case HSM_EVT_ON:
        return HSM_HANDLED();

    case HSM_EVT_OFF:
        return HSM_TRAN(oven_off);

    default:
        break;
    }
    return HSM_SUPER(oven_closed);
}

static enum hsm_rc oven_off(struct oven *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->history = HSM_STATE(oven_off);
        return HSM_HANDLED();

    case HSM_EVT_ON:
        return HSM_TRAN(oven_on);

    case HSM_EVT_OFF:
        return HSM_HANDLED();

    default:
        break;
    }
    return HSM_SUPER(oven_closed);
}

static enum hsm_rc oven_open(struct oven *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ON:
        me->history = HSM_STATE(oven_on);
        return HSM_HANDLED();

    case HSM_EVT_OFF:
        me->history = HSM_STATE(oven_off);
        return HSM_HANDLED();

    case HSM_EVT_CLOSE:
        return HSM_TRAN(me->history.fn, me->history.instance);

    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc oinit(struct oven *me, const struct event *event) {
    (void)event;
    me->history = HSM_STATE(oven_off);
    if (oven_is_open()) {
        return HSM_TRAN(oven_open);
    }
    return HSM_TRAN(oven_closed);
}

static void test_oven(void) {
    struct oven *me = &m_oven;
    hsm_ctor(&me->hsm, &HSM_STATE(oinit));

    hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(oven_off)));

    {
        struct event e = {.id = HSM_EVT_ON};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(oven_on)));
    }
    {
        struct event e = {.id = HSM_EVT_OPEN};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(oven_open)));
    }
    {
        struct event e = {.id = HSM_EVT_CLOSE};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(oven_on)));
    }
}

int main(void) {
    test_oven();
    return 0;
}
