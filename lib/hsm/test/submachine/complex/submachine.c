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
#include "submachine.h"

/* Test submachines. */

struct test {
    struct hsm hsm;
    void (*log)(char *fmt, ...);
};

static struct test m_test;

struct hsm *g_submachine = &m_test.hsm;

#define SM_0 0
#define SM_1 1
#define SM_2 2

static enum hsm_rc s1(struct test *me, const struct event *event);
static enum hsm_rc s11(struct test *me, const struct event *event);
static enum hsm_rc s2(struct test *me, const struct event *event);
static enum hsm_rc s21(struct test *me, const struct event *event);

static enum hsm_rc s1(struct test *me, const struct event *event) {
    const int instance = hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s1/%d-ENTRY;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s1/%d-EXIT;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s1/%d-INIT;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        return HSM_TRAN(s11, instance);

    case HSM_EVT_A:
        me->log("s1/%d-A;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 0)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 1)));
        return HSM_TRAN(s1, instance);

    case HSM_EVT_B:
        me->log("s1/%d-B;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 0)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 1)));
        return HSM_TRAN(s2, instance);

    case HSM_EVT_D: {
        me->log("s1/%d-D;", hsm_get_state_instance(&me->hsm));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, instance)));

        static const struct hsm_state tt[] = {
            [SM_0] = {.fn = HSM_STATE_FN(s1), .instance = SM_1},
            [SM_1] = {.fn = HSM_STATE_FN(s1), .instance = SM_0},
            [SM_2] = {.fn = HSM_STATE_FN(s1), .instance = SM_2}
        };
        ASSERT(instance < ARRAY_SIZE(tt));
        const struct hsm_state *tran = &tt[instance];

        return HSM_TRAN(tran->fn, tran->instance);
    }
    default:
        break;
    }

    static const struct hsm_state ss[] = {
        [SM_0] = {.fn = hsm_top, .instance = 0},
        [SM_1] = {.fn = HSM_STATE_FN(s1), .instance = SM_0},
        [SM_2] = {.fn = hsm_top, .instance = 0}
    };
    ASSERT(instance < ARRAY_SIZE(ss));
    const struct hsm_state *super = &ss[instance];

    return HSM_SUPER(super->fn, super->instance);
}

static enum hsm_rc s11(struct test *me, const struct event *event) {
    const int instance = hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s11/%d-ENTRY;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s11/%d-EXIT;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s11/%d-INIT;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_C:
        me->log("s11/%d-C;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s11, instance);

    case HSM_EVT_E:
        me->log("s11/%d-E;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s2, SM_2);

    default:
        break;
    }
    return HSM_SUPER(s1, instance);
}

static enum hsm_rc s2(struct test *me, const struct event *event) {
    const int instance = hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s2/%d-ENTRY;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s2/%d-EXIT;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s2/%d-INIT;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s21, instance);

    case HSM_EVT_A:
        me->log("s2/%d-A;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s2, instance);

    case HSM_EVT_B:
        me->log("s2/%d-B;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s1, instance);
    default:
        break;
    }

    static const struct hsm_state ss[] = {
        [SM_0] = {.fn = hsm_top, .instance = 0},
        [SM_1] = {.fn = HSM_STATE_FN(s2), .instance = SM_0},
        [SM_2] = {.fn = hsm_top, .instance = 0}
    };
    ASSERT(instance < ARRAY_SIZE(ss));
    const struct hsm_state *super = &ss[instance];

    return HSM_SUPER(super->fn, super->instance);
}

static enum hsm_rc s21(struct test *me, const struct event *event) {
    const int instance = hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s21/%d-ENTRY;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s21/%d-EXIT;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s21/%d-INIT;", hsm_get_state_instance(&me->hsm));
        return HSM_HANDLED();

    case HSM_EVT_C:
        me->log("s11/%d-C;", hsm_get_state_instance(&me->hsm));
        return HSM_TRAN(s21, instance);

    default:
        break;
    }
    return HSM_SUPER(s2, instance);
}

static enum hsm_rc sinit(struct test *me, const struct event *event) {
    (void)event;

    me->log("top/%d-INIT;", hsm_get_state_instance(&me->hsm));

    return HSM_TRAN(s1, SM_1);
}

void submachine_ctor(void (*log)(char *fmt, ...)) {
    struct test *me = &m_test;
    hsm_ctor(&me->hsm, &HSM_STATE(sinit));
    me->log = log;
}
