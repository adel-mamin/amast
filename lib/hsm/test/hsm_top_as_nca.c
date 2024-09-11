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

struct test {
    struct am_hsm hsm;
};

static struct test m_test;

/* Test am_hsm_top() as NCA. */

static enum am_hsm_rc s11(struct test *me, const struct am_event *event);
static enum am_hsm_rc s2(struct test *me, const struct am_event *event);

static enum am_hsm_rc s1(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_INIT:
        return AM_HSM_TRAN(s11);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc s11(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        return AM_HSM_TRAN(s2);
    default:
        break;
    }
    return AM_HSM_SUPER(s1);
}

static enum am_hsm_rc s2(struct test *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc sinit(struct test *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(s1);
}

static void test_am_hsm_top_as_nca(void) {
    struct test *me = &m_test;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(sinit));

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s11)));

    static const struct am_event E = {.id = HSM_EVT_A};
    am_hsm_dispatch(&me->hsm, &E);
    AM_ASSERT(am_hsm_is_in(&me->hsm, &AM_HSM_STATE(s2)));
}

int main(void) {
    test_am_hsm_top_as_nca();

    return 0;
}
