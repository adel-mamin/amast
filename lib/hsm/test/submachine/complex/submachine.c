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
#include "submachine.h"

/**
 * Test state machine with the following topology.
 * Note that s is a substate of hsm_top - the HSM top superstate of
 * am_hsm_top(). It was ommitted from the diagram for brevity.
 *
 * +---------------------------------------------------------------------------+
 * |                                   s                                       |
 * | +------------------------------------+ +--------------------------------+ |
 * | |       *        s1/0                | |     +        s1/2              | |
 * | |   +---|--------+  +------------+   | | +---|--------+  +------------+ | |
 * | |   |   | s11/0  |  |   s12/0    |   | | |   | s11/2  |  |   s12/2    | | |
 * | |   |   |        |  |   *        |   | | |   |        |  |   *        | | |
 * | |   |   |        |  |   |        |   | | |   |        |  |   |        | | |
 * | |   | +-v------+ |  | +-v------+ |   | | | +-v------+ |  | +-v------+ | | |
 * | |   | | s111/0 | |  | | s121/0 | |   | | | | s111/2 | |  | | s121/2 | | | |
 * | |   | +--------+ |  | +--------+ |   | | | +--------+ |  | +--------+ | | |
 * | |   +------------+  +------------+   | | +------------+  +------------+ | |
 * | | +--------------------------------+ | +---------------^----------------+ |
 * | | |     *        s1/1              | |                 |                  |
 * | | | +---|--------+  +------------+ | |                 |                  |
 * | | | |   | s11/1  |  |   s12/1    | | |                 *                  |
 * | | | |   |        |  |   *        | | |                                    |
 * | | | |   |        |  |   |        | | |                                    |
 * | | | | +-v------+ |  | +-v------+ | | |                                    |
 * | | | | | s111/1 | |  | | s121/1 | | | |                                    |
 * | | | | +--------+ |  | +--------+ | | |                                    |
 * | | | +------------+  +------------+ | |                                    |
 * | | +---------------^----------------+ |                                    |
 * | +-----------------|------------------+                                    |
 * +-------------------|------------------------------------+------------------+
 *                     |                                    | TERMINATE
 *                     *                                    *
 * [s1, s11, s111, s12, s121] states constitute a submachine:
 *
 *   +---------------------------------------+
 *   |       +           s1                  |
 *   |  +----|---------+   +--------------+  |
 *   |  |    | s11     |   |     s12      |  +--+
 *   |  |    |         |   |    *         |  |  | A
 *   |  |    |         |   |    |         |  <--+
 *   |  |  +-v------+  |   |  +-v------+  |  |
 *   |  |  |        |  | D |  |        |  |  | F   [SM_0]->s12/SM_1
 *   |  |  |  s111  +------>  |  s121  |  +------> [SM_1]->s12/SM_2
 *   |  |  |        |  |   |  |        |  |  |     [SM_2]->s12/SM_0
 *   |  |  +--------+  |   |  +-^---+--+  |  |
 *   |  |              |   |    |   | E   |  | H
 *   |  +----^----+----+   +----|---v-----+  +---> s
 *   |       | B  | G           | C          |
 *   +-------+----|-------------+------------+
 *                | [SM_0]->s1/SM_1
 *                v [SM_1]->s1/SM_2
 *                  [SM_2]->s1/SM_0
 *
 * The test instantiates three instances of the submachine:
 * SM_0(0), SM_1(1) and SM_2(2).
 */

struct test {
    struct am_hsm hsm;
    void (*log)(char *fmt, ...);
};

static struct test m_test;

struct am_hsm *g_submachine = &m_test.hsm;

#define SM_0 0
#define SM_1 1
#define SM_2 2

static enum am_hsm_rc s(struct test *me, const struct event *event);
static enum am_hsm_rc s1(struct test *me, const struct event *event);
static enum am_hsm_rc s11(struct test *me, const struct event *event);
static enum am_hsm_rc s111(struct test *me, const struct event *event);
static enum am_hsm_rc s12(struct test *me, const struct event *event);
static enum am_hsm_rc s121(struct test *me, const struct event *event);

static enum am_hsm_rc s(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s/%d-INIT;", instance);
        return AM_HSM_TRAN(s111, SM_2);

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s1(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s1, instance)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s1/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s1/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s1/%d-INIT;", instance);
        return AM_HSM_TRAN(s111, instance);

    case HSM_EVT_A:
        me->log("s1/%d-A;", instance);
        return AM_HSM_TRAN(s1, instance);

    case HSM_EVT_B:
        me->log("s1/%d-B;", instance);
        return AM_HSM_TRAN(s11, instance);

    case HSM_EVT_C:
        me->log("s1/%d-C;", instance);
        return AM_HSM_TRAN(s121, instance);

    case HSM_EVT_H:
        me->log("s1/%d-H;", instance);
        return AM_HSM_TRAN(s);

    default:
        break;
    }
    static const struct am_hsm_state ss[] = {
        [SM_0] = {.fn = (am_hsm_state_fn)s},
        [SM_1] = {.fn = (am_hsm_state_fn)s1, .instance = SM_0},
        [SM_2] = {.fn = (am_hsm_state_fn)s}
    };
    AM_ASSERT(instance < AM_COUNTOF(ss));
    const struct am_hsm_state *super = &ss[instance];
    return AM_HSM_SUPER(super->fn, super->instance);
}

static enum am_hsm_rc s11(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11, instance)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s11/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s11/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s11/%d-INIT;", instance);
        return AM_HSM_HANDLED();

    case HSM_EVT_G:
        me->log("s11/%d-G;", instance);
        static const struct am_hsm_state tt[] = {
            [SM_0] = {.fn = (am_hsm_state_fn)s1, .instance = SM_1},
            [SM_1] = {.fn = (am_hsm_state_fn)s1, .instance = SM_2},
            [SM_2] = {.fn = (am_hsm_state_fn)s1, .instance = SM_0}
        };
        AM_ASSERT(instance < AM_COUNTOF(tt));
        const struct am_hsm_state *tran = &tt[instance];

        return AM_HSM_TRAN(tran->fn, tran->instance);

    default:
        break;
    }
    return AM_HSM_SUPER(s1, instance);
}

static enum am_hsm_rc s111(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s111, instance)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s111/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s111/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s111/%d-INIT;", instance);
        return AM_HSM_HANDLED();

    case HSM_EVT_D:
        me->log("s111/%d-D;", instance);
        return AM_HSM_TRAN(s12, instance);

    default:
        break;
    }
    return AM_HSM_SUPER(s11, instance);
}

static enum am_hsm_rc s12(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s12, instance)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s12/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s12/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s12/%d-INIT;", instance);
        return AM_HSM_TRAN(s121, instance);

    case HSM_EVT_F:
        me->log("s12/%d-F;", instance);
        static const struct am_hsm_state tt[] = {
            [SM_0] = {.fn = (am_hsm_state_fn)s12, .instance = SM_1},
            [SM_1] = {.fn = (am_hsm_state_fn)s12, .instance = SM_2},
            [SM_2] = {.fn = (am_hsm_state_fn)s12, .instance = SM_0}
        };
        AM_ASSERT(instance < AM_COUNTOF(tt));
        const struct am_hsm_state *tran = &tt[instance];

        return AM_HSM_TRAN(tran->fn, tran->instance);

    default:
        break;
    }
    return AM_HSM_SUPER(s1, instance);
}

static enum am_hsm_rc s121(struct test *me, const struct event *event) {
    const int instance = am_hsm_get_state_instance(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s121, instance)));
    switch (event->id) {
    case AM_HSM_EVT_ENTRY:
        me->log("s121/%d-ENTRY;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_EXIT:
        me->log("s121/%d-EXIT;", instance);
        return AM_HSM_HANDLED();

    case AM_HSM_EVT_INIT:
        me->log("s121/%d-INIT;", instance);
        return AM_HSM_HANDLED();

    case HSM_EVT_E:
        me->log("s121/%d-E;", instance);
        return AM_HSM_TRAN(s12, instance);

    default:
        break;
    }
    return AM_HSM_SUPER(s12, instance);
}

static enum am_hsm_rc sinit(struct test *me, const struct event *event) {
    (void)event;

    me->log("top/%d-INIT;", am_hsm_get_state_instance(&me->hsm));

    return AM_HSM_TRAN(s1, SM_1);
}

void submachine_ctor(void (*log)(char *fmt, ...)) {
    struct test *me = &m_test;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    me->log = log;
}
