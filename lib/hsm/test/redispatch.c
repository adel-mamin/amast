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

#include <stddef.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"

struct redispatch {
    struct a1hsm hsm;
    int foo;
};

static struct redispatch m_redispatch;

/* test A1HSM_TRAN_REDISPATCH() */

static enum a1hsmrc s1(struct redispatch *me, const struct event *event);
static enum a1hsmrc s2(struct redispatch *me, const struct event *event);

static enum a1hsmrc s1(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        return A1HSM_TRAN_REDISPATCH(s2);
    default:
        break;
    }
    return A1HSM_SUPER(a1hsm_top);
}

static enum a1hsmrc s2(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        me->foo = 1;
        return A1HSM_HANDLED();
    default:
        break;
    }
    return A1HSM_SUPER(a1hsm_top);
}

static enum a1hsmrc sinit(struct redispatch *me, const struct event *event) {
    (void)event;
    me->foo = 0;
    return A1HSM_TRAN(s1);
}

static void test_redispatch(void) {
    struct redispatch *me = &m_redispatch;
    a1hsm_ctor(&me->hsm, &A1HSM_STATE(sinit));

    a1hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(0 == me->foo);

    static const struct event e = {.id = HSM_EVT_A};
    a1hsm_dispatch(&me->hsm, &e);
    ASSERT(1 == me->foo);
    ASSERT(a1hsm_state_is_eq(&me->hsm, &A1HSM_STATE(s2)));
}

int main(void) {
    test_redispatch();
    return 0;
}
