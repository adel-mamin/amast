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
 *  +----------------------+
 *  |       hsm_top        |
 *  | (HSM top superstate  |
 *  |     am_hsm_top())    |
 *  |                      |
 *  | +------------------+ |
 *  | |     *  s1        | |
 *  | |     |            | |
 *  | | +---v----------+ | |
 *  | | | am_bt_count  | | |
 *  | | |              | | |
 *  | | |  +--------+  | | |
 *  | | |  |  s11   |  | | |
 *  | | |  +--------+  | | |
 *  | | +--------------+ | |
 *  | +------------------+ |
 *  +----------------------+
 *
 * The am_bt_count() unit testing is done with the
 * help of 2 user states: s1 and s11.
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

#define AM_TEST_EVT_U1_FAILURE AM_EVT_USER
#define AM_TEST_EVT_U1_SUCCESS (AM_EVT_USER + 1)
#define AM_TEST_EVT_U2_FAILURE (AM_EVT_USER + 2)
#define AM_TEST_EVT_U2_SUCCESS (AM_EVT_USER + 3)

const struct am_event am_test_evt_u1_failure = {.id = AM_TEST_EVT_U1_FAILURE};
const struct am_event am_test_evt_u1_success = {.id = AM_TEST_EVT_U1_SUCCESS};
const struct am_event am_test_evt_u2_failure = {.id = AM_TEST_EVT_U2_FAILURE};
const struct am_event am_test_evt_u2_success = {.id = AM_TEST_EVT_U2_SUCCESS};

struct test {
    struct am_hsm hsm;
};

static struct test m_test;

static enum am_hsm_rc s1(struct test *me, const struct am_event *event);
static enum am_hsm_rc s11(struct test *me, const struct am_event *event);

static struct am_bt_count m_count = {
    .node = {.super = {.fn = (am_hsm_state_fn)s1}},
    .substate = {.fn = (am_hsm_state_fn)s11},
    .ntotal = 2,
    .success_min = 1
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_count, 0);
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
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s11-EXIT;");
        return AM_HSM_HANDLED();
    }
    case AM_TEST_EVT_U1_FAILURE:
    case AM_TEST_EVT_U2_FAILURE: {
        test_event_post(&me->hsm, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    case AM_TEST_EVT_U1_SUCCESS:
    case AM_TEST_EVT_U2_SUCCESS: {
        test_event_post(&me->hsm, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_count);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    return AM_HSM_TRAN(s1);
}

static void test_ctor(int success_min) {
    am_bt_ctor();

    struct test *me = &m_test;
    m_count.success_min = success_min;
    am_bt_add_count(&m_count, /*num=*/1);
    static struct am_bt_cfg cfg;
    cfg.hsm = &me->hsm;
    cfg.post = test_event_post;
    am_bt_add_cfg(&cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);
}

/* both users return failure */
static void test_failure(void) {
    test_ctor(/*success_min=*/1);

    struct test *me = &m_test;

    test_event_post(&me->hsm, &am_test_evt_u1_failure);
    test_event_post(&me->hsm, &am_test_evt_u2_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s11-EXIT;s1-BT_FAILURE;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

/*
 * Only one user returns failure and BT count node
 * recognizes the failure as it expects 2 users to succeed.
 * If one user already failed, then the count node should
 * report failure.
 */
static void test_failure_early(void) {
    test_ctor(/*success_min=*/2);

    struct test *me = &m_test;

    test_event_post(&me->hsm, &am_test_evt_u1_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s11-EXIT;s1-BT_FAILURE;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

/* the count node waits for at least one user to succeed */
static void test_success(void) {
    test_ctor(/*success_min=*/1);

    struct test *me = &m_test;

    test_event_post(&me->hsm, &am_test_evt_u1_success);
    test_event_post(&me->hsm, &am_test_evt_u2_success);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s11-ENTRY;s11-EXIT;s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

int main(void) {
    test_failure();
    test_failure_early();
    test_success();
    return 0;
}
