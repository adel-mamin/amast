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

#include <stdio.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"

/*
 * This test is a full implementation of the example described in
 * SUBMACHINES section of README.rst
 */

/* s1 submachine indices */
#define S1_0 0
#define S1_1 1

#define FOO (AM_EVT_USER)
#define BAR (AM_EVT_USER + 1)
#define BAZ (AM_EVT_USER + 2)

struct basic_sm {
    struct am_hsm hsm;
};

static struct basic_sm m_basic_sm;

static enum am_hsm_rc bs_s(struct basic_sm *me, const struct am_event *event);
static enum am_hsm_rc bs_s1(struct basic_sm *me, const struct am_event *event);
static enum am_hsm_rc bs_s2(struct basic_sm *me, const struct am_event *event);
static enum am_hsm_rc bs_s3(struct basic_sm *me, const struct am_event *event);

static enum am_hsm_rc bs_s(struct basic_sm *me, const struct am_event *event) {
    AM_ASSERT(0 == am_hsm_instance(&me->hsm));
    switch (event->id) {
    case FOO:
        return AM_HSM_TRAN(bs_s1, /*instance=*/S1_0);
    case BAR:
        return AM_HSM_TRAN(bs_s1, /*instance=*/S1_1);
    case BAZ:
        return AM_HSM_TRAN(bs_s);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc bs_s1(struct basic_sm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        static const struct am_hsm_state tt[] = {
            [S1_0] = {.fn = (am_hsm_state_fn)bs_s2},
            [S1_1] = {.fn = (am_hsm_state_fn)bs_s3}
        };
        int instance = am_hsm_instance(&me->hsm);
        AM_ASSERT(instance < AM_COUNTOF(tt));
        return AM_HSM_TRAN(tt[instance].fn);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(bs_s);
}

static enum am_hsm_rc bs_s2(struct basic_sm *me, const struct am_event *event) {
    (void)event;
    AM_ASSERT(0 == am_hsm_instance(&me->hsm));
    return AM_HSM_SUPER(bs_s1, S1_0);
}

static enum am_hsm_rc bs_s3(struct basic_sm *me, const struct am_event *event) {
    (void)event;
    AM_ASSERT(0 == am_hsm_instance(&me->hsm));
    return AM_HSM_SUPER(bs_s1, S1_1);
}

static enum am_hsm_rc bs_init(struct basic_sm *me, const struct am_event *evt) {
    (void)evt;
    return AM_HSM_TRAN(bs_s);
}

static void test_basic_sm(void) {
    struct basic_sm *me = &m_basic_sm;
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(bs_init));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(bs_s)));

    {
        struct am_event e = {.id = FOO};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_0)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_1)));
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(bs_s2)));
    }
    {
        struct am_event e = {.id = BAZ};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_0)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_1)));
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(bs_s)));
    }
    {
        struct am_event e = {.id = BAR};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_0)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_1)));
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(bs_s3)));
    }
    {
        struct am_event e = {.id = BAZ};
        am_hsm_dispatch(&me->hsm, &e);
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_0)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(bs_s1, S1_1)));
        AM_ASSERT(am_hsm_state_is_eq(&me->hsm, AM_HSM_STATE_CTOR(bs_s)));
    }
}

int main(void) {
    test_basic_sm();

    return 0;
}
