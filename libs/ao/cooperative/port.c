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
#include <stddef.h>

#include "bit/bit.h"
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

bool am_ao_run_all(void) {
    struct am_ao_state *me = &am_ao_state_;
    bool dispatched = false;
    do {
        me->crit_enter();
        if (am_bit_u64_is_empty(&am_ready_aos_)) {
            me->crit_exit();
            break;
        }
        int msb = am_bit_u64_msb(&am_ready_aos_);
        me->crit_exit();

        struct am_ao *ao = me->ao[msb];
        AM_ASSERT(ao);
        AM_ASSERT(ao->prio == msb);

        const struct am_event *e = am_event_pop_front(&ao->event_queue);
        if (!e) {
            me->crit_enter();
            if (am_queue_is_empty(&ao->event_queue)) {
                am_bit_u64_clear(&am_ready_aos_, ao->prio);
            }
            me->crit_exit();
            continue;
        }
        me->debug(ao, e);
        AM_ATOMIC_STORE_N(&ao->last_event, e->id);
        am_hsm_dispatch(&ao->hsm, e);
        AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_INVALID);
        am_event_free(&e);

        dispatched = true;
    } while (!dispatched);

    return dispatched;
}

void am_ao_start(
    struct am_ao *ao,
    int prio,
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
    AM_ASSERT(prio >= AM_AO_PRIO_MIN);
    AM_ASSERT(prio <= AM_AO_PRIO_MAX);
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    struct am_blk blk = {
        .ptr = queue, .size = (int)sizeof(struct am_event *) * queue_size
    };

    am_queue_init(
        &ao->event_queue, sizeof(struct am_event *), AM_ALIGNOF_EVENT_PTR, &blk
    );

    ao->prio = prio;
    ao->name = name;
    ao->task_id = am_pal_task_own_id();

    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(NULL == me->ao[prio]);
    me->ao[prio] = ao;
    am_hsm_init(&ao->hsm, init_event);
}

void am_ao_notify(const struct am_ao *ao) {
    AM_ASSERT(ao);

    if (AM_PAL_TASK_ID_NONE == ao->task_id) {
        return;
    }
    struct am_ao_state *me = &am_ao_state_;

    me->crit_enter();
    am_bit_u64_set(&am_ready_aos_, ao->prio);
    me->crit_exit();

    am_pal_task_notify(ao->task_id);
}
