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

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
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
    void (*log)(const char* fmt, ...);
};

static struct regular m_regular;

static enum am_rc r_s2(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc r_s21(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc r_s211(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc r_s1(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc r_s11(struct am_hsm* hsm, const struct am_event* event);

static enum am_rc regular_initial(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;

    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    me->foo = 0;
    me->log("top-INIT;");
    return am_hsm_tran(hsm, r_s2);
}

static enum am_rc r_s(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        me->log("s-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));

        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));

        return am_hsm_handled(hsm);
    }
    case AM_EVT_INIT: {
        me->log("s-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));

        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));

        return am_hsm_tran(hsm, r_s11);
    }
    case AM_EVT_EXIT: {
        me->log("s-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));

        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));

        return am_hsm_handled(hsm);
    }
    case HSM_EVT_I: {
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s211)));
        if (me->foo) {
            me->foo = 0;
            me->log("s-I;");
            return am_hsm_handled(hsm);
        }
        return am_hsm_handled(hsm);
    }
    case HSM_EVT_E:
        AM_ASSERT(
            (am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s211))) ||
            (am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)))
        );
        me->foo = 0;
        me->log("s-E;");
        return am_hsm_tran(hsm, r_s11);

    case HSM_EVT_TERM:
        me->log("s->TERM");
        return am_hsm_handled(hsm);

    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc r_s1(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        me->log("s1-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));
        return am_hsm_handled(hsm);
    }
    case AM_EVT_INIT: {
        me->log("s1-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));
        return am_hsm_tran(hsm, r_s11);
    }
    case AM_EVT_EXIT: {
        me->log("s1-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));
        return am_hsm_handled(hsm);
    }
    case HSM_EVT_I:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        me->log("s1-I;");
        return am_hsm_handled(hsm);

    case HSM_EVT_C:
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        me->log("s1-C;");
        return am_hsm_tran(hsm, r_s2);

    case HSM_EVT_F:
        me->log("s1-F;");
        return am_hsm_tran(hsm, r_s211);

    case HSM_EVT_A:
        me->log("s1-A;");
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        return am_hsm_tran(hsm, r_s1);

    case HSM_EVT_B:
        me->log("s1-B;");
        return am_hsm_tran(hsm, r_s11);

    case HSM_EVT_D:
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s1-D;");
            return am_hsm_tran(hsm, r_s);
        }
        return am_hsm_handled(hsm);

    default:
        break;
    }
    return am_hsm_super(hsm, r_s);
}

static enum am_rc r_s11(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s11-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        me->log("s11-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        me->log("s11-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        return am_hsm_handled(hsm);

    case HSM_EVT_G:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        me->log("s11-G;");
        return am_hsm_tran(hsm, r_s211);

    case HSM_EVT_H:
        me->log("s11-H;");
        return am_hsm_tran(hsm, r_s);

    case HSM_EVT_D:
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s11)));
        if (me->foo) {
            me->foo = 0;
            me->log("s11-D;");
            return am_hsm_tran(hsm, r_s1);
        }
        break;

    default:
        break;
    }
    return am_hsm_super(hsm, r_s1);
}

static enum am_rc r_s2(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s2-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        me->log("s2-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        return am_hsm_tran(hsm, r_s211);

    case AM_EVT_EXIT:
        me->log("s2-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        return am_hsm_handled(hsm);

    case HSM_EVT_I:
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s211)));
        if (!me->foo) {
            me->foo = 1;
            me->log("s2-I;");
            return am_hsm_handled(hsm);
        }
        break;

    case HSM_EVT_F:
        me->log("s2-F;");
        return am_hsm_tran(hsm, r_s11);

    case HSM_EVT_C:
        me->log("s2-C;");
        return am_hsm_tran(hsm, r_s1);

    default:
        break;
    }
    return am_hsm_super(hsm, r_s);
}

static enum am_rc r_s21(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s21-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        me->log("s21-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        return am_hsm_tran(hsm, r_s211);

    case AM_EVT_EXIT:
        me->log("s21-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        return am_hsm_handled(hsm);

    case HSM_EVT_A: {
        me->log("s21-A;");
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s211)));

        struct am_hsm_state state = am_hsm_get_state(hsm);
        AM_ASSERT(am_hsm_state_is_eq(hsm, state));

        return am_hsm_tran(hsm, r_s21);
    }
    case HSM_EVT_B:
        me->log("s21-B;");
        return am_hsm_tran(hsm, r_s211);

    case HSM_EVT_G:
        me->log("s21-G;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        AM_ASSERT(am_hsm_state_is_eq(hsm, am_hsm_state_make(r_s211)));
        return am_hsm_tran(hsm, r_s1);

    default:
        break;
    }
    return am_hsm_super(hsm, r_s2);
}

static enum am_rc r_s211(struct am_hsm* hsm, const struct am_event* event) {
    struct regular* me = AM_CONTAINER_OF(hsm, struct regular, hsm);
    AM_ASSERT(am_hsm_get_instance(hsm) == 0);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s211-ENTRY;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s211)));
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        me->log("s211-EXIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s211)));
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        me->log("s211-INIT;");
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s211)));
        return am_hsm_handled(hsm);

    case HSM_EVT_D:
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s11)));
        AM_ASSERT(!am_hsm_is_in(hsm, am_hsm_state_make(r_s1)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s211)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s21)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s2)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(r_s)));
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(am_hsm_top)));
        me->log("s211-D;");
        return am_hsm_tran(hsm, r_s21);

    case HSM_EVT_H:
        me->log("s211-H;");
        return am_hsm_tran(hsm, r_s);

    default:
        break;
    }
    return am_hsm_super(hsm, r_s21);
}

void regular_init(void (*log)(const char* fmt, ...)) {
    struct regular* me = &m_regular;
    am_hsm_init(&me->hsm, am_hsm_state_make(regular_initial));
    me->log = log;
}

struct am_hsm* regular_get_obj(void) { return &m_regular.hsm; }
