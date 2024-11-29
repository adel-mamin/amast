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

#ifdef AMAST_AO_PREEMPTIVE

#include <stdbool.h>
#include <stddef.h>

#include "blk/blk.h"
#include "common/alignment.h"
#include "common/macros.h"
#include "hsm/hsm.h"
#include "queue/queue.h"
#include "event/event.h"
#include "timer/timer.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

static void am_ao_task(void *param) {
    AM_ASSERT(param);

    struct am_ao *ao = (struct am_ao *)param;

    while (AM_LIKELY(!ao->stopped)) {
        const struct am_event *e = am_event_pop_front(ao, &ao->event_queue);
        struct am_ao_state *me = &g_am_ao_state;
        me->debug(ao, e);

        ao->last_event = e->id;
        am_hsm_dispatch(&ao->hsm, e);
        ao->last_event = AM_EVT_INVALID;

        am_event_free(e);
    }
}

bool am_ao_run_all(bool loop) {
    const struct am_ao_state *me = &g_am_ao_state;
    (void)loop;
    while (loop && AM_UNLIKELY(!me->ao_state_dtor_called)) {
        am_pal_sleep_ticks(/*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT, /*ticks=*/1);
        am_timer_tick(/*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT);
    }
    return false;
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
    AM_ASSERT(ao);
    AM_ASSERT(prio >= 0);
    AM_ASSERT(prio < AM_AO_NUM_MAX);
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    struct am_blk blk = {
        .ptr = queue, .size = (int)sizeof(queue[0]) * queue_size
    };

    am_queue_init(
        &ao->event_queue,
        sizeof(struct am_event *),
        AM_ALIGNOF(struct am_event *),
        &blk
    );

    ao->prio = prio;
    ao->name = name;

    struct am_ao_state *me = &g_am_ao_state;
    AM_ASSERT(NULL == me->ao[prio]);
    me->ao[prio] = ao;
    am_hsm_init(&ao->hsm, init_event);

    ao->task_id = am_pal_task_create(
        name,
        prio,
        stack,
        stack_size,
        /*entry=*/am_ao_task,
        /*arg=*/ao
    );
}

void am_ao_notify(void *ao) {
    AM_ASSERT(ao);
    const struct am_ao *ao_ = (struct am_ao *)ao;
    if (AM_PAL_TASK_ID_NONE == ao_->task_id) {
        return;
    }
    am_pal_task_notify(ao_->task_id);
}

void am_ao_wait(void *ao) {
    AM_ASSERT(ao);
    const struct am_ao *ao_ = (struct am_ao *)ao;
    am_pal_task_wait(ao_->task_id);
}

#endif /* AMAST_AO_PREEMPTIVE */