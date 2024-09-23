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
 * The unit-tested topology:
 *
 *  +------------------------------------+
 *  |              hsm_top               |
 *  |  (HSM top superstate am_hsm_top()) |
 *  |                                    |
 *  | +--------------------------------+ |
 *  | |     *  s1                      | |
 *  | |     |                          | |
 *  | | +---v------------------------+ | |
 *  | | | am_bt_retry_until_success  | | |
 *  | | |                            | | |
 *  | | |  +----------------------+  | | |
 *  | | |  |         s11          |  | | |
 *  | | |  +----------------------+  | | |
 *  | | +----------------------------+ | |
 *  | +--------------------------------+ |
 *  +------------------------------------+
 *
 * The am_bt_retry_until_success() unit testing is done with the
 * help of 2 user states: s1 and s11.
 * s11 runs three times. It returns failure for 2 runs and
 * holds the HSM on 3rd run.
 * am_bt_retry_until_success is configured to run infinite number
 * of attempts.
 */

#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "bt/bt.h"
#include "test_log.h"
#include "test_event.h"

struct test {
    struct am_hsm hsm;
    int cnt;
    unsigned infinite : 1;
};

static struct test m_test;

const struct am_event am_evt_user = {.id = AM_EVT_USER};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event);
static enum am_hsm_rc s11(struct test *me, const struct am_event *event);

static struct am_bt_retry_until_success m_retry_until_success = {
    .node = {.super = {.fn = (am_hsm_state_fn)s1}},
    .substate = {.fn = (am_hsm_state_fn)s11},
    .attempts_total = -1 /* infinite number of attempts */
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_retry_until_success);
    }
    case AM_BT_EVT_SUCCESS: {
        TLOG("s1-BT_SUCCESS;");
        return AM_HSM_HANDLED();
    }
    case AM_BT_EVT_FAILURE: {
        TLOG("s1-BT_FAILURE;");
        return AM_HSM_HANDLED();
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
        const struct am_event *e;
        if (me->infinite) {
            e = (2 == me->cnt) ? &am_evt_user : &am_bt_evt_failure;
        } else {
            e = me->cnt ? &am_bt_evt_success : &am_bt_evt_failure;
        }
        test_event_post(&me->hsm, e);
        ++me->cnt;
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s11-EXIT;");
        return AM_HSM_HANDLED();
    }
    case AM_EVT_USER: {
        TLOG("s11-USER;");
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_retry_until_success);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    me->cnt = 0;
    return AM_HSM_TRAN(s1);
}

static void test_ctor(bool infinite) {
    am_bt_ctor();

    struct test *me = &m_test;
    me->infinite = infinite;
    am_bt_add_retry_until_success(&m_retry_until_success, /*num=*/1);
    struct am_bt_cfg cfg = {.hsm = &me->hsm, .post = test_event_post};
    am_bt_add_cfg(&cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);
}

static void test_infinite(void) {
    test_ctor(/*infinite=*/true);

    struct test *me = &m_test;
    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s11-EXIT;s11-ENTRY;s11-EXIT;s11-ENTRY;"
        "s11-USER;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

static void test_limited(void) {
    test_ctor(/*infinite=*/false);

    struct test *me = &m_test;
    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s11-EXIT;s11-ENTRY;s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

int main(void) {
    test_infinite();
    test_limited();
    return 0;
}
