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

#include <stdint.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"
#include "event/event_common.h"
#include "submachine.h"

/**
 * Test state machine with the following topology.
 * Note that s is a substate of hsm_top - the HSM top superstate of
 * am_hsm_top(). It was omitted from the diagram for brevity.
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

struct complex_sm {
    struct am_hsm hsm;
    AM_PRINTF(1, 0) void (*log)(const char* fmt, ...);
};

static struct complex_sm m_complex_sm;

#define SM_0 0
#define SM_1 1
#define SM_2 2

static enum am_rc cs_s(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc cs_s1(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc cs_s11(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc cs_s111(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc cs_s12(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc cs_s121(struct am_hsm* hsm, const struct am_event* event);

static enum am_rc cs_s(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(cs_s)));
        me->log("s/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(cs_s)));
        me->log("s/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(cs_s)));
        me->log("s/%d-INIT;", instance);
        return am_hsm_tran_i(hsm, cs_s111, SM_2);

    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc cs_s1(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s1, instance)));
        me->log("s1/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s1, instance)));
        me->log("s1/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s1, instance)));
        me->log("s1/%d-INIT;", instance);
        return am_hsm_tran_i(hsm, cs_s111, instance);

    case HSM_EVT_A:
        me->log("s1/%d-A;", instance);
        return am_hsm_tran_i(hsm, cs_s1, instance);

    case HSM_EVT_B:
        me->log("s1/%d-B;", instance);
        return am_hsm_tran_i(hsm, cs_s11, instance);

    case HSM_EVT_C:
        me->log("s1/%d-C;", instance);
        return am_hsm_tran_i(hsm, cs_s121, instance);

    case HSM_EVT_H:
        me->log("s1/%d-H;", instance);
        return am_hsm_tran(hsm, cs_s);

    default:
        break;
    }
    static const struct am_hsm_state ss[] = {
        [SM_0] = {.fn = cs_s},
        [SM_1] = {.fn = cs_s1, .instance = SM_0},
        [SM_2] = {.fn = cs_s}
    };
    AM_ASSERT(instance < AM_COUNTOF(ss));
    const struct am_hsm_state* super = &ss[instance];
    return am_hsm_super_i(hsm, super->fn, super->instance);
}

static enum am_rc cs_s11(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s11, instance)));
        me->log("s11/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s11, instance)));
        me->log("s11/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s11, instance)));
        me->log("s11/%d-INIT;", instance);
        return am_hsm_handled(hsm);

    case HSM_EVT_G:
        me->log("s11/%d-G;", instance);
        static const struct am_hsm_state tt[] = {
            [SM_0] = {.fn = cs_s1, .instance = SM_1},
            [SM_1] = {.fn = cs_s1, .instance = SM_2},
            [SM_2] = {.fn = cs_s1, .instance = SM_0}
        };
        AM_ASSERT(instance < AM_COUNTOF(tt));
        const struct am_hsm_state* tran = &tt[instance];

        return am_hsm_tran_i(hsm, tran->fn, tran->instance);

    default:
        break;
    }
    return am_hsm_super_i(hsm, cs_s1, instance);
}

static enum am_rc cs_s111(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s111, instance)));
        me->log("s111/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s111, instance)));
        me->log("s111/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s111, instance)));
        me->log("s111/%d-INIT;", instance);
        return am_hsm_handled(hsm);

    case HSM_EVT_D:
        me->log("s111/%d-D;", instance);
        return am_hsm_tran_i(hsm, cs_s12, instance);

    default:
        break;
    }
    return am_hsm_super_i(hsm, cs_s11, instance);
}

static enum am_rc cs_s12(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s12, instance)));
        me->log("s12/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s12, instance)));
        me->log("s12/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s12, instance)));
        me->log("s12/%d-INIT;", instance);
        return am_hsm_tran_i(hsm, cs_s121, instance);

    case HSM_EVT_F:
        me->log("s12/%d-F;", instance);
        static const struct am_hsm_state tt[] = {
            [SM_0] = {.fn = cs_s12, .instance = SM_1},
            [SM_1] = {.fn = cs_s12, .instance = SM_2},
            [SM_2] = {.fn = cs_s12, .instance = SM_0}
        };
        AM_ASSERT(instance < AM_COUNTOF(tt));
        const struct am_hsm_state* tran = &tt[instance];

        return am_hsm_tran_i(hsm, tran->fn, tran->instance);

    default:
        break;
    }
    return am_hsm_super_i(hsm, cs_s1, instance);
}

static enum am_rc cs_s121(struct am_hsm* hsm, const struct am_event* event) {
    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s121, instance)));
        me->log("s121/%d-ENTRY;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s121, instance)));
        me->log("s121/%d-EXIT;", instance);
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make_i(cs_s121, instance)));
        me->log("s121/%d-INIT;", instance);
        return am_hsm_handled(hsm);

    case HSM_EVT_E:
        me->log("s121/%d-E;", instance);
        return am_hsm_tran_i(hsm, cs_s12, instance);

    default:
        break;
    }
    return am_hsm_super_i(hsm, cs_s12, instance);
}

static enum am_rc complex_sm_init(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;

    struct complex_sm* me = AM_CONTAINER_OF(hsm, struct complex_sm, hsm);
    me->log("top/%d-INIT;", am_hsm_get_instance(hsm));

    return am_hsm_tran_i(hsm, cs_s1, SM_1);
}

void complex_sm_create(AM_PRINTF(1, 0) void (*log)(const char* fmt, ...)) {
    struct complex_sm* me = &m_complex_sm;
    am_hsm_init(&me->hsm, am_hsm_state_make(complex_sm_init));
    me->log = log;
}

struct am_hsm* complex_get_obj(void) { return &m_complex_sm.hsm; }
