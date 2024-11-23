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

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "blk/blk.h"
#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "dlist/dlist.h"
#include "hsm/hsm.h"
#include "onesize/onesize.h"
#include "queue/queue.h"
#include "slist/slist.h"
#include "bit/bit.h"
#include "ao/state.h"

#include "ao/ao.h"

/** Active object (AO module internal state instance. */
struct am_ao_state g_am_ao_state;

bool am_ao_publish_x(const struct am_event *event, int margin) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(AM_EVENT_HAS_PUBSUB_ID(event));
    AM_ASSERT(margin >= 0);

    struct am_ao_state *me = &g_am_ao_state;

    me->crit_enter();

    if (!am_event_is_static(event)) {
        /*
         * To avoid a potential race condition if higher priority
         * active object preempts the event publishing and frees the event
         * as processed.
         */
        am_event_inc_ref_cnt(event);
    }

    me->crit_exit();

    bool rc = true;

    /*
     * The event publishing is done for higher priority
     * active objects first to avoid priority inversion.
     */
    struct am_ao_subscribe_list *sub = &me->sub[event->id];
    for (int i = AM_COUNTOF(sub->list) - 1; i >= 0; i--) {
        unsigned list = sub->list[i];
        while (list) {
            int msb = am_bit_u8_msb((uint8_t)list);
            list &= ~(1U << (unsigned)msb);

            int ind = 8 * i + msb;
            struct am_ao *ao = me->ao[ind];
            AM_ASSERT(ao);
            bool pushed = am_event_push_back_x(ao, &ao->event_queue, event, margin);
            if (!pushed) {
                rc = false;
            }
        }
    }

    /*
     * Tries to free the event.
     * It is needed to balance the ref counter increment at the beginning of
     * ao_publish(). Also takes care of the case when no active objects
     * subscribed to this event.
     */
    am_event_free(event);

    return rc;
}

void am_ao_publish(const struct am_event *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(AM_EVENT_HAS_PUBSUB_ID(event));

    am_ao_publish_x(event, /*margin=*/0);
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

void am_ao_stop(const struct am_ao *ao) {
    AM_ASSERT(ao);
    AM_ASSERT(ao->prio < AM_AO_NUM_MAX);

    struct am_ao_state *me = &g_am_ao_state;
    me->ao[ao->prio] = NULL;
}

static void am_ao_on_idle(void) {}

static void am_ao_debug(const struct am_ao *ao, const struct am_event *e) {
    (void)ao;
    (void)e;
}

static void am_ao_crit_enter(void) {}
static void am_ao_crit_exit(void) {}

void am_ao_state_ctor(const struct am_ao_state_cfg *cfg) {
    AM_ASSERT(cfg);

    struct am_ao_state *me = &g_am_ao_state;
    memset(me, 0, sizeof(*me)); /* NOLINT */

    me->on_idle = cfg->on_idle;
    if (!me->on_idle) {
        me->on_idle = am_ao_on_idle;
    }
    me->debug = cfg->debug;
    if (!me->debug) {
        me->debug = am_ao_debug;
    }
    me->crit_enter = cfg->crit_enter;
    if (!me->crit_enter) {
        me->crit_enter = am_ao_crit_enter;
    }
    me->crit_exit = cfg->crit_exit;
    if (!me->crit_exit) {
        me->crit_exit = am_ao_crit_exit;
    }

    am_ao_port_ctor(&me->port);
}

void am_ao_state_dtor(void) {
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
        int len = am_queue_length(q);
        int cap = am_queue_capacity(q);
        int tnum = AM_MIN(num, len);
        if (0 == tnum) {
            log(ao->name, 0, len, cap, /*event=*/-1);
            continue;
        }
        for (int j = 0; j < tnum; j++) {
            struct am_event **e = (struct am_event **)am_queue_pop_front(q);
            AM_ASSERT(e);
            AM_ASSERT(*e);
            log(ao->name, j, len, cap, (*e)->id);
        }
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
        log(ao->name, /*event=*/ao->last_event);
    }
}