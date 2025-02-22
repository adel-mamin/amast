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

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "queue/queue.h"
#include "event/event.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

void am_ao_state_ctor_(void) {}

static void am_ao_task(void *param) {
    AM_ASSERT(param);

    am_ao_wait_start_all();

    struct am_ao *ao = (struct am_ao *)param;

    am_hsm_init(&ao->hsm, ao->init_event);

    struct am_ao_state *me = &am_ao_state_;
    while (AM_LIKELY(me->aos[ao->prio])) {
        me->crit_enter();
        while (am_queue_is_empty(&ao->event_queue)) {
            me->crit_exit();
            am_pal_task_wait(ao->task_id);
            me->crit_enter();
        }
        me->crit_exit();
        const struct am_event *e = am_event_pop_front(&ao->event_queue);
        AM_ASSERT(e);
        me->debug(ao, e);

        AM_ATOMIC_STORE_N(&ao->last_event, e->id);
        am_hsm_dispatch(&ao->hsm, e);
        AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_INVALID);

        am_event_free(&e);
    }
}

bool am_ao_run_all(void) {
    struct am_ao_state *me = &am_ao_state_;

    if (!me->startup_complete) {
        /* start all AOs */
        am_pal_mutex_unlock(me->startup_mutex);
        me->startup_complete = true;
    }
    am_pal_run_all(AM_PAL_RUN_ALL_DEFAULT);
    /* wait all AOs to complete */
    am_pal_task_wait(AM_PAL_TASK_ID_MAIN);
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
    AM_ASSERT(ao->ctor_called);
    AM_ASSERT(prio >= AM_AO_PRIO_MIN);
    AM_ASSERT(prio <= AM_AO_PRIO_MAX);
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    struct am_blk blk = {
        .ptr = queue, .size = (int)sizeof(struct am_event *) * queue_size
    };

    am_queue_ctor(
        &ao->event_queue, sizeof(struct am_event *), AM_ALIGNOF_EVENT_PTR, &blk
    );

    ao->prio = prio;
    ao->name = name;
    ao->init_event = init_event;

    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(NULL == me->aos[prio]);
    me->aos[prio] = ao;

    me->crit_enter();
    ++me->aos_cnt;
    me->crit_exit();

    ao->task_id = am_pal_task_create(
        name,
        prio,
        stack,
        stack_size,
        /*entry=*/am_ao_task,
        /*arg=*/ao
    );
}

void am_ao_stop(struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(ao->prio < AM_AO_NUM_MAX);
    int task_id = am_pal_task_get_own_id();
    AM_ASSERT(task_id == ao->task_id); /* check API description */
    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->aos_cnt);

    if (me->subscribe_list_set) {
        am_ao_unsubscribe_all(ao);
    }

    me->crit_enter();

    struct am_event **event = NULL;
    while ((event = (struct am_event **)am_queue_pop_front(&ao->event_queue)) !=
           NULL) {
        me->crit_exit();
        const struct am_event *e = *event;
        AM_ASSERT(e);
        am_event_free(&e);
        me->crit_enter();
    }
    am_queue_dtor(&ao->event_queue);

    me->aos[ao->prio] = NULL;
    --me->aos_cnt;
    bool running_aos = me->aos_cnt;

    me->crit_exit();

    ao->ctor_called = false;

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

void am_ao_wait_start_all(void) {
    const struct am_ao_state *me = &am_ao_state_;
    am_pal_mutex_lock(me->startup_mutex);
    am_pal_mutex_unlock(me->startup_mutex);
}

int am_ao_get_own_prio(void) {
    int task_id = am_pal_task_get_own_id();
    AM_ASSERT(AM_PAL_TASK_ID_MAIN != task_id);
    struct am_ao_state *me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao *ao = me->aos[i];
        if (ao && ao->task_id == task_id) {
            return ao->prio;
        }
    }
    AM_ASSERT(0);
}
