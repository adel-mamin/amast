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
#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "event/event_async.h"
#include "event/event_queue.h"
#include "pal/pal.h"
#include "ao/ao.h"
#include "state.h"

static struct am_bit_u64 am_ready_aos_ = {0};

void am_ao_global_init_(void) {
    memset(&am_ready_aos_, 0, sizeof(am_ready_aos_));
}

static bool am_ao_handle(void* ctx, const struct am_event* event) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    struct am_ao* ao = ctx;
    struct am_ao_state* me = &am_ao_state_;
    AM_ATOMIC_STORE_N(&ao->last_event, event->id);
    me->running_ao_prio = ao->prio;

    ao->user_event_handler(ao->ctx, event);

    me->running_ao_prio = AM_AO_PRIO_INVALID;
    AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_EMPTY);

    return true;
}

static void am_ao_stop_(struct am_ao* ao) {
    struct am_ao_state* me = &am_ao_state_;

    AM_ASSERT(ao->stop_requested);
    AM_ASSERT(me->aos_count);

    if (ao->user_deinit_handler) {
        me->running_ao_prio = AM_AO_PRIO_INVALID;
        ao->user_deinit_handler(ao->ctx);
        me->running_ao_prio = AM_AO_PRIO_INVALID;
    }

    am_event_async_unregister(&me->async_hub, ao->prio.ao);

    me->crit_enter();

    am_event_queue_flush_unsafe(&ao->event_queue);
    am_event_queue_deinit(&ao->event_queue);
    am_bit_u64_clear(&am_ready_aos_, ao->prio.ao);

    me->aos[ao->prio.ao] = NULL;

    ao->init_called = false;
    AM_ATOMIC_STORE_N(&ao->running, false);

    --me->aos_count;

    me->crit_exit();
}

bool am_ao_step(void) {
    struct am_ao_state* me = &am_ao_state_;

    bool init_complete = AM_ATOMIC_EXCHANGE_N(&me->init_complete, true);
    if (!init_complete) {
        am_task_init_wait();
    }

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
        struct am_ao* ao = me->aos[msb];
        AM_ASSERT(ao);
        AM_ASSERT(ao->prio.ao == msb);

        if (am_event_queue_is_empty_unsafe(&ao->event_queue)) {
            am_bit_u64_clear(&am_ready_aos_, ao->prio.ao);
            me->crit_exit();
            continue;
        }

        me->crit_exit();

        enum am_rc rc = am_event_queue_pop_front_with_cb(
            &ao->event_queue, am_ao_handle, ao
        );
        AM_ASSERT(AM_RC_OK == rc);

        if (ao->stop_requested) {
            am_ao_stop_(ao);
        }

        dispatched = true;
    } while (!dispatched);

    return dispatched;
}

void am_ao_run(void) {
    while (am_ao_get_count() > 0) {
        am_ao_step();
    }
}

void am_ao_start(struct am_ao* ao, struct am_ao_cfg* cfg) {
    AM_ASSERT(ao);
    AM_ASSERT(ao->init_called);
    AM_ASSERT(cfg);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(cfg->prio));
    AM_ASSERT(cfg->queue);
    AM_ASSERT(cfg->queue_size > 0);

    struct am_ao_state* me = &am_ao_state_;
    am_event_queue_init(
        &ao->event_queue, cfg->queue, cfg->queue_size, me->alloc
    );

    ao->prio = cfg->prio;
    ao->name = cfg->name;
    ao->task_id = am_task_get_own_id();

    AM_ASSERT(NULL == me->aos[cfg->prio.ao]);
    me->aos[cfg->prio.ao] = ao;
    ++me->aos_count;

    AM_ATOMIC_STORE_N(&ao->running, true);

    me->running_ao_prio = cfg->prio;

    am_event_async_register_with_id(
        &me->async_hub, am_ao_event_enqueue_unsafe, ao, ao->prio.ao
    );

    ao->stop_requested = false;

    if (ao->user_init_handler) {
        ao->user_init_handler(ao->ctx, cfg->init_event);
    }
    me->running_ao_prio = AM_AO_PRIO_INVALID;

    if (ao->stop_requested) {
        am_ao_stop_(ao);
    }
}

void am_ao_notify_unsafe(const struct am_ao* ao) {
    if (AM_TASK_ID_NONE == ao->task_id) {
        return;
    }
    am_bit_u64_set(&am_ready_aos_, ao->prio.ao);
    am_task_notify(ao->task_id);
}

void am_ao_notify(const struct am_ao* ao) {
    AM_ASSERT(ao);

    struct am_ao_state* me = &am_ao_state_;
    me->crit_enter();
    am_ao_notify_unsafe(ao);
    me->crit_exit();
}

int am_ao_get_own_prio(void) {
    const struct am_ao_state* me = &am_ao_state_;
    AM_ASSERT(AM_AO_PRIO_IS_VALID(me->running_ao_prio));
    return me->running_ao_prio.ao;
}
