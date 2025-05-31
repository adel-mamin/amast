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

/** Active object (AO) library internal state instance. */
struct am_ao_state am_ao_state_;

bool am_ao_event_queue_is_empty(struct am_ao *ao) {
    struct am_ao_state *me = &am_ao_state_;
    me->crit_enter();
    bool empty = am_queue_is_empty(&ao->event_queue);
    me->crit_exit();
    return empty;
}

bool am_ao_publish_exclude_x(
    const struct am_event *event, const struct am_ao *ao, int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(AM_EVENT_HAS_PUBSUB_ID(event));
    AM_ASSERT(margin >= 0);
    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->subscribe_list_set);
    AM_ASSERT(me->sub);

    if (!am_event_is_static(event)) {
        /*
         * To avoid a potential race condition, if higher priority
         * active object preempts the event publishing and frees the event
         * as processed.
         */
        am_event_inc_ref_cnt(event);
    }

    bool all_published = true;

    /*
     * The event publishing is done for higher priority
     * active objects first to avoid priority inversion.
     */
    struct am_ao_subscribe_list *sub = &me->sub[event->id];
    for (int i = AM_COUNTOF(sub->list) - 1; i >= 0; --i) {
        me->crit_enter();
        unsigned list = sub->list[i];
        me->crit_exit();
        while (list) {
            int msb = am_bit_u8_msb((uint8_t)list);
            list &= ~(1U << (unsigned)msb);

            int ind = 8 * i + msb;
            struct am_ao *ao_ = me->aos[ind];
            AM_ASSERT(ao_);
            if (ao_ == ao) {
                continue;
            }
            enum am_event_rc rc =
                am_event_push_back_x(&ao_->event_queue, event, margin);
            if (AM_EVENT_RC_ERR == rc) {
                if (0 == margin) {
                    AM_ASSERT(0);
                }
                all_published = false;
                continue;
            }
            if (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc) {
                am_ao_notify(ao_);
            }
        }
    }

    /*
     * Tries to free the event.
     * It is needed to balance the ref counter increment at the beginning of
     * the function. Also takes care of the case when no active objects
     * subscribed to this event.
     */
    am_event_free(&event);

    return all_published;
}

void am_ao_publish_exclude(
    const struct am_event *event, const struct am_ao *ao
) {
    am_ao_publish_exclude_x(event, ao, /*margin=*/0);
}

bool am_ao_publish_x(const struct am_event *event, int margin) {
    return am_ao_publish_exclude_x(event, /*ao=*/NULL, margin);
}

void am_ao_publish(const struct am_event *event) {
    am_ao_publish_exclude_x(event, /*ao=*/NULL, /*margin=*/0);
}

bool am_ao_post_fifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);

    enum am_event_rc rc = am_event_push_back_x(&ao->event_queue, event, margin);
    if (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify(ao);
    }
    return (AM_EVENT_RC_OK == rc) || (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc);
}

static void am_ao_post_fifo_unsafe(
    struct am_ao *ao, const struct am_event *event
) {
    AM_ASSERT(ao);
    AM_ASSERT(event);

    enum am_event_rc rc = am_event_push_back_unsafe(&ao->event_queue, event);
    if (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify_unsafe(ao);
    }
}

void am_ao_post_fifo(struct am_ao *ao, const struct am_event *event) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    bool posted = am_ao_post_fifo_x(ao, event, /*margin=*/0);
    AM_ASSERT(posted);
}

bool am_ao_post_lifo_x(
    struct am_ao *ao, const struct am_event *event, int margin
) {
    AM_ASSERT(ao);
    AM_ASSERT(event);
    AM_ASSERT(margin >= 0);

    enum am_event_rc rc =
        am_event_push_front_x(&ao->event_queue, event, margin);
    if (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc) {
        am_ao_notify(ao);
    }
    return (AM_EVENT_RC_OK == rc) || (AM_EVENT_RC_OK_QUEUE_WAS_EMPTY == rc);
}

void am_ao_post_lifo(struct am_ao *ao, const struct am_event *event) {
    bool posted = am_ao_post_lifo_x(ao, event, /*margin=*/0);
    AM_ASSERT(posted);
}

void am_ao_subscribe(const struct am_ao *ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));
    AM_ASSERT(event >= AM_EVT_USER);
    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->subscribe_list_set);
    AM_ASSERT(event < me->nsub);
    AM_ASSERT(me->aos[ao->prio.ao] == ao);
    AM_ASSERT(me->sub);

    int i = ao->prio.ao / 8;

    me->crit_enter();

    me->sub[event].list[i] |= (uint8_t)(1U << (unsigned)(ao->prio.ao % 8));

    me->crit_exit();
}

void am_ao_unsubscribe(const struct am_ao *ao, int event) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));
    AM_ASSERT(event >= AM_EVT_USER);
    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->subscribe_list_set);
    AM_ASSERT(event < me->nsub);
    AM_ASSERT(me->aos[ao->prio.ao] == ao);
    AM_ASSERT(me->sub);

    int ind = event - AM_EVT_USER;
    int i = ao->prio.ao / 8;

    me->crit_enter();

    unsigned list = me->sub[ind].list[i];
    list &= ~(1U << (unsigned)(ao->prio.ao % 8));
    me->sub[ind].list[i] = (uint8_t)list;

    me->crit_exit();
}

void am_ao_unsubscribe_all(const struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(AM_AO_PRIO_IS_VALID(ao->prio));

    struct am_ao_state *me = &am_ao_state_;
    AM_ASSERT(me->subscribe_list_set);
    if (!me->sub) {
        return;
    }

    AM_ASSERT(me->aos[ao->prio.ao] == ao);

    int j = ao->prio.ao / 8;

    for (int i = 0; i < me->nsub; ++i) {
        me->crit_enter();

        unsigned list = me->sub[i].list[j];
        list &= ~(1U << (unsigned)(ao->prio.ao % 8));
        me->sub[i].list[j] = (uint8_t)list;

        me->crit_exit();
    }
}

void am_ao_ctor(struct am_ao *ao, struct am_hsm_state state) {
    AM_ASSERT(ao);

    memset(ao, 0, sizeof(*ao));
    am_hsm_ctor(&ao->hsm, state);
    ao->ctor_called = true;
}

static void am_ao_debug_stub(const struct am_ao *ao, const struct am_event *e) {
    (void)ao;
    (void)e;
}

void am_ao_state_ctor(const struct am_ao_state_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->crit_enter);
    AM_ASSERT(cfg->crit_exit);

    struct am_ao_state *me = &am_ao_state_;
    memset(me, 0, sizeof(*me));

    am_pal_ctor(/*arg=*/NULL);

    am_ao_state_ctor_();

    me->startup_complete = false;

    me->debug = cfg->debug;
    if (!me->debug) {
        me->debug = am_ao_debug_stub;
    }
    me->crit_enter = cfg->crit_enter;
    me->crit_exit = cfg->crit_exit;
    me->on_idle = cfg->on_idle;

    me->running_ao_prio = AM_AO_PRIO_INVALID;

    struct am_event_state_cfg cfg_event = {
        .crit_enter = cfg->crit_enter,
        .crit_exit = cfg->crit_exit,
    };
    am_event_state_ctor(&cfg_event);

    struct am_timer_state_cfg cfg_timer = {
        .post_unsafe = (am_timer_post_unsafe_fn)am_ao_post_fifo_unsafe,
        .crit_enter = cfg->crit_enter,
        .crit_exit = cfg->crit_exit,
    };
    am_timer_state_ctor(&cfg_timer);
}

void am_ao_state_dtor(void) { am_pal_dtor(); }

void am_ao_init_subscribe_list(struct am_ao_subscribe_list *sub, int nsub) {
    AM_ASSERT(sub);
    AM_ASSERT(nsub >= AM_EVT_USER);

    struct am_ao_state *me = &am_ao_state_;
    me->sub = sub;
    me->nsub = nsub;
    memset(sub, 0, sizeof(*sub) * (size_t)nsub);
    me->subscribe_list_set = true;
}

void am_ao_log_event_queues(
    int num,
    void (*log)(
        const char *name, int i, int len, int cap, const struct am_event *event
    )
) {
    AM_ASSERT(num != 0);
    AM_ASSERT(log);

    if (num < 0) {
        num = INT_MAX;
    }

    struct am_ao_state *me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao *ao = me->aos[i];
        if (!ao) {
            continue;
        }
        struct am_queue *q = &ao->event_queue;
        if (!am_queue_is_valid(q)) {
            continue;
        }
        const int cap = am_queue_get_capacity(q);
        const int nbusy = am_queue_get_nbusy(q);
        const int tnum = AM_MIN(num, nbusy);
        if (0 == tnum) {
            log(ao->name, 0, nbusy, cap, /*event=*/NULL);
            continue;
        }
        for (int j = 0; j < tnum; ++j) {
            struct am_event **e = am_queue_pop_front(q);
            AM_ASSERT(e);
            AM_ASSERT(*e);
            log(ao->name, j, nbusy, cap, *e);
        }
    }
}

void am_ao_log_last_events(void (*log)(const char *name, int event)) {
    AM_ASSERT(log);

    struct am_ao_state *me = &am_ao_state_;
    for (int i = 0; i < AM_COUNTOF(me->aos); ++i) {
        struct am_ao *ao = me->aos[i];
        if (!ao) {
            continue;
        }
        log(ao->name, /*event=*/AM_ATOMIC_LOAD_N(&ao->last_event));
    }
}

int am_ao_get_cnt(void) {
    struct am_ao_state *me = &am_ao_state_;
    me->crit_enter();
    int cnt = me->aos_cnt;
    me->crit_exit();
    return cnt;
}
