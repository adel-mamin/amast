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
 * Timer API implementation.
 */

#include <string.h>
#include <stddef.h>

#include "common/macros.h"
#include "dlist/dlist.h"
#include "timer/timer.h"
#include "pal/pal.h"

/** Timer module descriptor. */
struct am_timer {
    /**
     * Timer event domains.
     * Each domain comprises a list of the timer events,
     * which belong to this domain.
     */
    struct am_dlist domains[AM_PAL_TICK_DOMAIN_MAX];
    /** timer module configuration */
    struct am_timer_cfg cfg;
};

static struct am_timer m_timer;

static void am_timer_crit_enter(void) {}
static void am_timer_crit_exit(void) {}

void am_timer_state_ctor(const struct am_timer_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->post || cfg->publish);

    struct am_timer *me = &m_timer;
    memset(me, 0, sizeof(*me));
    for (int i = 0; i < AM_COUNTOF(me->domains); ++i) {
        am_dlist_init(&me->domains[i]);
    }
    me->cfg = *cfg;
    if (!me->cfg.crit_enter || !me->cfg.crit_exit) {
        me->cfg.crit_enter = am_timer_crit_enter;
        me->cfg.crit_exit = am_timer_crit_exit;
    }
}

void am_timer_event_ctor(struct am_event_timer *event, int id, int domain) {
    AM_ASSERT(event);
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(domain < AM_PAL_TICK_DOMAIN_MAX);

    /* timer events are never deallocated */
    memset(event, 0, sizeof(*event));
    am_dlist_item_init(&event->item);
    event->event.id = id;
    event->event.tick_domain = (unsigned)domain & AM_EVENT_TICK_DOMAIN_MASK;
}

void am_timer_arm(
    struct am_event_timer *event, void *owner, int ticks, int interval
) {
    struct am_timer *me = &m_timer;

    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    /* make sure it wasn't already armed */
    AM_ASSERT(!am_dlist_item_is_linked(&event->item));
    AM_ASSERT(event->event.id > 0);
    AM_ASSERT(event->event.tick_domain < AM_COUNTOF(me->domains));
    AM_ASSERT(ticks >= 0);
    if (owner) {
        AM_ASSERT(me->cfg.post);
    } else {
        AM_ASSERT(me->cfg.publish);
    }

    event->owner = owner;
    event->shot_in_ticks = ticks;
    event->interval_ticks = interval;
    event->event.pubsub_time = owner ? false : true;

    me->cfg.crit_enter();
    am_dlist_push_back(&me->domains[event->event.tick_domain], &event->item);
    me->cfg.crit_exit();
}

bool am_timer_disarm(struct am_event_timer *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    struct am_timer *me = &m_timer;

    me->cfg.crit_enter();
    bool was_armed = am_dlist_pop(&event->item);
    me->cfg.crit_exit();
    event->shot_in_ticks = event->interval_ticks = 0;

    return was_armed;
}

bool am_timer_is_armed(const struct am_event_timer *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    return am_dlist_item_is_linked(&event->item);
}

void am_timer_tick(int domain) {
    struct am_timer *me = &m_timer;

    AM_ASSERT(domain < AM_COUNTOF(me->domains));

    struct am_dlist_iterator it;

    me->cfg.crit_enter();
    am_dlist_iterator_init(
        &me->domains[domain], &it, /*direction=*/AM_DLIST_FORWARD
    );

    struct am_dlist_item *p = NULL;
    while ((p = am_dlist_iterator_next(&it)) != NULL) {
        struct am_event_timer *timer =
            AM_CONTAINER_OF(p, struct am_event_timer, item);

        AM_ASSERT(timer->shot_in_ticks);
        --timer->shot_in_ticks;
        if (timer->shot_in_ticks) {
            continue;
        }
        struct am_event_timer *t = timer;
        if (me->cfg.update) {
            t = me->cfg.update(timer);
        }
        if (t->interval_ticks) {
            t->shot_in_ticks = t->interval_ticks;
        } else {
            am_dlist_iterator_pop(&it);
        }
        if (t->event.pubsub_time) {
            AM_ASSERT(me->cfg.publish);
            me->cfg.publish(&t->event);
        } else {
            AM_ASSERT(me->cfg.post);
            me->cfg.post(t->owner, &t->event);
        }
    }
    me->cfg.crit_exit();
}

struct am_event_timer *am_timer_event_allocate(int id, int size, int domain) {
    AM_ASSERT(size >= (int)sizeof(struct am_event_timer));
    struct am_event *e = am_event_allocate(id, size, /*margin=*/0);
    AM_DISABLE_WARNING(AM_W_CAST_ALIGN);
    struct am_event_timer *te = (struct am_event_timer *)e;
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN);
    am_timer_event_ctor(te, id, domain);
    return te;
}

bool am_timer_domain_is_empty(int domain) {
    struct am_timer *me = &m_timer;
    AM_ASSERT(domain >= 0);
    AM_ASSERT(domain < AM_COUNTOF(me->domains));
    return am_dlist_is_empty(&me->domains[domain]);
}
