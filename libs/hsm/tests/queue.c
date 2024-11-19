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

/**
 * Test HSM with event queue.
 * Test event allocation, sending and garbage collection.
 * HSM has two states: hsmq_a and hsmq_b.
 * On handling of event A HSM allocates event B, sends it to itself and
 * transition to state hsm_b, where the event B is processed.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#include "common/macros.h"
#include "common/alignment.h"
#include "blk/blk.h"
#include "event/event.h"
#include "queue/queue.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"

#define AM_EVT_A AM_EVT_USER
#define AM_EVT_B (AM_EVT_USER + 1)
#define AM_EVT_C (AM_EVT_USER + 2)

static char m_hsmq_log_buf[256];

static void hsmq_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_hsmq_log_buf, (int)sizeof(m_hsmq_log_buf), fmt, ap);
    va_end(ap);
}

struct am_hsmq {
    struct am_hsm hsm;
    struct am_queue queue;
    void (*log)(const char *fmt, ...);
};

static struct am_hsmq am_hsmq_;

static struct am_hsm *am_hsmq = &am_hsmq_.hsm;

static enum am_hsm_rc hsmq_a(struct am_hsmq *me, const struct am_event *event);
static enum am_hsm_rc hsmq_b(struct am_hsmq *me, const struct am_event *event);

static void hsmq_commit(void) {
    struct am_hsmq *me = &am_hsmq_;
    while (!am_queue_is_empty(&me->queue)) {
        const struct am_event **e = am_queue_pop_front(&me->queue);
        AM_ASSERT(e);
        AM_ASSERT(*e);
        am_hsm_dispatch(am_hsmq, *e);
    }
}

static enum am_hsm_rc hsmq_init(
    struct am_hsmq *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(hsmq_a);
}

static void hsmq_ctor(void (*log)(const char *fmt, ...)) {
    struct am_hsmq *me = &am_hsmq_;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE_CTOR(hsmq_init));
    me->log = log;

    /* setup HSM event queue */
    static const struct am_event *pool[2];
    struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
    am_queue_init(
        &me->queue,
        /*isize=*/sizeof(pool[0]),
        AM_ALIGNOF(struct am_event *),
        &blk
    );
}

static enum am_hsm_rc hsmq_a(struct am_hsmq *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_A: {
        me->log("a-A;");
        const struct am_event *e =
            am_event_allocate(/*id=*/AM_EVT_B, sizeof(*e), /*margin=*/0);
        am_event_push_back(me, &me->queue, e);
        return AM_HSM_TRAN(hsmq_b);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc hsmq_b(struct am_hsmq *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_B: {
        me->log("b-B;");
        return AM_HSM_HANDLED();
    }
    case AM_EVT_C: {
        me->log("b-C;");
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

int main(void) {
    struct am_event_cfg cfg = {0};
    am_event_state_ctor(&cfg);

    {
        static char pool[1 * AM_EVENT_BLOCK_SIZE(struct am_event)] AM_ALIGNED(
            AM_EVENT_BLOCK_ALIGNMENT(struct am_event)
        );
        am_event_add_pool(
            pool,
            (int)sizeof(pool),
            AM_EVENT_BLOCK_SIZE(struct am_event),
            AM_EVENT_BLOCK_ALIGNMENT(struct am_event)
        );
        AM_ASSERT(1 == am_event_get_pool_nblocks(/*index=*/0));
    }

    hsmq_ctor(hsmq_log);

    m_hsmq_log_buf[0] = '\0';
    am_hsm_init(am_hsmq, /*init_event=*/NULL);

    static const struct hsmq_test {
        int event;
        const char *out;
    } in[] = {{AM_EVT_A, "a-A;b-B;"}, {AM_EVT_C, "b-C;"}};

    for (int i = 0; i < AM_COUNTOF(in); i++) {
        struct am_event e = {.id = in[i].event};
        am_hsm_dispatch(am_hsmq, &e);
        hsmq_commit();
        AM_ASSERT(0 == strncmp(m_hsmq_log_buf, in[i].out, strlen(in[i].out)));
        m_hsmq_log_buf[0] = '\0';
    }

    am_hsm_dtor(am_hsmq);

    /* make sure there is no memory leak */
    AM_ASSERT(1 == am_event_get_pool_nblocks(/*index=*/0));

    return 0;
}
