/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Adel Mamin
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
#include "regular.h"

/*
 * Contrived hierarchical state machine (HSM) that contains all possible
 * state transition topologies up to four level of state nesting.
 * Depicted by hsm.png file borrowed from
 * "Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded
 * Systems 2nd Edition" by Miro Samek <https://www.state-machine.com/psicc2>
 */

struct test {
    struct hsm hsm;
    int foo;
    void (*log)(char *fmt, ...);
};

static struct test m_test;

struct hsm *g_regular = &m_test.hsm;

static enum hsm_rc s2(struct test *me, const struct event *event);
static enum hsm_rc s21(struct test *me, const struct event *event);
static enum hsm_rc s211(struct test *me, const struct event *event);
static enum hsm_rc s1(struct test *me, const struct event *event);
static enum hsm_rc s11(struct test *me, const struct event *event);

static enum hsm_rc test_init(struct test *me, const struct event *event) {
    (void)event;

    me->foo = 0;
    me->log("top-INIT;");
    return HSM_TRAN(s2);
}

static enum hsm_rc s(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s11);

    case HSM_EVT_EXIT:
        me->log("s-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I: {
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        if (me->foo) {
            me->foo = 0;
            me->log("s-I;");
            return HSM_HANDLED();
        }
        return HSM_HANDLED();
    }
    case HSM_EVT_E:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        ASSERT(
            (hsm_state_is_eq(&me->hsm, &HSM_STATE(s211))) ||
            (hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)))
        );
        me->foo = 0;
        me->log("s-E;");
        return HSM_TRAN(s11);

    case HSM_EVT_TERM:
        me->log("s->TERM");
        return HSM_HANDLED();

    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc s1(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s1-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s1-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s11);

    case HSM_EVT_EXIT:
        me->log("s1-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s1-I;");
        return HSM_HANDLED();

    case HSM_EVT_C:
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s1-C;");
        return HSM_TRAN(s2);

    case HSM_EVT_F:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s1-F;");
        return HSM_TRAN(s211);

    case HSM_EVT_A:
        me->log("s1-A;");
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s1);

    case HSM_EVT_B:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s1-B;");
        return HSM_TRAN(s11);

    case HSM_EVT_D:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s1-D;");
            return HSM_TRAN(s);
        }
        return HSM_HANDLED();

    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc s11(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s11-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s11-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s11-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_G:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s11-G;");
        return HSM_TRAN(s211);

    case HSM_EVT_H:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s11-H;");
        return HSM_TRAN(s);

    case HSM_EVT_D:
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        if (me->foo) {
            me->foo = 0;
            me->log("s11-D;");
            return HSM_TRAN(s1);
        }
        break;

    default:
        break;
    }
    return HSM_SUPER(s1);
}

static enum hsm_rc s2(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s2-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s2-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s211);

    case HSM_EVT_EXIT:
        me->log("s2-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        if (!me->foo) {
            me->foo = 1;
            me->log("s2-I;");
            return HSM_HANDLED();
        }
        break;

    case HSM_EVT_F:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s2-F;");
        return HSM_TRAN(s11);

    case HSM_EVT_C:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s2-C;");
        return HSM_TRAN(s1);

    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc s21(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s21-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s21-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s211);

    case HSM_EVT_EXIT:
        me->log("s21-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_A:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s21-A;");
        return HSM_TRAN(s21);

    case HSM_EVT_B:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s21-B;");
        return HSM_TRAN(s211);

    case HSM_EVT_G:
        me->log("s21-G;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_TRAN(s1);

    default:
        break;
    }
    return HSM_SUPER(s2);
}

static enum hsm_rc s211(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        me->log("s211-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        me->log("s211-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        me->log("s211-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_D:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s211-D;");
        return HSM_TRAN(s21);

    case HSM_EVT_H:
        ASSERT(hsm_get_state_instance(&me->hsm) == 0);
        me->log("s211-H;");
        return HSM_TRAN(s);

    default:
        break;
    }
    return HSM_SUPER(s21);
}

void regular_ctor(void (*log)(char *fmt, ...)) {
    struct test *me = &m_test;
    hsm_ctor(&me->hsm, &HSM_STATE(test_init));
    me->log = log;
}
