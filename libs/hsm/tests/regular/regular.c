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

#include "common/macros.h"
#include "event/event.h"
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

struct regular {
    struct am_hsm hsm;
    int foo;
    void (*log)(const char *fmt, ...);
};

static struct regular m_regular;

struct am_hsm *g_regular = &m_regular.hsm;

static enum am_hsm_rc s2(struct regular *me, const struct am_event *event);
static enum am_hsm_rc s21(struct regular *me, const struct am_event *event);
static enum am_hsm_rc s211(struct regular *me, const struct am_event *event);
static enum am_hsm_rc s1(struct regular *me, const struct am_event *event);
static enum am_hsm_rc s11(struct regular *me, const struct am_event *event);

static enum am_hsm_rc regular_init(
    struct regular *me, const struct am_event *event
) {
    (void)event;

    me->foo = 0;
    me->log("top-INIT;");
    return AM_HSM_TRAN(s2);
}

static enum am_hsm_rc s(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        me->log("s-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));

        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));

        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        me->log("s-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));

        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));

        return AM_HSM_TRAN(s11);
    }
    case AM_HSM_EVT_EXIT: {
        me->log("s-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));

        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));

        return AM_HSM_HANDLED();
    }
    case HSM_EVT_I: {
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s211)));
        if (me->foo) {
            me->foo = 0;
            me->log("s-I;");
            return AM_HSM_HANDLED();
        }
        return AM_HSM_HANDLED();
    }
    case HSM_EVT_E:
        AM_ASSERT(
            (am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s211))) ||
            (am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)))
        );
        me->foo = 0;
        me->log("s-E;");
        return AM_HSM_TRAN(s11);

    case HSM_EVT_TERM:
        me->log("s->TERM");
        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s1(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        me->log("s1-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        me->log("s1-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));
        return AM_HSM_TRAN(s11);
    }
    case AM_HSM_EVT_EXIT: {
        me->log("s1-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));
        return AM_HSM_HANDLED();
    }
    case HSM_EVT_I:
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        me->log("s1-I;");
        return AM_HSM_HANDLED();

    case HSM_EVT_C:
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        me->log("s1-C;");
        return AM_HSM_TRAN(s2);

    case HSM_EVT_F:
        me->log("s1-F;");
        return AM_HSM_TRAN(s211);

    case HSM_EVT_A:
        me->log("s1-A;");
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        return AM_HSM_TRAN(s1);

    case HSM_EVT_B:
        me->log("s1-B;");
        return AM_HSM_TRAN(s11);

    case HSM_EVT_D:
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s1-D;");
            return AM_HSM_TRAN(s);
        }
        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(s);
}

static enum am_hsm_rc s11(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s11-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s11-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s11-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        return AM_HSM_HANDLED();

    case HSM_EVT_G:
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        me->log("s11-G;");
        return AM_HSM_TRAN(s211);

    case HSM_EVT_H:
        me->log("s11-H;");
        return AM_HSM_TRAN(s);

    case HSM_EVT_D:
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s11)));
        if (me->foo) {
            me->foo = 0;
            me->log("s11-D;");
            return AM_HSM_TRAN(s1);
        }
        break;

    default:
        break;
    }
    return AM_HSM_SUPER(s1);
}

static enum am_hsm_rc s2(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s2-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s2-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        return AM_HSM_TRAN(s211);

    case AM_HSM_EVT_EXIT:
        me->log("s2-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        return AM_HSM_HANDLED();

    case HSM_EVT_I:
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s211)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s2-I;");
            return AM_HSM_HANDLED();
        }
        break;

    case HSM_EVT_F:
        me->log("s2-F;");
        return AM_HSM_TRAN(s11);

    case HSM_EVT_C:
        me->log("s2-C;");
        return AM_HSM_TRAN(s1);

    default:
        break;
    }
    return AM_HSM_SUPER(s);
}

static enum am_hsm_rc s21(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s21-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s21-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        return AM_HSM_TRAN(s211);

    case AM_HSM_EVT_EXIT:
        me->log("s21-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        return AM_HSM_HANDLED();

    case HSM_EVT_A: {
        me->log("s21-A;");
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s211)));

        struct am_hsm_state state = am_hsm_get_active_state(&me->hsm);
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &state));

        return AM_HSM_TRAN(s21);
    }
    case HSM_EVT_B:
        me->log("s21-B;");
        return AM_HSM_TRAN(s211);

    case HSM_EVT_G:
        me->log("s21-G;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        AM_ASSERT(am_hsm_active_state_is_eq(&me->hsm, &AM_HSM_STATE(s211)));
        return AM_HSM_TRAN(s1);

    default:
        break;
    }
    return AM_HSM_SUPER(s2);
}

static enum am_hsm_rc s211(struct regular *me, const struct am_event *event) {
    AM_ASSERT(am_hsm_get_own_instance(&me->hsm) == 0);
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s211-ENTRY;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s211)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s211-EXIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s211)));
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s211-INIT;");
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s211)));
        return AM_HSM_HANDLED();

    case HSM_EVT_D:
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));
        AM_ASSERT(!am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s211)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s21)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
        AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(am_hsm_top)));
        me->log("s211-D;");
        return AM_HSM_TRAN(s21);

    case HSM_EVT_H:
        me->log("s211-H;");
        return AM_HSM_TRAN(s);

    default:
        break;
    }
    return AM_HSM_SUPER(s21);
}

void regular_ctor(void (*log)(const char *fmt, ...)) {
    struct regular *me = &m_regular;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(regular_init));
    me->log = log;
}
