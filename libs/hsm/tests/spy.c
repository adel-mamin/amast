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
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"
#include "common.h"

struct test {
    struct am_hsm hsm;
    void (*log)(char *fmt, ...);
};

static struct test m_test;

/* test HSM spy callback operation */

static enum am_hsm_rc s(struct test *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_USER:
        me->log("s-AM_EVT_USER;");
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc test_init(struct test *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(s);
}

static void test_ctor(void (*log)(char *fmt, ...)) {
    struct test *me = &m_test;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(test_init));
    me->log = log;
}

static void spy(struct am_hsm *hsm, const struct am_event *event) {
    if (AM_EVT_USER == event->id) {
        struct test *me = (struct test *)hsm;
        me->log("spy-AM_EVT_USER;");
        return;
    }
    AM_ASSERT(0);
}

static char m_log_buf[256];

static void test_log(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_log_buf, (int)sizeof(m_log_buf), fmt, ap);
    va_end(ap);
}

static void test_spy(void) {
    test_ctor(test_log);

    struct test *me = &m_test;
    am_hsm_set_spy(&me->hsm, spy);

    am_hsm_init(&me->hsm, /*init_event=*/NULL);
    am_hsm_dispatch(&me->hsm, &(struct am_event){.id = AM_EVT_USER});

    const char *out = "spy-AM_EVT_USER;s-AM_EVT_USER;";
    AM_ASSERT(0 == strncmp(m_log_buf, out, strlen(out)));
}

int main(void) {
    test_spy();
    return 0;
}
