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
 * @file
 *
 * Active Object (AO) API implementation.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/macros.h"
#include "hsm/hsm.h"
#include "event/event.h"
#include "queue/queue.h"
#include "bit/bit.h"
#include "timer/timer.h"
#include "pal/pal.h"
#include "ao/state.h"

#include "ao/ao.h"

/** Active object (AO) module internal state instance. */
struct am_ao_state g_am_ao_state;

static bool am_ao_event_queue_is_empty(struct am_ao *ao) {
    struct am_ao_state *me = &g_am_ao_state;
    me->crit_enter();
    bool empty = am_queue_is_empty(&ao->event_queue);
    me->crit_exit();
    return empty;
}

bool am_ao_publish_x(const struct am_event **event, int margin) {
    AM_ASSERT(event);
    AM_ASSERT(*event);
    const struct am_event *e = *event;
    AM_ASSERT(AM_EVENT_HAS_USER_ID(e));
    AM_ASSERT(AM_EVENT_HAS_PUBSUB_ID(e));
    AM_ASSERT(margin >= 0);

    struct am_ao_state *me = &g_am_ao_state;

    if (!am_event_is_static(e)) {
        /*
         * To avoid a potential race condition if higher priority
         * active object preempts the event publishing and frees the event
         * as processed.
         */
        am_event_inc_ref_cnt(e);
    }

    bool rc = true;

    /*
     * The event publishing is done for higher priority
     * active objects first to avoid priority inversion.
     */
    struct am_ao_subscribe_list *sub = &me->sub[e->id];
    for (int i = AM_COUNTOF(sub->list) - 1; i >= 0; i--) {
        me->crit_enter();
        unsigned list = sub->list[i];
        me->crit_exit();
        while (list) {
            int msb = am_bit_u8_msb((uint8_t)list);
            list &= ~(1U << (unsigned)msb);

            int ind = 8 * i + msb;
            struct am_ao *ao = me->ao[ind];
            AM_ASSERT(ao);
            bool was_empty = am_ao_event_queue_is_empty(ao);
            bool pushed = am_event_push_back_x(&ao->event_queue, event, margin);
            if (!pushed) {
                rc = false;
                continue;
            }
            if (was_empty) {
                am_ao_notify(ao);
            }
        }
    }

    /*
     * Tries to free the event.
     * It is needed to balance the ref counter increment at the beginning of
     * the function. Also takes care of the case when no active objects
     * subscribed to this event.
     */
    am_event_free(event);

    return rc;
}

void am_ao_publish(const struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(AM_EVENT_HAS_PUBSUB_ID(event));

    am_ao_publish_x(&event, /*margin=*/0);
}

void am_ao_post_fifo(struct am_ao *ao, const struct am_event *event) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    bool was_empty = am_ao_event_queue_is_empty(ao);
    am_event_push_back(&ao->event_queue, event);
    if (was_empty) {
        am_ao_notify(ao);
    }
}

bool am_ao_post_fifo_x(
    struct am_ao *ao, const struct am_event **event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);
    bool was_empty = am_ao_event_queue_is_empty(ao);
    bool posted = am_event_push_back_x(&ao->event_queue, event, margin);
    if (posted && was_empty) {
        am_ao_notify(ao);
    }
    return posted;
}

void am_ao_post_lifo(struct am_ao *ao, const struct am_event *event) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    bool was_empty = am_ao_event_queue_is_empty(ao);
    am_event_push_front(&ao->event_queue, event);
    if (was_empty) {
        am_ao_notify(ao);
    }
}

bool am_ao_post_lifo_x(
    struct am_ao *ao, const struct am_event **event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);
    bool was_empty = am_ao_event_queue_is_empty(ao);
    bool posted = am_event_push_front_x(&ao->event_queue, event, margin);
    if (posted && was_empty) {
        am_ao_notify(ao);
    }
    return posted;
}

void am_ao_subscribe(const struct am_ao *ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao));
    AM_ASSERT(event >= AM_EVT_USER);
    AM_ASSERT(event < g_am_ao_state.nsub);
    struct am_ao_state *me = &g_am_ao_state;
    AM_ASSERT(me->ao[ao->prio] == ao);

    int i = ao->prio / 8;

    me->crit_enter();

    me->sub[event].list[i] |= (uint8_t)(1U << (unsigned)(ao->prio % 8));

    me->crit_exit();
}

void am_ao_unsubscribe(const struct am_ao *ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao));
    AM_ASSERT(event >= AM_EVT_USER);
    AM_ASSERT(event < g_am_ao_state.nsub);
    struct am_ao_state *me = &g_am_ao_state;
    AM_ASSERT(me->ao[ao->prio] == ao);

    int ind = event - AM_EVT_USER;
    int i = ao->prio / 8;

    me->crit_enter();

    unsigned list = me->sub[ind].list[i];
    list &= ~(1U << (unsigned)(ao->prio % 8));
    me->sub[ind].list[i] = (uint8_t)list;

    me->crit_exit();
}

void am_ao_unsubscribe_all(const struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao));

    struct am_ao_state *me = &g_am_ao_state;
    AM_ASSERT(me->ao[ao->prio] == ao);

    int j = ao->prio / 8;

    for (int i = 0; i < me->nsub; i++) {
        me->crit_enter();

        unsigned list = me->sub[i].list[j];
        list &= ~(1U << (unsigned)(ao->prio % 8));
        me->sub[i].list[j] = (uint8_t)list;

        me->crit_exit();
    }
}

void am_ao_ctor(struct am_ao *ao, const struct am_hsm_state *state) {
    AM_ASSERT(ao);
    AM_ASSERT(state);
    am_hsm_ctor(&ao->hsm, state);
}

void am_ao_stop(struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(ao->prio < AM_AO_NUM_MAX);

    am_ao_unsubscribe_all(ao);
    AM_ATOMIC_STORE_N(&ao->stopped, true);
    struct am_ao_state *me = &g_am_ao_state;
    me->ao[ao->prio] = NULL;
}

static void am_ao_on_idle_stub(void) {}

static void am_ao_debug_stub(const struct am_ao *ao, const struct am_event *e) {
    (void)ao;
    (void)e;
}

void am_ao_state_ctor(const struct am_ao_state_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->crit_enter);
    AM_ASSERT(cfg->crit_exit);

    struct am_ao_state *me = &g_am_ao_state;
    memset(me, 0, sizeof(*me));

    me->startup_mutex = am_pal_mutex_create();
    am_pal_mutex_lock(me->startup_mutex);

    me->on_idle = cfg->on_idle;
    if (!me->on_idle) {
        me->on_idle = am_ao_on_idle_stub;
    }
    me->debug = cfg->debug;
    if (!me->debug) {
        me->debug = am_ao_debug_stub;
    }
    me->crit_enter = cfg->crit_enter;
    me->crit_exit = cfg->crit_exit;

    am_pal_ctor();

    struct am_event_cfg cfg_event = {
        .push_front = (am_event_push_front_fn)am_ao_post_lifo,
        .crit_enter = cfg->crit_enter,
        .crit_exit = cfg->crit_exit,
    };
    am_event_state_ctor(&cfg_event);

    struct am_timer_cfg cfg_timer = {
        .post = (am_timer_post_fn)am_ao_post_fifo,
        .crit_enter = cfg->crit_enter,
        .crit_exit = cfg->crit_exit,
    };
    am_timer_state_ctor(&cfg_timer);
}

void am_ao_state_dtor(void) {
    struct am_ao_state *me = &g_am_ao_state;
    AM_ATOMIC_STORE_N(&me->ao_state_dtor_called, true);
}

void am_ao_init_subscribe_list(struct am_ao_subscribe_list *sub, int nsub) {
    AM_ASSERT(sub);
    AM_ASSERT(nsub > 0);

    struct am_ao_state *me = &g_am_ao_state;
    me->sub = sub;
    me->nsub = nsub;
    memset(sub, 0, sizeof(*sub) * (size_t)nsub);
}

void am_ao_dump_event_queues(
    int num, void (*log)(const char *name, int i, int len, int cap, int event)
) {
    AM_ASSERT(log);

    struct am_ao_state *me = &g_am_ao_state;
    for (int i = 0; i < AM_COUNTOF(me->ao); i++) {
        struct am_ao *ao = me->ao[i];
        if (!ao) {
            continue;
        }
        struct am_queue *q = &ao->event_queue;
        int cap = am_queue_capacity(q);
        me->crit_enter();
        int len = am_queue_length(q);
        int tnum = AM_MIN(num, len);
        if (0 == tnum) {
            me->crit_exit();
            log(ao->name, 0, len, cap, /*event=*/AM_EVT_INVALID);
            continue;
        }
        for (int j = 0; j < tnum; j++) {
            struct am_event **e = (struct am_event **)am_queue_pop_front(q);
            AM_ASSERT(e);
            AM_ASSERT(*e);
            log(ao->name, j, len, cap, (*e)->id);
        }
        me->crit_exit();
    }
}

void am_ao_log_last_events(void (*log)(const char *name, int event)) {
    AM_ASSERT(log);

    struct am_ao_state *me = &g_am_ao_state;
    for (int i = 0; i < AM_COUNTOF(me->ao); i++) {
        struct am_ao *ao = me->ao[i];
        if (!ao) {
            continue;
        }
        log(ao->name, /*event=*/AM_ATOMIC_LOAD_N(&ao->last_event));
    }
}
