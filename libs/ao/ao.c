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
 * Active Object (AO) API implementation.
 */

#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event_common.h"
#include "event/event_async.h"
#include "event/event_queue.h"
#include "pal/pal.h"
#include "ao/state.h"

#include "ao/ao.h"

/** Active object (AO) library internal state instance. */
struct am_ao_state am_ao_state_;

bool am_ao_event_queue_is_empty(struct am_ao* ao) {
    return am_event_queue_is_empty(&ao->event_queue);
}

bool am_ao_publish_exclude_x(
    const struct am_event* event, const struct am_ao* ao, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(margin >= 0);
    struct am_ao_state* me = &am_ao_state_;
    AM_ASSERT(AM_ATOMIC_LOAD_N(&me->startup_complete));

    struct am_event_queue_policy policy = {
        .lifo = 0,
        .margin = margin,
        .exclude_id = ao ? ao->prio.ao : AM_EVENT_PUBLISHER_ID_NONE
    };
    return am_event_async_publish(event, policy);
}

void am_ao_publish_exclude(
    const struct am_event* event, const struct am_ao* ao
) {
    am_ao_publish_exclude_x(event, ao, /*margin=*/0);
}

bool am_ao_publish_x(const struct am_event* event, int margin) {
    return am_ao_publish_exclude_x(event, /*ao=*/NULL, margin);
}

void am_ao_publish(const struct am_event* event) {
    am_ao_publish_exclude_x(event, /*ao=*/NULL, /*margin=*/0);
}

bool am_ao_post_fifo_x(
    struct am_ao* ao, const struct am_event* event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_ATOMIC_LOAD_N(&ao->ctor_called));
    AM_ASSERT(AM_ATOMIC_LOAD_N(&ao->running));
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);

    struct am_event_queue_policy policy = {.lifo = 0, .margin = margin};
    enum am_rc rc = am_event_queue_push(&ao->event_queue, event, policy);
    if (AM_RC_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify(ao);
    }
    return (AM_RC_OK == rc) || (AM_RC_QUEUE_WAS_EMPTY == rc);
}

void am_ao_post_fifo(struct am_ao* ao, const struct am_event* event) {
    bool posted = am_ao_post_fifo_x(ao, event, /*margin=*/0);
    AM_ASSERT(posted);
}

bool am_ao_post_lifo_x(
    struct am_ao* ao, const struct am_event* event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_ATOMIC_LOAD_N(&ao->ctor_called));
    AM_ASSERT(AM_ATOMIC_LOAD_N(&ao->running));
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);

    struct am_event_queue_policy policy = {.margin = margin};
    enum am_rc rc = am_event_queue_push(&ao->event_queue, event, policy);
    if (AM_RC_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify(ao);
    }
    return (AM_RC_OK == rc) || (AM_RC_QUEUE_WAS_EMPTY == rc);
}

void am_ao_post_lifo(struct am_ao* ao, const struct am_event* event) {
    bool posted = am_ao_post_lifo_x(ao, event, /*margin=*/0);
    AM_ASSERT(posted);
}

void am_ao_subscribe(const struct am_ao* ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));
    AM_ASSERT(event >= AM_EVT_USER);

    am_event_async_subscribe(ao->prio.ao, event);
}

void am_ao_unsubscribe(const struct am_ao* ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));
    AM_ASSERT(event >= AM_EVT_USER);

    am_event_async_unsubscribe(ao->prio.ao, event);
}

void am_ao_unsubscribe_all(const struct am_ao* ao) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));

    am_event_async_unsubscribe_all(ao->prio.ao);
}

void am_ao_ctor(
    struct am_ao* ao, am_ao_fn init_handler, am_ao_fn event_handler, void* ctx
) {
    AM_ASSERT(ao);
    AM_ASSERT(event_handler);
    AM_ASSERT(ctx);

    memset(ao, 0, sizeof(*ao));
    ao->user_init_handler = init_handler;
    ao->user_event_handler = event_handler;
    ao->ctx = ctx;
    ao->ctor_called = true;
}

void am_ao_state_ctor(const struct am_ao_state_cfg* cfg) {
    struct am_ao_state* me = &am_ao_state_;
    memset(me, 0, sizeof(*me));

    am_ao_state_ctor_();

    AM_ATOMIC_STORE_N(&me->startup_complete, false);

    if (cfg) {
        me->crit_enter = cfg->crit_enter;
        me->crit_exit = cfg->crit_exit;
        me->on_idle = cfg->on_idle;
        me->alloc = cfg->alloc;
    } else {
        me->crit_enter = am_crit_enter;
        me->crit_exit = am_crit_exit;
        me->on_idle = am_on_idle;
        me->alloc = NULL;
    }

    me->running_ao_prio = AM_AO_PRIO_INVALID;

    am_event_register_crit(me->crit_enter, me->crit_exit);
}

void am_ao_state_dtor(void) {}

void am_ao_crash_dump_event_queues_unsafe(
    int num,
    void (*log)(
        const char* name, int i, int len, int cap, const struct am_event* event
    )
) {
    AM_ASSERT(num != 0);
    AM_ASSERT(log);

    if (num < 0) {
        num = INT_MAX;
    }

    struct am_ao_state* me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao* ao = me->aos[i];
        if (!ao) {
            continue;
        }
        struct am_event_queue* q = &ao->event_queue;
        if (!am_event_queue_is_valid(q)) {
            continue;
        }
        const int cap = am_event_queue_get_capacity(q);
        const int nbusy = am_event_queue_get_nbusy_unsafe(q);
        const int tnum = AM_MIN(num, nbusy);
        if (0 == tnum) {
            log(ao->name, 0, nbusy, cap, /*event=*/NULL);
            continue;
        }
        for (int j = 0; j < tnum; ++j) {
            const struct am_event* e = am_event_queue_pop_front_unsafe(q);
            AM_ASSERT(e);
            log(ao->name, j, nbusy, cap, e);
        }
    }
}

void am_ao_log_last_events(void (*log)(const char* name, int event)) {
    AM_ASSERT(log);

    struct am_ao_state* me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao* ao = me->aos[i];
        if (!ao) {
            continue;
        }
        log(ao->name, /*event=*/AM_ATOMIC_LOAD_N(&ao->last_event));
    }
}

int am_ao_get_cnt(void) { return AM_ATOMIC_LOAD_N(&am_ao_state_.aos_cnt); }

enum am_rc am_ao_event_handler(
    void* ctx, const struct am_event* event, struct am_event_queue_policy policy
) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);
    AM_ASSERT(policy.margin >= 0);

    struct am_ao* ao = ctx;

    AM_ASSERT(AM_ATOMIC_LOAD_N(&ao->running));

    enum am_rc rc = am_event_queue_push_unsafe(&ao->event_queue, event, policy);
    if (AM_RC_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify_unsafe(ao);
    }
    return AM_RC_OK;
}
