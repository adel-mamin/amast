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

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "common/macros.h"
#include "event/event.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"

#define AM_EVT_PUB AM_EVT_USER
#define AM_AO_EVT_PUB_MAX (AM_EVT_PUB + 1)

#include "ao/ao.h"
#include "pal/pal.h"

struct test_publish {
    struct am_ao ao;
    void (*log)(const char *fmt, ...);
    char log_buf[256];
};

static struct test_publish m_publish;
static struct am_ao *m_me = &m_publish.ao;

static struct am_ao_subscribe_list m_pubsub_list[AM_AO_EVT_PUB_MAX];
static const struct am_event *m_queue_publish[1];

static enum am_hsm_rc publish_s(
    struct test_publish *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_PUB:
        me->log("s-PUB;");
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc publish_sinit(
    struct test_publish *me, const struct am_event *event
) {
    (void)event;
    am_ao_subscribe(&me->ao, AM_EVT_PUB);
    return AM_HSM_TRAN(publish_s);
}

static void publish_ctor(void (*log)(const char *fmt, ...)) {
    struct test_publish *me = &m_publish;
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(publish_sinit));
    me->log = log;
}

static void publish_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    struct test_publish *me = &m_publish;
    str_vlcatf(me->log_buf, (int)sizeof(me->log_buf), fmt, ap);
    va_end(ap);
}

static void test_publish(void) {
    struct am_ao_state_cfg cfg = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg);

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    publish_ctor(publish_log);

    am_ao_start(
        m_me,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue_publish,
        /*nqueue=*/AM_COUNTOF(m_queue_publish),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"publish",
        /*init_event=*/NULL
    );

    static const struct am_event event = {.id = AM_EVT_PUB};

    am_ao_run_all();
    am_ao_publish(&event);
    am_ao_run_all();

    const char *expected = "s-PUB";
    AM_ASSERT(0 == strncmp(m_publish.log_buf, expected, strlen(expected)));
    m_publish.log_buf[0] = '\0';

    am_ao_run_all();
    am_ao_publish_exclude(&event, m_me);
    am_ao_run_all();

    AM_ASSERT('\0' == m_publish.log_buf[0]);
}

int main(void) {
    test_publish();
    return 0;
}
