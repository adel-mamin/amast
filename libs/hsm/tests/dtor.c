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

struct test_dtor {
    struct am_hsm hsm;
};

static struct test_dtor m_test_dtor;

/* test am_hsm_dtor() */

static enum am_hsm_rc dtor_s(
    struct test_dtor *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_EXIT:
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc dtor_sinit(
    struct test_dtor *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(dtor_s);
}

static void test_dtor(void) {
    struct test_dtor *me = &m_test_dtor;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(dtor_sinit));
    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    am_hsm_dtor(&me->hsm);
}

int main(void) {
    test_dtor();
    return 0;
}
