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

#include <stdio.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"

/* s1 submachine indices */
#define S1_0 0
#define S1_1 1

#define FOO (HSM_EVT_USER)
#define BAR (HSM_EVT_USER + 1)
#define BAZ (HSM_EVT_USER + 2)

struct basic {
    struct hsm hsm;
};

static struct basic m_basic;

static enum hsm_rc s(struct basic *me, const struct event *event);
static enum hsm_rc s1(struct basic *me, const struct event *event);
static enum hsm_rc s2(struct basic *me, const struct event *event);
static enum hsm_rc s3(struct basic *me, const struct event *event);

static enum hsm_rc s(struct basic *me, const struct event *event) {
    switch (event->id) {
    case FOO:
        return HSM_TRAN(s1, /*instance=*/S1_0);
    case BAR:
        return HSM_TRAN(s1, /*instance=*/S1_1);
    case BAZ:
        return HSM_TRAN(s);
    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc s1(struct basic *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_INIT: {
        static const struct hsm_state tt[] = {
            [S1_0] = {.fn = HSM_STATE_FN(s2)},
            [S1_1] = {.fn = HSM_STATE_FN(s3)}
        };
        int instance = hsm_get_state_instance(&me->hsm);
        ASSERT(instance < ARRAY_SIZE(tt));
        const struct hsm_state *tran = &tt[instance];
        return HSM_TRAN(tran->fn, tran->instance);
    }
    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc s2(struct basic *me, const struct event *event) {
    (void)event;
    return HSM_SUPER(s1, S1_0);
}

static enum hsm_rc s3(struct basic *me, const struct event *event) {
    (void)event;
    return HSM_SUPER(s1, S1_1);
}

static enum hsm_rc binit(struct basic *me, const struct event *event) {
    (void)event;
    return HSM_TRAN(s);
}

static void test_basic(void) {
    struct basic *me = &m_basic;
    hsm_ctor(&me->hsm, &HSM_STATE(binit));

    hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s)));

    {
        struct event e = {.id = FOO};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, S1_0)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s2)));
    }
    {
        struct event e = {.id = BAZ};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s)));
    }
    {
        struct event e = {.id = BAR};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, S1_1)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s3)));
    }
    {
        struct event e = {.id = BAZ};
        hsm_dispatch(&me->hsm, &e);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s)));
    }
}

int main(void) {
    test_basic();

    return 0;
}
