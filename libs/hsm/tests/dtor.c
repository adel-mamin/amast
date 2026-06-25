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

/**
 * The topology of the tested HSM:
 *
 *  +-------------+
 *  |  dtor_sinit |
 *  +------+------+
 *         |
 *  +------|-------------+
 *  |      |  am_hsm_top |
 *  | +----v---+         |
 *  | | dtor_s |         |
 *  | +--------+         |
 *  +--------------------+
 */

#include <stddef.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "hsm/hsm.h"

struct dtor_hsm {
    struct am_hsm hsm;
};

static struct dtor_hsm m_dtor_hsm;

/* test am_hsm_destroy() */

static enum am_rc dtor_s(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_EXIT:
        return am_hsm_handled(hsm);
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc dtor_sinit(struct am_hsm* hsm, const struct am_event* event) {
    (void)event;
    return am_hsm_tran(hsm, dtor_s);
}

static void dtor_hsm(void) {
    struct dtor_hsm* me = &m_dtor_hsm;
    am_hsm_create(&me->hsm, am_hsm_state_make(dtor_sinit));
    am_hsm_start(&me->hsm, /*init_event=*/NULL);
    am_hsm_destroy(&me->hsm);
    AM_ASSERT(am_hsm_is_in(&me->hsm, am_hsm_state_make(am_hsm_top)));
}

int main(void) {
    dtor_hsm();
    return 0;
}
