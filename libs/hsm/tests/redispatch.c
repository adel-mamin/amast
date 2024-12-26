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
#include "event/event.h"
#include "hsm/hsm.h"
#include "common.h"

struct test_redisp {
    struct am_hsm hsm;
    int foo;
    int foo2;
};

static struct test_redisp m_test_redisp;

/* test AM_HSM_TRAN_REDISPATCH() */

static enum am_hsm_rc redisp_s1(
    struct test_redisp *me, const struct am_event *event
);
static enum am_hsm_rc redisp_s2(
    struct test_redisp *me, const struct am_event *event
);

static enum am_hsm_rc redisp_s1(
    struct test_redisp *me, const struct am_event *event
) {
    switch (event->id) {
    case HSM_EVT_A:
        return AM_HSM_TRAN_REDISPATCH(redisp_s2);
    case HSM_EVT_B:
        me->foo2 = 2;
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc redisp_s2(
    struct test_redisp *me, const struct am_event *event
) {
    switch (event->id) {
    case HSM_EVT_A:
        me->foo = 1;
        return AM_HSM_HANDLED();
    case HSM_EVT_B:
        return AM_HSM_TRAN_REDISPATCH(redisp_s1, 0);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc redisp_sinit(
    struct test_redisp *me, const struct am_event *event
) {
    (void)event;
    me->foo = 0;
    me->foo2 = 0;
    return AM_HSM_TRAN(redisp_s1);
}

static void test_redispatch(void) {
    struct test_redisp *me = &m_test_redisp;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE_CTOR(redisp_sinit));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(0 == me->foo);

    static const struct am_event e1 = {.id = HSM_EVT_A};
    am_hsm_dispatch(&me->hsm, &e1);
    AM_ASSERT(1 == me->foo);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE_CTOR(redisp_s2)));

    static const struct am_event e2 = {.id = HSM_EVT_B};
    am_hsm_dispatch(&me->hsm, &e2);
    AM_ASSERT(2 == me->foo2);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE_CTOR(redisp_s1)));
}

int main(void) {
    test_redispatch();
    return 0;
}
