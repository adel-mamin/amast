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

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "hsm/hsm.h"
#include "queue/queue.h"
#include "event/event.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

void am_ao_state_ctor_(void) { am_pal_lock_all_tasks(); }

static void am_ao_handle(void *ctx, const struct am_event *event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct am_ao *ao = ctx;
    struct am_ao_state *me = &am_ao_state_;

    me->debug(ao, event);

    AM_ATOMIC_STORE_N(&ao->last_event, event->id);
    am_hsm_dispatch(&ao->hsm, event);
    AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_INVALID);
}

static void am_ao_task(void *param) {
    AM_ASSERT(param);

    am_pal_wait_all_tasks();

    struct am_ao *ao = (struct am_ao *)param;

    ao->task_id = am_pal_task_get_own_id();

    while (AM_LIKELY(!ao->stopped)) {
        struct am_ao_state *me = &am_ao_state_;
        me->crit_enter();
        while (am_queue_is_empty(&ao->event_queue)) {
            me->crit_exit();
            am_pal_task_wait(ao->task_id);
            me->crit_enter();
        }
        me->crit_exit();
        bool popped = am_event_pop_front(&ao->event_queue, am_ao_handle, ao);
        AM_ASSERT(popped);
    }
}

bool am_ao_run_all(void) {
    struct am_ao_state *me = &am_ao_state_;

    if (!AM_ATOMIC_LOAD_N(&me->startup_complete)) {
        /* start all AOs */
        am_pal_unlock_all_tasks();
        AM_ATOMIC_STORE_N(&me->startup_complete, true);
    }
    /* wait all AOs to complete */
    am_pal_task_wait(AM_PAL_TASK_ID_MAIN);
    return false;
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
    AM_ASSERT(ao);
    AM_ASSERT(ao->ctor_called);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(prio));
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    am_queue_ctor(
        &ao->event_queue,
        sizeof(struct am_event *),
        AM_ALIGNOF(am_event_ptr_t),
        queue,
        (int)sizeof(struct am_event *) * queue_size
    );

    ao->prio = prio;
    ao->name = name;
    ao->init_event = init_event;

    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(NULL == me->aos[prio.ao]);
    me->aos[prio.ao] = ao;

    me->crit_enter();
    ++me->aos_cnt;
    me->crit_exit();

    am_hsm_init(&ao->hsm, init_event);

    ao->task_id = am_pal_task_create(
        name,
        prio.task,
        stack,
        stack_size,
        /*entry=*/am_ao_task,
        /*arg=*/ao
    );
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
        me->crit_exit();
        AM_ASSERT(*e);
        am_event_free(*e);
        me->crit_enter();
    }
    am_queue_dtor(&ao->event_queue);

    me->aos[ao->prio.ao] = NULL;
    --me->aos_cnt;
    bool running_aos = me->aos_cnt;

    ao->ctor_called = false;

    ao->stopped = true;

    me->crit_exit();

    if (!running_aos) {
        am_pal_task_notify(/*task=*/AM_PAL_TASK_ID_MAIN);
    }
}

void am_ao_notify(const struct am_ao *ao) {
    AM_ASSERT(ao);
    if (AM_PAL_TASK_ID_NONE == ao->task_id) {
        return;
    }
    am_pal_task_notify(ao->task_id);
}

void am_ao_notify_unsafe(const struct am_ao *ao) { am_ao_notify(ao); }

int am_ao_get_own_prio(void) {
    int task_id = am_pal_task_get_own_id();
    AM_ASSERT(AM_PAL_TASK_ID_MAIN != task_id);
    struct am_ao_state *me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao *ao = me->aos[i];
        if (ao && ao->task_id == task_id) {
            return ao->prio.ao;
        }
    }
    AM_ASSERT(0);
}
