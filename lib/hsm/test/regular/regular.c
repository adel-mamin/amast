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
    struct a1hsm hsm;
    int foo;
    void (*log)(char *fmt, ...);
};

static struct test m_test;

struct a1hsm *g_regular = &m_test.hsm;

static enum a1hsmrc s2(struct test *me, const struct event *event);
static enum a1hsmrc s21(struct test *me, const struct event *event);
static enum a1hsmrc s211(struct test *me, const struct event *event);
static enum a1hsmrc s1(struct test *me, const struct event *event);
static enum a1hsmrc s11(struct test *me, const struct event *event);

static enum a1hsmrc test_init(struct test *me, const struct event *event) {
    (void)event;

    me->foo = 0;
    me->log("top-INIT;");
    return A1HSM_TRAN(s2);
}

static enum a1hsmrc s(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        return A1HSM_TRAN(s11);

    case A1HSM_EVT_EXIT:
        me->log("s-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        return A1HSM_HANDLED();

    case HSM_EVT_I: {
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s211)));
        if (me->foo) {
            me->foo = 0;
            me->log("s-I;");
            return A1HSM_HANDLED();
        }
        return A1HSM_HANDLED();
    }
    case HSM_EVT_E:
        ASSERT(
            (a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s211))) ||
            (a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)))
        );
        me->foo = 0;
        me->log("s-E;");
        return A1HSM_TRAN(s11);

    case HSM_EVT_TERM:
        me->log("s->TERM");
        return A1HSM_HANDLED();

    default:
        break;
    }
    return A1HSM_SUPER(a1hsm_top);
}

static enum a1hsmrc s1(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s1-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s1-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        return A1HSM_TRAN(s11);

    case A1HSM_EVT_EXIT:
        me->log("s1-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        return A1HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        me->log("s1-I;");
        return A1HSM_HANDLED();

    case HSM_EVT_C:
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        me->log("s1-C;");
        return A1HSM_TRAN(s2);

    case HSM_EVT_F:
        me->log("s1-F;");
        return A1HSM_TRAN(s211);

    case HSM_EVT_A:
        me->log("s1-A;");
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        return A1HSM_TRAN(s1);

    case HSM_EVT_B:
        me->log("s1-B;");
        return A1HSM_TRAN(s11);

    case HSM_EVT_D:
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s1-D;");
            return A1HSM_TRAN(s);
        }
        return A1HSM_HANDLED();

    default:
        break;
    }
    return A1HSM_SUPER(s);
}

static enum a1hsmrc s11(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s11-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_EXIT:
        me->log("s11-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s11-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        return A1HSM_HANDLED();

    case HSM_EVT_G:
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        me->log("s11-G;");
        return A1HSM_TRAN(s211);

    case HSM_EVT_H:
        me->log("s11-H;");
        return A1HSM_TRAN(s);

    case HSM_EVT_D:
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s11)));
        if (me->foo) {
            me->foo = 0;
            me->log("s11-D;");
            return A1HSM_TRAN(s1);
        }
        break;

    default:
        break;
    }
    return A1HSM_SUPER(s1);
}

static enum a1hsmrc s2(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s2-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s2-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        return A1HSM_TRAN(s211);

    case A1HSM_EVT_EXIT:
        me->log("s2-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        return A1HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s211)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s2-I;");
            return A1HSM_HANDLED();
        }
        break;

    case HSM_EVT_F:
        me->log("s2-F;");
        return A1HSM_TRAN(s11);

    case HSM_EVT_C:
        me->log("s2-C;");
        return A1HSM_TRAN(s1);

    default:
        break;
    }
    return A1HSM_SUPER(s);
}

static enum a1hsmrc s21(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s21-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s21-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        return A1HSM_TRAN(s211);

    case A1HSM_EVT_EXIT:
        me->log("s21-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        return A1HSM_HANDLED();

    case HSM_EVT_A:
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s211)));
        me->log("s21-A;");
        return A1HSM_TRAN(s21);

    case HSM_EVT_B:
        me->log("s21-B;");
        return A1HSM_TRAN(s211);

    case HSM_EVT_G:
        me->log("s21-G;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s211)));
        return A1HSM_TRAN(s1);

    default:
        break;
    }
    return A1HSM_SUPER(s2);
}

static enum a1hsmrc s211(struct test *me, const struct event *event) {
    ASSERT(a1hsm_get_state_instance(&me->hsm) == 0);
    switch (event->id) {
    case A1HSM_EVT_ENTRY:
        me->log("s211-ENTRY;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s211)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_EXIT:
        me->log("s211-EXIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s211)));
        return A1HSM_HANDLED();

    case A1HSM_EVT_INIT:
        me->log("s211-INIT;");
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s211)));
        return A1HSM_HANDLED();

    case HSM_EVT_D:
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));
        ASSERT(!a1hsm_is_in(&me->hsm, &A1HSM_STATE(s1)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s211)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s21)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s)));
        ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(a1hsm_top)));
        me->log("s211-D;");
        return A1HSM_TRAN(s21);

    case HSM_EVT_H:
        me->log("s211-H;");
        return A1HSM_TRAN(s);

    default:
        break;
    }
    return A1HSM_SUPER(s21);
}

void regular_ctor(void (*log)(char *fmt, ...)) {
    struct test *me = &m_test;
    a1hsm_ctor(&me->hsm, &A1HSM_STATE(test_init));
    me->log = log;
}
