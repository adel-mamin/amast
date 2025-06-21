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
 * Test HSM with event queue.
 * Test event allocation, sending and garbage collection.
 *
 * HSM topology:
 *
 *  +------------+
 *  | hsmq_sinit |
 *  +------+-----+
 *         |
 *  +------|--------------------------+
 *  |      |    am_hsm_top            |
 *  | +----v-----+       +----------+ |
 *  | |          |       | B/       | |
 *  | |          |       | C/       | |
 *  | |          |   A   |          | |
 *  | | hsmq_s1  +------->  hsmq_s2 | |
 *  | +----------+       +----------+ |
 *  +---------------------------------+
 *
 * On handling of event A HSM allocates event B, sends it to itself and
 * transition to state hsm_b, where the event B is processed.
 */

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "event/event.h"
#include "queue/queue.h"
#include "onesize/onesize.h"
#include "slist/slist.h"
#include "strlib/strlib.h"
#include "pal/pal.h"
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
    struct am_queue event_queue;
    void (*log)(const char *fmt, ...);
};

static struct am_hsmq am_hsmq_;

static struct am_hsm *am_hsmq = &am_hsmq_.hsm;

static enum am_hsm_rc hsmq_s1(struct am_hsmq *me, const struct am_event *event);
static enum am_hsm_rc hsmq_s2(struct am_hsmq *me, const struct am_event *event);

static void hsmq_dispatch(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct am_hsm *me = ctx;
    am_hsm_dispatch(me, event);
}

static void hsmq_commit(void) {
    struct am_hsmq *me = &am_hsmq_;
    while (!am_queue_is_empty(&me->event_queue)) {
        bool popped = am_event_pop_front(&me->event_queue, hsmq_dispatch, me);
        AM_ASSERT(popped);
    }
}

static enum am_hsm_rc hsmq_sinit(
    struct am_hsmq *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_TRAN(hsmq_s1);
}

static void hsmq_ctor(void (*log)(const char *fmt, ...)) {
    struct am_hsmq *me = &am_hsmq_;
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(hsmq_sinit));
    me->log = log;

    /* setup HSM event queue */
    static const struct am_event *pool[2];
    am_queue_ctor(
        &me->event_queue,
        /*isize=*/sizeof(pool[0]),
        AM_ALIGNOF(am_event_ptr_t),
        pool,
        (int)sizeof(pool)
    );
}

static enum am_hsm_rc hsmq_s1(
    struct am_hsmq *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_A: {
        me->log("a-A;");
        const struct am_event *e =
            am_event_allocate(/*id=*/AM_EVT_B, sizeof(*e));
        am_event_push_back(&me->event_queue, e);
        return AM_HSM_TRAN(hsmq_s2);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc hsmq_s2(
    struct am_hsmq *me, const struct am_event *event
) {
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
    struct am_event_state_cfg cfg = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_event_state_ctor(&cfg);

    {
        static char pool[1 * AM_POOL_BLOCK_SIZEOF(struct am_event)] AM_ALIGNED(
            AM_ALIGN_MAX
        );
        am_event_add_pool(
            pool,
            (int)sizeof(pool),
            AM_POOL_BLOCK_SIZEOF(struct am_event),
            AM_POOL_BLOCK_ALIGNMENT(AM_ALIGNOF(am_event_t))
        );
        AM_ASSERT(1 == am_event_get_pool_nblocks(/*index=*/0));
        AM_ASSERT(1 == am_event_get_pool_nfree(/*index=*/0));
    }

    hsmq_ctor(hsmq_log);

    m_hsmq_log_buf[0] = '\0';
    am_hsm_init(am_hsmq, /*init_event=*/NULL);

    static const struct hsmq_test {
        int event;
        const char *out;
    } in[] = {
        {.event = AM_EVT_A, .out = "a-A;b-B;"},
        {.event = AM_EVT_C, .out = "b-C;"}
    };

    for (int i = 0; i < AM_COUNTOF(in); ++i) {
        struct am_event e = {.id = in[i].event};
        am_hsm_dispatch(am_hsmq, &e);
        hsmq_commit();
        AM_ASSERT(0 == strncmp(m_hsmq_log_buf, in[i].out, strlen(in[i].out)));
        m_hsmq_log_buf[0] = '\0';
    }

    am_hsm_dtor(am_hsmq);

    /* make sure there is no memory leak */
    AM_ASSERT(1 == am_event_get_pool_nfree(/*index=*/0));

    return 0;
}
