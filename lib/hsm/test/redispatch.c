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

struct redispatch {
    struct am_hsm hsm;
    int foo;
    int foo2;
};

static struct redispatch m_redispatch;

/* test AM_HSM_TRAN_REDISPATCH() */

static enum am_hsm_rc s1(struct redispatch *me, const struct event *event);
static enum am_hsm_rc s2(struct redispatch *me, const struct event *event);

static enum am_hsm_rc s1(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        return AM_HSM_TRAN_REDISPATCH(s2);
    case HSM_EVT_B:
        me->foo2 = 2;
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s2(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        me->foo = 1;
        return AM_HSM_HANDLED();
    case HSM_EVT_B:
        return AM_HSM_TRAN_REDISPATCH(s1, 0);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc sinit(struct redispatch *me, const struct event *event) {
    (void)event;
    me->foo = 0;
    me->foo2 = 0;
    return AM_HSM_TRAN(s1);
}

static void test_redispatch(void) {
    struct redispatch *me = &m_redispatch;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(0 == me->foo);

    {
        static const struct event e = {.id = HSM_EVT_A};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(1 == me->foo);
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(s2)));
    }
    {
        static const struct event e = {.id = HSM_EVT_B};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(2 == me->foo2);
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, &AM_HSM_STATE(s1)));
    }
}

int main(void) {
    test_redispatch();
    return 0;
}
