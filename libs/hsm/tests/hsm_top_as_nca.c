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

#include <stddef.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "hsm/hsm.h"
#include "common.h"

static struct am_hsm m_test_nca;

/* Test am_hsm_top() as NCA. */

static enum am_rc nca_s11(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc nca_s2(struct am_hsm* hsm, const struct am_event* event);

static enum am_rc nca_s1(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_INIT:
        return am_hsm_tran(hsm, nca_s11);
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc nca_s11(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case HSM_EVT_A:
        return am_hsm_tran(hsm, nca_s2);
    default:
        break;
    }
    return am_hsm_super(hsm, nca_s1);
}

static enum am_rc nca_s2(struct am_hsm* hsm, const struct am_event* event) {
    (void)event;
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc nca_init(struct am_hsm* hsm, const struct am_event* event) {
    (void)event;
    return am_hsm_tran(hsm, nca_s1);
}

static void test_am_hsm_top_as_nca(void) {
    struct am_hsm* hsm = &m_test_nca;
    am_hsm_create(hsm, am_hsm_state_make(nca_init));

    am_hsm_start(hsm, /*init_event=*/NULL);
    AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(nca_s11)));

    static const struct am_event event = {.id = HSM_EVT_A};
    am_hsm_dispatch(hsm, &event);
    AM_ASSERT(am_hsm_is_in(hsm, am_hsm_state_make(nca_s2)));
}

int main(void) {
    test_am_hsm_top_as_nca();

    return 0;
}
