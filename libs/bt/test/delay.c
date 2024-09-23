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
 *  | | | am_bt_delay  | | |
 *  | | |              | | |
 *  | | |  +--------+  | | |
 *  | | |  |  s11   |  | | |
 *  | | |  +--------+  | | |
 *  | | +--------------+ | |
 *  | +------------------+ |
 *  +----------------------+
 *
 * The am_bt_delay() unit testing is done with the
 * help of 2 user states: s1 and s11.
 * s11 runs once after a timeout and returns failure.
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

static struct am_bt_delay m_delay = {
    .node = {.super = {.fn = (am_hsm_state_fn)s1}},
    .substate = {.fn = (am_hsm_state_fn)s11},
    .delay_ticks = 2,
    .domain = 0
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_delay);
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
    return AM_HSM_SUPER(am_bt_delay);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    return AM_HSM_TRAN(s1);
}

static void post_cb(void *owner, const struct am_event *event) {
    (void)event;
    AM_ASSERT(owner == &m_test);
    AM_ASSERT(AM_BT_EVT_DELAY == event->id);
    TLOG("am_bt_delay-DELAY;");
    am_hsm_dispatch(owner, event);
}

static void test_ctor(void) {
    am_bt_ctor();

    struct am_timer_cfg timer_cfg = {
        .post = post_cb, .publish = NULL, .update = NULL
    };
    am_timer_ctor(&timer_cfg);

    struct test *me = &m_test;
    am_bt_add_delay(&m_delay, /*num=*/1);

    static struct am_bt_cfg bt_cfg;
    bt_cfg.hsm = &me->hsm;
    bt_cfg.post = test_event_post;
    am_bt_add_cfg(&bt_cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);
}

static void test_failure(void) {
    test_ctor();

    while (am_timer_any_armed(/*domain=*/0)) {
        am_timer_tick(/*domain=*/0);
    }

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;am_bt_delay-DELAY;s11-ENTRY;s1-BT_FAILURE;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

static void test_success(void) {
    test_ctor();

    while (am_timer_any_armed(/*domain=*/0)) {
        am_timer_tick(/*domain=*/0);
    }

    struct test *me = &m_test;
    test_event_post(&me->hsm, &am_bt_evt_success);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;am_bt_delay-DELAY;s11-ENTRY;s1-BT_SUCCESS;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
}

int main(void) {
    test_failure();
    test_success();
    return 0;
}
