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

#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"
#include "common.h"

struct reenter_hsm {
    struct am_hsm hsm;
    AM_PRINTF(1, 0) void (*log)(const char* fmt, ...);
    char log_buf[256];
};

static struct reenter_hsm m_reenter_hsm;

/* test HSM state re-enter operation */

static enum am_rc reenter_hsm_s1(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc reenter_hsm_s(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct reenter_hsm* me = AM_CONTAINER_OF(hsm, struct reenter_hsm, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s-ENTRY;");
        return am_hsm_handled(hsm);
    case AM_EVT_INIT:
        return am_hsm_tran(hsm, reenter_hsm_s1);
    case HSM_EVT_A:
        me->log("s-EVT_A;");
        return am_hsm_tran(hsm, reenter_hsm_s);
    case AM_EVT_EXIT:
        me->log("s-EXIT;");
        return am_hsm_handled(hsm);
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc reenter_hsm_s1(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct reenter_hsm* me = AM_CONTAINER_OF(hsm, struct reenter_hsm, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        me->log("s1-ENTRY;");
        return am_hsm_handled(hsm);
        break;
    case HSM_EVT_B:
        me->log("s1-EVT_B;");
        return am_hsm_tran(hsm, reenter_hsm_s1);
    case HSM_EVT_C:
        me->log("s1-EVT_C;");
        return am_hsm_tran(hsm, reenter_hsm_s);
    case AM_EVT_EXIT:
        me->log("s1-EXIT;");
        return am_hsm_handled(hsm);
    default:
        break;
    }
    return am_hsm_super(hsm, reenter_hsm_s);
}

static enum am_rc reenter_hsm_init(
    struct am_hsm* hsm, const struct am_event* event
) {
    (void)event;
    return am_hsm_tran(hsm, reenter_hsm_s);
}

static void reenter_hsm_ctor(
    AM_PRINTF(1, 0) void (*log)(const char* fmt, ...)
) {
    struct reenter_hsm* me = &m_reenter_hsm;
    am_hsm_ctor(&me->hsm, am_hsm_state(reenter_hsm_init));
    me->log = log;
    me->log_buf[0] = '\0';
}

static AM_PRINTF(1, 0) void reenter_hsm_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(
        m_reenter_hsm.log_buf, (int)sizeof(m_reenter_hsm.log_buf), fmt, ap
    );
    va_end(ap);
}

static void test_reenter_hsm(void) {
    reenter_hsm_ctor(reenter_hsm_log);

    struct reenter_hsm* me = &m_reenter_hsm;

    am_hsm_init(&me->hsm, /*init_event=*/NULL);

    {
        const char* out = "s-ENTRY;s1-ENTRY;";
        AM_ASSERT(0 == strncmp(m_reenter_hsm.log_buf, out, strlen(out)));
        m_reenter_hsm.log_buf[0] = '\0';
    }

    struct test {
        uint16_t event;
        const char* out;
    };
    static const struct test in[] = {
        /* clang-format off */
        {HSM_EVT_A, "s-EVT_A;s1-EXIT;s-EXIT;s-ENTRY;s1-ENTRY;"},
        {HSM_EVT_B, "s1-EVT_B;s1-EXIT;s1-ENTRY;"},
        {HSM_EVT_C, "s1-EVT_C;s1-EXIT;s1-ENTRY;"},
        /* clang-format on */
    };

    for (int i = 0; i < AM_COUNTOF(in); ++i) {
        struct am_event e = {.id = in[i].event};
        am_hsm_dispatch(&m_reenter_hsm.hsm, &e);
        AM_ASSERT(
            0 == strncmp(m_reenter_hsm.log_buf, in[i].out, strlen(in[i].out))
        );
        m_reenter_hsm.log_buf[0] = '\0';
    }
}

int main(void) {
    test_reenter_hsm();
    return 0;
}
