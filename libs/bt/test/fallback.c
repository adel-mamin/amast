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
 *  | (HSM top superstate am_hsm_top())  |
 *  |                                    |
 *  | +--------------------------------+ |
 *  | |     *         s1               | |
 *  | |     |                          | |
 *  | | +---v------------------------+ | |
 *  | | |     am_bt_fallback()       | | |
 *  | | |                            | | |
 *  | | |   +--------+  +--------+   | | |
 *  | | |   |  s11   |  |  s12   |   | | |
 *  | | |   +--------+  +--------+   | | |
 *  | | +----------------------------+ | |
 *  | +--------------------------------+ |
 *  +------------------------------------+
 *
 * The am_bt_fallback() unit testing is done with the
 * help of 3 user states: s1, s11 and s12.
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
};

static struct test m_test;

static enum am_hsm_rc s1(struct test *me, const struct am_event *event);
static enum am_hsm_rc s11(struct test *me, const struct am_event *event);
static enum am_hsm_rc s12(struct test *me, const struct am_event *event);

static const struct am_hsm_state substates[] = {
    {.fn = (am_hsm_state_fn)s11}, {.fn = (am_hsm_state_fn)s12}
};

static struct am_bt_fallback m_fallback = {
    .node = {.super = {.fn = (am_hsm_state_fn)s1}},
    .substates = substates,
    .nsubstates = 2,
    .isubstate = 0,
    .init_done = 0
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_fallback, 0);
    }
    case AM_HSM_EVT_ENTRY: {
        TLOG("s1-ENTRY;");
        return AM_HSM_HANDLED();
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
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_fallback);
}

static enum am_hsm_rc s12(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        TLOG("s12-ENTRY;");
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s12-EXIT;");
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_bt_fallback);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    return AM_HSM_TRAN(s1);
}

static enum am_hsm_rc sinit2(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit2-INIT;");
    return AM_HSM_TRAN(s12);
}

static void test_ctor(struct am_hsm_state *init) {
    am_bt_ctor();

    struct test *me = &m_test;
    m_fallback.init_done = 0;
    am_bt_add_fallback(&m_fallback, /*num=*/1);
    static struct am_bt_cfg cfg;
    cfg.hsm = &me->hsm;
    cfg.post = test_event_post;
    am_bt_add_cfg(&cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, init);
    am_hsm_init(&me->hsm, /*init_event=*/NULL);
}

static void test_failure(void) {
    test_ctor(&AM_HSM_STATE(sinit));

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_failure);
    test_event_post(&me->hsm, &am_bt_evt_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-EXIT;s12-ENTRY;"
        "s1-BT_FAILURE;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

static void test_jump_second(void) {
    test_ctor(&AM_HSM_STATE(sinit2));

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {"sinit2-INIT;s1-ENTRY;s12-ENTRY;s1-BT_FAILURE;"};
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

static void test_success_first(void) {
    test_ctor(&AM_HSM_STATE(sinit));

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_success);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-ENTRY;s1-INIT;s11-ENTRY;s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

static void test_success_second(void) {
    test_ctor(&AM_HSM_STATE(sinit));

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_failure);
    test_event_post(&me->hsm, &am_bt_evt_success);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-EXIT;s12-ENTRY;"
        "s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

int main(void) {
    test_failure();
    test_jump_second();
    test_success_first();
    test_success_second();
    return 0;
}
