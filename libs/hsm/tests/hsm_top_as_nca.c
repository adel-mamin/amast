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

#include <stdio.h>

#include "common/macros.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "common.h"

struct test_nca {
    struct am_hsm hsm;
};

static struct test_nca m_test_nca;

/* Test am_hsm_top() as NCA. */

static enum am_hsm_rc nca_s11(
    struct test_nca *me, const struct am_event *event
);
static enum am_hsm_rc nca_s2(struct test_nca *me, const struct am_event *event);

static enum am_hsm_rc nca_s1(
    struct test_nca *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_INIT:
        return AM_HSM_TRAN(nca_s11);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc nca_s11(
    struct test_nca *me, const struct am_event *event
) {
    switch (event->id) {
    case HSM_EVT_A:
        return AM_HSM_TRAN(nca_s2);
    default:
        break;
    }
    return AM_HSM_SUPER(nca_s1);
}

static enum am_hsm_rc nca_s2(
    struct test_nca *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc nca_init(
    struct test_nca *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(nca_s1);
}

static void test_am_hsm_top_as_nca(void) {
    struct test_nca *me = &m_test_nca;
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(nca_init));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(nca_s11)));

    static const struct am_event event = {.id = HSM_EVT_A};
    am_hsm_dispatch(&me->hsm, &event);
    AM_ASSERT(am_hsm_is_in(&me->hsm, AM_HSM_STATE_CTOR(nca_s2)));
}

int main(void) {
    test_am_hsm_top_as_nca();

    return 0;
}
