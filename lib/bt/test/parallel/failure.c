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
 *  | | |       am_bt_parallel       | | |
 *  | | +----------------------------+ | |
 *  | +----+---------------------+-----+ |
 *  |      |                     |       |
 *  +------|---------------------|-------+
 *         |                     |
 *  +------|---------+ +---------|-------+
 *  |      | hsm_top | | hsm_top |       |
 *  | +----v-------+ | | +-------v-----+ |
 *  | |     s2     | | | |      s3     | |
 *  | +------------+ | | +-------------+ |
 *  +----------------+ +-----------------+
 *
 * The am_bt_parallel() unit testing is done with the
 * help of 3 user HSMs: s1, s2 and s3.
 * Both s2 and s3 sub-HSMs return failure.
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

#define AM_TEST_EVT_S2_FAILURE AM_EVT_USER
#define AM_TEST_EVT_S3_FAILURE (AM_EVT_USER + 1)

const struct am_event am_test_evt_s2_failure = {.id = AM_TEST_EVT_S2_FAILURE};
const struct am_event am_test_evt_s3_failure = {.id = AM_TEST_EVT_S3_FAILURE};

struct test {
    struct am_hsm hsm;
};

static struct test m_test;

struct s2_state {
    struct am_hsm hsm;
    struct am_hsm *super;
};

static struct s2_state s2_state;

struct s3_state {
    struct am_hsm hsm;
    struct am_hsm *super;
};

static struct s3_state s3_state;

static enum am_hsm_rc s1(struct test *me, const struct am_event *event);

static enum am_hsm_rc s2(struct s2_state *me, const struct am_event *event);
static enum am_hsm_rc s2_init(
    struct s2_state *me, const struct am_event *event
);

static enum am_hsm_rc s3(struct s3_state *me, const struct am_event *event);
static enum am_hsm_rc s3_init(
    struct s3_state *me, const struct am_event *event
);

void s2_ctor(struct am_hsm *hsm, struct am_hsm *super) {
    struct s2_state *me = (struct s2_state *)hsm;
    me->super = super;
    am_hsm_ctor(hsm, &AM_HSM_STATE(s2_init));
}

void s3_ctor(struct am_hsm *hsm, struct am_hsm *super) {
    struct s3_state *me = (struct s3_state *)hsm;
    me->super = super;
    am_hsm_ctor(hsm, &AM_HSM_STATE(s3_init));
}

static const struct am_bt_subhsm subhsms[] = {
    {.ctor = s2_ctor, .hsm = &s2_state.hsm},
    {.ctor = s3_ctor, .hsm = &s3_state.hsm},
};

static struct am_bt_parallel m_parallel = {
    .node = {.super = {.fn = (am_hsm_state_fn)s1}},
    .subhsms = subhsms,
    .nsubhsms = 2,
    .success_min = 1,
};

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        TLOG("s1-INIT;");
        return AM_HSM_TRAN(am_bt_parallel, 0);
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

static enum am_hsm_rc s2(struct s2_state *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        TLOG("s2-ENTRY;");
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s2-EXIT;");
        return AM_HSM_HANDLED();
    }
    case AM_TEST_EVT_S2_FAILURE:
        test_event_post(&me->hsm, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s2_init(
    struct s2_state *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(s2);
}

static enum am_hsm_rc s3(struct s3_state *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        TLOG("s3-ENTRY;");
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        TLOG("s3-EXIT;");
        return AM_HSM_HANDLED();
    }
    case AM_TEST_EVT_S3_FAILURE:
        test_event_post(&me->hsm, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s3_init(
    struct s3_state *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(s3);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    TLOG("sinit-INIT;");
    return AM_HSM_TRAN(s1);
}

int main(void) {
    am_bt_ctor();

    struct test *me = &m_test;
    am_bt_add_parallel(&m_parallel, /*num=*/1);
    struct am_bt_cfg cfg = {.hsm = &me->hsm, .post = test_event_post};
    am_bt_add_cfg(&cfg);

    test_log_clear();

    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);

    test_event_post(&me->hsm, &am_test_evt_s2_failure);
    test_event_post(&me->hsm, &am_test_evt_s3_failure);

    const struct am_event *event;
    while ((event = test_event_get()) != NULL) {
        am_hsm_dispatch(&me->hsm, event);
    }
    static const char *out = {
        "sinit-INIT;s1-INIT;s2-ENTRY;s3-ENTRY;s2-EXIT;s3-EXIT;s1-BT_FAILURE;"
    };
    AM_ASSERT(0 == strncmp(test_log_get(), out, strlen(out)));
    return 0;
}
