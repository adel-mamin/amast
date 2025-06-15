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
 * Test FSM with event queue.
 * Test event allocation, sending and garbage collection.
 * FSM has two states: fsmq_a and fsmq_b.
 * On handling of event A FSM allocates event B, sends it to itself and
 * transition to state fsm_b, where the event B is processed.
 */

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "queue/queue.h"
#include "onesize/onesize.h"
#include "slist/slist.h"
#include "strlib/strlib.h"
#include "pal/pal.h"
#include "fsm/fsm.h"

#define AM_EVT_A AM_EVT_USER
#define AM_EVT_B (AM_EVT_USER + 1)
#define AM_EVT_C (AM_EVT_USER + 2)

static char m_fsmq_log_buf[256];

static void fsmq_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_fsmq_log_buf, (int)sizeof(m_fsmq_log_buf), fmt, ap);
    va_end(ap);
}

struct am_fsmq {
    struct am_fsm fsm;
    struct am_queue event_queue;
    void (*log)(const char *fmt, ...);
};

static struct am_fsmq am_fsmq_;

static struct am_fsm *am_fsmq = &am_fsmq_.fsm;

static enum am_fsm_rc fsmq_a(struct am_fsmq *me, const struct am_event *event);
static enum am_fsm_rc fsmq_b(struct am_fsmq *me, const struct am_event *event);

static void fsmq_handle(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct am_fsm *me = ctx;
    AM_ASSERT(event);
    am_fsm_dispatch(me, event);
}

static void fsmq_commit(void) {
    struct am_fsmq *me = &am_fsmq_;
    while (!am_queue_is_empty(&me->event_queue)) {
        bool popped = am_event_pop_front(&me->event_queue, fsmq_handle, me);
        AM_ASSERT(popped);
    }
}

static enum am_fsm_rc fsmq_init(
    struct am_fsmq *me, const struct am_event *event
) {
    (void)event;
    return AM_FSM_TRAN(fsmq_a);
}

static void fsmq_ctor(void (*log)(const char *fmt, ...)) {
    struct am_fsmq *me = &am_fsmq_;
    am_fsm_ctor(&me->fsm, AM_FSM_STATE_CTOR(fsmq_init));
    me->log = log;

    /* setup FSM event queue */
    static const struct am_event *pool[2];
    struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};
    am_queue_ctor(
        &me->event_queue,
        /*isize=*/sizeof(pool[0]),
        AM_ALIGNOF(am_event_ptr_t),
        &blk
    );
}

static enum am_fsm_rc fsmq_a(struct am_fsmq *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_A: {
        me->log("a-A;");
        const struct am_event *e =
            am_event_allocate(/*id=*/AM_EVT_B, sizeof(*e));
        am_event_push_back(&me->event_queue, e);
        return AM_FSM_TRAN(fsmq_b);
    }
    default:
        break;
    }
    return AM_FSM_HANDLED();
}

static enum am_fsm_rc fsmq_b(struct am_fsmq *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_B: {
        me->log("b-B;");
        return AM_FSM_HANDLED();
    }
    case AM_EVT_C: {
        me->log("b-C;");
        return AM_FSM_HANDLED();
    }
    default:
        break;
    }
    return AM_FSM_HANDLED();
}

int main(void) {
    struct am_event_state_cfg cfg = {
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit,
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

    fsmq_ctor(fsmq_log);

    m_fsmq_log_buf[0] = '\0';
    am_fsm_init(am_fsmq, /*init_event=*/NULL);

    static const struct fsmq_test {
        int event;
        const char *out;
    } in[] = {{AM_EVT_A, "a-A;b-B;"}, {AM_EVT_C, "b-C;"}};

    for (int i = 0; i < AM_COUNTOF(in); ++i) {
        struct am_event e = {.id = in[i].event};
        am_fsm_dispatch(am_fsmq, &e);
        fsmq_commit();
        AM_ASSERT(0 == strncmp(m_fsmq_log_buf, in[i].out, strlen(in[i].out)));
        m_fsmq_log_buf[0] = '\0';
    }

    am_fsm_dtor(am_fsmq);

    /* make sure there is no memory leak */
    AM_ASSERT(1 == am_event_get_pool_nfree(/*index=*/0));

    return 0;
}
