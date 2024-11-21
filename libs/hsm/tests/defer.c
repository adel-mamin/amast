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

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#include "common/alignment.h"
#include "common/macros.h"
#include "event/event.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"
#include "common.h"
#include "blk/blk.h"
#include "queue/queue.h"

struct test_defer {
    struct am_hsm hsm;
    struct am_queue event_queue;
    struct am_queue defer_queue;
    void (*log)(const char *fmt, ...);
    char log_buf[256];
};

static struct test_defer m_test_defer;

static enum am_hsm_rc defer_s1(
    struct test_defer *me, const struct am_event *event
);
static enum am_hsm_rc defer_s2(
    struct test_defer *me, const struct am_event *event
);

static enum am_hsm_rc defer_s1(
    struct test_defer *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_EXIT:
        (void)am_event_recall(me, &me->defer_queue);
        return AM_HSM_HANDLED();
    case HSM_EVT_A:
        me->log("s1-A;");
        am_event_defer(&me->defer_queue, event);
        return AM_HSM_HANDLED();
    case HSM_EVT_B:
        me->log("s1-B;");
        return AM_HSM_TRAN(defer_s2);
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc defer_s2(
    struct test_defer *me, const struct am_event *event
) {
    switch (event->id) {
    case HSM_EVT_A:
        me->log("s2-A;");
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc defer_sinit(
    struct test_defer *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(defer_s1);
}

static void defer_ctor(void (*log)(const char *fmt, ...)) {
    struct test_defer *me = &m_test_defer;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE_CTOR(defer_sinit));
    me->log = log;

    /* setup HSM event queue */
    {
        static const struct am_event *pool[2];
        struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
        am_queue_init(
            &me->event_queue,
            /*isize=*/sizeof(pool[0]),
            AM_ALIGNOF(struct am_event *),
            &blk
        );
    }

    /* setup HSM defer queue */
    {
        static const struct am_event *pool[2];
        struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
        am_queue_init(
            &me->defer_queue,
            /*isize=*/sizeof(pool[0]),
            AM_ALIGNOF(struct am_event *),
            &blk
        );
    }
}

static void defer_hsm_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    struct test_defer *me = &m_test_defer;
    str_vlcatf(me->log_buf, (int)sizeof(me->log_buf), fmt, ap);
    va_end(ap);
}

static void defer_push_front(void *owner, const struct am_event *event) {
    struct test_defer *me = (struct test_defer *)owner;
    am_event_push_front(owner, &me->event_queue, event);
}

static void defer_commit(void) {
    struct test_defer *me = &m_test_defer;
    while (!am_queue_is_empty(&me->event_queue)) {
        const struct am_event **e = am_queue_pop_front(&me->event_queue);
        AM_ASSERT(e);
        AM_ASSERT(*e);
        am_hsm_dispatch(&me->hsm, *e);
    }
}

static void test_defer(void) {
    struct am_event_cfg cfg = {.push_front = defer_push_front};
    am_event_state_ctor(&cfg);

    {
        static char pool[2 * AM_EVENT_BLOCK_SIZE(struct am_event)] AM_ALIGNED(
            AM_EVENT_BLOCK_ALIGNMENT(struct am_event)
        );
        am_event_add_pool(
            pool,
            (int)sizeof(pool),
            AM_EVENT_BLOCK_SIZE(struct am_event),
            AM_EVENT_BLOCK_ALIGNMENT(struct am_event)
        );
        AM_ASSERT(2 == am_event_get_pool_nblocks(/*index=*/0));
    }

    defer_ctor(defer_hsm_log);

    struct test_defer *me = &m_test_defer;
    am_hsm_init(&me->hsm, /*init_event=*/NULL);

    struct test {
        int event;
        const char *out;
    };
    static const struct test in[] = {
        {HSM_EVT_A, "s1-A;"},
        {HSM_EVT_B, "s1-B;s2-A;"},
    };

    for (int i = 0; i < AM_COUNTOF(in); i++) {
        const struct am_event *e = am_event_allocate(
            in[i].event, (int)sizeof(struct am_event), /*margin=*/0
        );
        am_hsm_dispatch(&me->hsm, e);
        defer_commit();
        AM_ASSERT(0 == strncmp(me->log_buf, in[i].out, strlen(in[i].out)));
        me->log_buf[0] = '\0';
    }

    /* make sure there is no memory leak */
    AM_ASSERT(2 == am_event_get_pool_nblocks(/*index=*/0));
}

int main(void) {
    test_defer();
    return 0;
}
