/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
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
    struct hsm hsm;
    int foo;
};

static struct redispatch m_redispatch;

/* test HSM_TRAN_REDISPATCH() */

static enum hsm_rc s(struct redispatch *me, const struct event *event);
static enum hsm_rc s1(struct redispatch *me, const struct event *event);

static enum hsm_rc s(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        return HSM_TRAN_REDISPATCH(s1);
    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc s1(struct redispatch *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_A:
        me->foo = 1;
        return HSM_HANDLED();
    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc sinit(struct redispatch *me, const struct event *event) {
    (void)event;
    return HSM_TRAN(s);
}

static void test_redispatch(void) {
    struct redispatch *me = &m_redispatch;
    hsm_ctor(&me->hsm, &HSM_STATE(sinit));
    me->foo = 0;

    hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(0 == me->foo);

    static const struct event e = {.id = HSM_EVT_A};
    hsm_dispatch(&me->hsm, &e);
    ASSERT(1 == me->foo);
    ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s1)));
}

int main(void) {
    test_redispatch();
    return 0;
}
