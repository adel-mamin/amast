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

/**
 *  The unit-tested topology:
 *
 *  +----------------------------------------------------------+
 *  |                         hsm_top                          |
 *  |            (HSM top superstate am_hsm_top())             |
 *  |                                                          |
 *  | +------------------------------------------------------+ |
 *  | |     *                    s1                          | |
 *  | |     |                                                | |
 *  | | +---v-------------------+  +-----------------------+ | |
 *  | | | am_bt_force_success/0 |  | am_bt_force_success/1 | | |
 *  | | |                       |  |                       | | |
 *  | | |   +--------------+    |  |   +--------------+    | | |
 *  | | |   |      s11     |    |  |   |     s12      |    | | |
 *  | | |   +--------------+    |  |   +--------------+    | | |
 *  | | +-----------------------+  +-----------------------+ | |
 *  | +------------------------------------------------------+ |
 *  +----------------------------------------------------------+
 *
 * The am_bt_force_success() unit testing is done with the
 * help of 3 user states: s1, s11 and s12.
 * s1 owns the behavior tree, s11 always returns success and
 * s12 always returns failure.
 */

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "bt/bt.h"
#include "test_log.h"
#include "test_event.h"

struct test {
    struct am_hsm hsm;
};

static struct test m_test;

static enum am_hsm_rc s1(struct test *me, const struct am_event *event);
static enum am_hsm_rc s11(struct test *me, const struct am_event *event);
static enum am_hsm_rc s12(struct test *me, const struct am_event *event);

#define BT_FORCE_SUCCESS_0 0
#define BT_FORCE_SUCCESS_1 1
static struct am_bt_force_success m_force_success[] = {
    [BT_FORCE_SUCCESS_0] =
        {.node = {.super = {.fn = (am_hsm_state_fn)s1}},
         .substate = {.fn = (am_hsm_state_fn)s11}},
    [BT_FORCE_SUCCESS_1] =
        {.node = {.super = {.fn = (am_hsm_state_fn)s1}},
         .substate = {.fn = (am_hsm_state_fn)s12}}
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_force_success, BT_FORCE_SUCCESS_0);
    }
    case AM_BT_EVT_SUCCESS: {
        TLOG("s1-BT_SUCCESS;");
        if (am_hsm_is_in(
                &me->hsm, &AM_HSM_STATE(am_bt_force_success, BT_FORCE_SUCCESS_0)
            )) {
            return AM_HSM_TRAN(am_bt_force_success, BT_FORCE_SUCCESS_1);
        }
        break;
    }
    case AM_BT_EVT_FAILURE: {
        AM_ASSERT(0);
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s11(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        TLOG("s11-ENTRY;");
        test_event_post(&me->hsm, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s11-EXIT;");
        test_event_post(&me->hsm, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_force_success, BT_FORCE_SUCCESS_0);
}

static enum am_hsm_rc s12(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        TLOG("s12-ENTRY;");
        test_event_post(&me->hsm, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s12-EXIT;");
        test_event_post(&me->hsm, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_force_success, BT_FORCE_SUCCESS_1);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    return AM_HSM_TRAN(s1);
}

int main(void) {
    am_bt_ctor();

    struct test *me = &m_test;
    am_bt_add_force_success(m_force_success, /*num=*/2);
    struct am_bt_cfg cfg = {.hsm = &me->hsm, .post = test_event_post};
    am_bt_add_cfg(&cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s1-BT_SUCCESS;"
        "s11-EXIT;s12-ENTRY;s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));

    return 0;
}
