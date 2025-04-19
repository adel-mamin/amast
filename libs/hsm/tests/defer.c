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

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "onesize/onesize.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"
#include "common.h"
#include "queue/queue.h"
#include "pal/pal.h"

/**
 * The topology of the tested HSM:
 *
 *  +-------------+
 *  | defer_sinit |
 *  +------+------+
 *         |
 *  +------|--------------------------+
 *  |      |    am_hsm_top            |
 *  | +----v-----+       +----------+ |
 *  | | A/defer  |       | A/       | |
 *  | | X:recall |       |          | |
 *  | |          |   B   |          | |
 *  | | defer_s1 +-------> defer_s2 | |
 *  | +----------+       +----------+ |
 *  +---------------------------------+
 */

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

static void defer_push_front(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct test_defer *me = (struct test_defer *)ctx;
    am_event_push_front(&me->event_queue, event);
}

static enum am_hsm_rc defer_s1(
    struct test_defer *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_EXIT:
        (void)am_event_pop_front(&me->defer_queue, defer_push_front, me);
        return AM_HSM_HANDLED();
    case HSM_EVT_A:
        me->log("s1-A;");
        am_event_push_back(&me->defer_queue, event);
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
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(defer_sinit));
    me->log = log;

    /* setup HSM event queue */
    {
        static const struct am_event *pool[2];
        struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
        am_queue_ctor(
            &me->event_queue,
            /*isize=*/sizeof(pool[0]),
            AM_ALIGNOF_EVENT_PTR,
            &blk
        );
    }

    /* setup HSM defer queue */
    {
        static const struct am_event *pool[2];
        struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
        am_queue_ctor(
            &me->defer_queue,
            /*isize=*/sizeof(pool[0]),
            AM_ALIGNOF_EVENT_PTR,
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

static void defer_dispatch(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct test_defer *me = (struct test_defer *)ctx;
    am_hsm_dispatch(&me->hsm, event);
}

static void defer_commit(void) {
    struct test_defer *me = &m_test_defer;
    while (!am_queue_is_empty(&me->event_queue)) {
        bool popped = am_event_pop_front(&me->event_queue, defer_dispatch, me);
        AM_ASSERT(popped);
    }
}

static void test_defer(void) {
    struct am_event_state_cfg cfg = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_event_state_ctor(&cfg);

    {
        static char pool[2 * AM_POOL_BLOCK_SIZEOF(struct am_event)] AM_ALIGNED(
            AM_ALIGN_MAX
        );
        am_event_add_pool(
            pool,
            (int)sizeof(pool),
            AM_POOL_BLOCK_SIZEOF(struct am_event),
            AM_POOL_BLOCK_ALIGNMENT(AM_ALIGNOF_EVENT)
        );
        AM_ASSERT(2 == am_event_get_pool_nblocks(/*index=*/0));
        AM_ASSERT(2 == am_event_get_pool_nfree(/*index=*/0));
    }

    defer_ctor(defer_hsm_log);

    struct test_defer *me = &m_test_defer;
    am_hsm_init(&me->hsm, /*init_event=*/NULL);

    static const struct test {
        int event;
        const char *out;
    } in[] = {
        {.event = HSM_EVT_A, .out = "s1-A;"},
        {.event = HSM_EVT_B, .out = "s1-B;s2-A;"},
    };

    for (int i = 0; i < AM_COUNTOF(in); ++i) {
        const struct am_event *e =
            am_event_allocate(in[i].event, (int)sizeof(struct am_event));
        am_event_inc_ref_cnt(e);
        am_hsm_dispatch(&me->hsm, e);
        am_event_free(&e);
        defer_commit();
        AM_ASSERT(0 == strncmp(me->log_buf, in[i].out, strlen(in[i].out)));
        me->log_buf[0] = '\0';
    }

    /* make sure there is no memory leak */
    AM_ASSERT(2 == am_event_get_pool_nfree(/*index=*/0));
}

int main(void) {
    test_defer();
    return 0;
}
