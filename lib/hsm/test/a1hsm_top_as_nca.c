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
    struct a1hsm hsm;
};

static struct test m_test;

/* Test a1hsm_top() as NCA. */

static enum a1hsmrc s11(struct test *me, const struct event *event);
static enum a1hsmrc s2(struct test *me, const struct event *event);

static enum a1hsmrc s1(struct test *me, const struct event *event) {
    switch (event->id) {
    case A1HSM_EVT_INIT:
        return A1HSM_TRAN(s11);
    default:
        break;
    }
    return A1HSM_SUPER(a1hsm_top);
}

static enum a1hsmrc s11(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        return A1HSM_TRAN(s2);
    default:
        break;
    }
    return A1HSM_SUPER(s1);
}

static enum a1hsmrc s2(struct test *me, const struct event *event) {
    (void)event;
    return A1HSM_SUPER(a1hsm_top);
}

static enum a1hsmrc sinit(struct test *me, const struct event *event) {
    (void)event;
    return A1HSM_TRAN(s1);
}

static void test_a1hsm_top_as_nca(void) {
    struct test *me = &m_test;
    a1hsm_ctor(&me->hsm, &A1HSM_STATE(sinit));

    a1hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s11)));

    static const struct event E = {.id = HSM_EVT_A};
    a1hsm_dispatch(&me->hsm, &E);
    ASSERT(a1hsm_is_in(&me->hsm, &A1HSM_STATE(s2)));
}

int main(void) {
    test_a1hsm_top_as_nca();

    return 0;
}
