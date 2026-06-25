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

#include <stddef.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "hsm/hsm.h"

#define HSM_EVT_ON (AM_EVT_USER)
#define HSM_EVT_OFF (AM_EVT_USER + 1)
#define HSM_EVT_OPEN (AM_EVT_USER + 2)
#define HSM_EVT_CLOSE (AM_EVT_USER + 3)

struct oven_hsm {
    struct am_hsm hsm;
    struct am_hsm_state history;
};

static struct oven_hsm m_oven_hsm;

/* test transition to HSM history */

static enum am_rc oven_hsm_open(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc oven_hsm_closed(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc oven_hsm_on(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc oven_hsm_off(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc oven_hsm_open(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct oven_hsm* me = AM_CONTAINER_OF(hsm, struct oven_hsm, hsm);
    switch (event->id) {
    case HSM_EVT_OFF:
        me->history = am_hsm_state_make(oven_hsm_off);
        return am_hsm_handled(hsm);

    case HSM_EVT_CLOSE:
        return am_hsm_tran(hsm, oven_hsm_closed);

    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc oven_hsm_closed(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct oven_hsm* me = AM_CONTAINER_OF(hsm, struct oven_hsm, hsm);
    switch (event->id) {
    case AM_EVT_INIT:
        return am_hsm_tran(hsm, me->history.fn);

    case HSM_EVT_OPEN:
        return am_hsm_tran(hsm, oven_hsm_open);

    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc oven_hsm_on(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct oven_hsm* me = AM_CONTAINER_OF(hsm, struct oven_hsm, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->history = am_hsm_get_state(hsm);
        return am_hsm_handled(hsm);

    case HSM_EVT_OFF:
        return am_hsm_tran(hsm, oven_hsm_off);

    default:
        break;
    }
    return am_hsm_super(hsm, oven_hsm_closed);
}

static enum am_rc oven_hsm_off(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct oven_hsm* me = AM_CONTAINER_OF(hsm, struct oven_hsm, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->history = am_hsm_get_state(hsm);
        return am_hsm_handled(hsm);

    case HSM_EVT_ON:
        return am_hsm_tran(hsm, oven_hsm_on);

    default:
        break;
    }
    return am_hsm_super(hsm, oven_hsm_closed);
}

static enum am_rc oven_hsm_init(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;

    struct oven_hsm* me = AM_CONTAINER_OF(hsm, struct oven_hsm, hsm);
    me->history = am_hsm_state_make(oven_hsm_off);
    return am_hsm_tran(hsm, oven_hsm_closed);
}

static void test_oven_hsm(void) {
    struct oven_hsm* me = &m_oven_hsm;
    am_hsm_create(&me->hsm, am_hsm_state_make(oven_hsm_init));

    am_hsm_start(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, am_hsm_state_make(oven_hsm_off)));

    am_hsm_dispatch(&me->hsm, &(struct am_event){.id = HSM_EVT_ON});
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, am_hsm_state_make(oven_hsm_on)));

    am_hsm_dispatch(&me->hsm, &(struct am_event){.id = HSM_EVT_OPEN});
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, am_hsm_state_make(oven_hsm_open)));

    am_hsm_dispatch(&me->hsm, &(struct am_event){.id = HSM_EVT_CLOSE});
    AM_ASSERT(am_hsm_state_is_eq(&me->hsm, am_hsm_state_make(oven_hsm_on)));
}

int main(void) {
    test_oven_hsm();
    return 0;
}
