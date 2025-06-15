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
#include <string.h>

#include "bit/bit.h"
#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "queue/queue.h"
#include "event/event.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

static struct am_bit_u64 am_ready_aos_ = {0};

void am_ao_state_ctor_(void) {
    memset(&am_ready_aos_, 0, sizeof(am_ready_aos_));
}

static void am_ao_handle(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct am_ao *ao = ctx;
    struct am_ao_state *me = &am_ao_state_;
    me->debug(ao, event);
    AM_ATOMIC_STORE_N(&ao->last_event, event->id);
    me->running_ao_prio = ao->prio;

    am_hsm_dispatch(&ao->hsm, event);

    me->running_ao_prio = AM_AO_PRIO_INVALID;
    AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_INVALID);
}

bool am_ao_run_all(void) {
    struct am_ao_state *me = &am_ao_state_;

    bool dispatched = false;
    do {
        me->crit_enter();
        if (am_bit_u64_is_empty(&am_ready_aos_)) {
            AM_ASSERT(!dispatched);
            if (me->on_idle) {
                /*
                 * We intentionally do not call me->crit_exit() before
                 * calling me->on_idle() callback here to let
                 * the callback to enter low power mode, if needed.
                 */
                me->on_idle();
            }
            me->crit_exit();
            break;
        }
        int msb = am_bit_u64_msb(&am_ready_aos_);
        me->crit_exit();

        struct am_ao *ao = me->aos[msb];
        AM_ASSERT(ao);
        AM_ASSERT(ao->prio.ao == msb);

        bool popped = am_event_pop_front(&ao->event_queue, am_ao_handle, ao);
        if (!popped) {
            me->crit_enter();
            if (am_queue_is_empty(&ao->event_queue)) {
                am_bit_u64_clear(&am_ready_aos_, ao->prio.ao);
            }
            me->crit_exit();
            continue;
        }
        dispatched = true;
    } while (!dispatched);

    return dispatched;
}

void am_ao_start(
    struct am_ao *ao,
    struct am_ao_prio prio,
    const struct am_event *queue[],
    int queue_size,
    void *stack,
    int stack_size,
    const char *name,
    const struct am_event *init_event
) {
    (void)stack;
    (void)stack_size;

    AM_ASSERT(ao);
    AM_ASSERT(ao->ctor_called);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(prio));
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    struct am_blk blk = {
        .ptr = queue, .size = (int)sizeof(struct am_event *) * queue_size
    };

    am_queue_ctor(
        &ao->event_queue,
        sizeof(struct am_event *),
        AM_ALIGNOF(am_event_ptr_t),
        &blk
    );

    ao->prio = prio;
    ao->name = name;
    ao->task_id = am_pal_task_get_own_id();
    ao->init_event = init_event;

    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(NULL == me->aos[prio.ao]);
    me->aos[prio.ao] = ao;
    ++me->aos_cnt;

    me->running_ao_prio = prio;
    am_hsm_init(&ao->hsm, ao->init_event);
    me->running_ao_prio = AM_AO_PRIO_INVALID;
}

void am_ao_stop(struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));
    int task_id = am_pal_task_get_own_id();
    AM_ASSERT(task_id == ao->task_id); /* check API description */
    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->aos_cnt);

    if (me->subscribe_list_set) {
        am_ao_unsubscribe_all(ao);
    }

    me->crit_enter();

    struct am_event **e = NULL;
    while ((e = am_queue_pop_front(&ao->event_queue)) != NULL) {
        const struct am_event *event = *e;
        me->crit_exit();
        AM_ASSERT(event);
        am_event_free(&event);
        me->crit_enter();
    }
    am_queue_dtor(&ao->event_queue);
    am_bit_u64_clear(&am_ready_aos_, ao->prio.ao);

    me->aos[ao->prio.ao] = NULL;
    --me->aos_cnt;
    ao->ctor_called = false;
    ao->stopped = true;

    me->crit_exit();
}

void am_ao_notify_unsafe(const struct am_ao *ao) {
    if (AM_PAL_TASK_ID_NONE == ao->task_id) {
        return;
    }
    am_bit_u64_set(&am_ready_aos_, ao->prio.ao);
    am_pal_task_notify(ao->task_id);
}

void am_ao_notify(const struct am_ao *ao) {
    AM_ASSERT(ao);

    struct am_ao_state *me = &am_ao_state_;
    me->crit_enter();
    am_ao_notify_unsafe(ao);
    me->crit_exit();
}

int am_ao_get_own_prio(void) {
    const struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(AM_AO_PRIO_IS_VALID(me->running_ao_prio));
    return me->running_ao_prio.ao;
}
