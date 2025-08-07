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

#include <string.h>
#include <stdarg.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "strlib/strlib.h"
#include "fsm/fsm.h"

struct reenter_fsm {
    struct am_fsm fsm;
    AM_PRINTF(1, 0) void (*log)(const char *fmt, ...);
    char log_buf[256];
};

static struct reenter_fsm m_reenter_fsm;

/* test FSM state re-enter operation */

static enum am_rc reenter_fsm_s(
    struct reenter_fsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_FSM_ENTRY:
        me->log("s-AM_EVT_FSM_ENTRY;");
        break;
    case AM_EVT_USER:
        me->log("s-AM_EVT_USER;");
        return AM_FSM_TRAN(reenter_fsm_s);
    case AM_EVT_FSM_EXIT:
        me->log("s-AM_EVT_FSM_EXIT;");
        break;
    default:
        break;
    }
    return AM_FSM_HANDLED();
}

static enum am_rc reenter_fsm_init(
    struct reenter_fsm *me, const struct am_event *event
) {
    (void)event;
    return AM_FSM_TRAN(reenter_fsm_s);
}

static void reenter_fsm_ctor(
    AM_PRINTF(1, 0) void (*log)(const char *fmt, ...)
) {
    struct reenter_fsm *me = &m_reenter_fsm;
    am_fsm_ctor(&me->fsm, AM_FSM_STATE_CTOR(reenter_fsm_init));
    me->log = log;
}

static AM_PRINTF(1, 0) void reenter_fsm_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(
        m_reenter_fsm.log_buf, (int)sizeof(m_reenter_fsm.log_buf), fmt, ap
    );
    va_end(ap);
}

static void test_reenter_fsm(void) {
    reenter_fsm_ctor(reenter_fsm_log);

    struct reenter_fsm *me = &m_reenter_fsm;

    am_fsm_init(&me->fsm, /*init_event=*/NULL);
    am_fsm_dispatch(&me->fsm, &(struct am_event){.id = AM_EVT_USER});

    const char *out =
        "s-AM_EVT_FSM_ENTRY;s-AM_EVT_USER;s-AM_EVT_FSM_EXIT;"
        "s-AM_EVT_FSM_ENTRY;";
    AM_ASSERT(0 == strncmp(m_reenter_fsm.log_buf, out, strlen(out)));
}

int main(void) {
    test_reenter_fsm();
    return 0;
}
