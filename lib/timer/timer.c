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
 * timer API implementation
 */

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "dlist/dlist.h"
#include "timer/timer.h"

/** Timer module descriptor. */
struct timer {
    /**
     * Timer event domains.
     * Each domain comprises a list of the timer events,
     * which belong to this domain.
     */
    struct am_dlist domains[AM_TICK_DOMAIN_MAX];
    /** timer module configuration */
    struct am_timer_cfg cfg;
};

static struct timer m_timer;

void am_timer_ctor(const struct am_timer_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->post);

    memset(&m_timer, 0, sizeof(m_timer));
    for (int i = 0; i < AM_COUNTOF(m_timer.domains); ++i) {
        am_dlist_init(&m_timer.domains[i]);
    }
    m_timer.cfg = *cfg;
}

void am_timer_event_ctor(struct am_event_timer *event, int id, int domain) {
    AM_ASSERT(event);
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(domain < AM_TICK_DOMAIN_MAX);

    memset(event, 0, sizeof(*event));
    am_dlist_item_init(&event->item);
    event->event.id = id;
    event->event.tick_domain = domain;
}

void am_timer_arm(
    struct am_event_timer *event, void *owner, int ticks, int interval
) {
    struct timer *me = &m_timer;

    AM_ASSERT(event);
    /* make sure it wasn't already armed */
    AM_ASSERT(!am_dlist_item_is_linked(&event->item));
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    AM_ASSERT(event->event.tick_domain < AM_COUNTOF(me->domains));
    AM_ASSERT(ticks >= 0);

    event->owner = owner;
    event->shot_in_ticks = ticks;
    event->interval_ticks = interval;
    event->event.pubsub_time = owner ? false : true;
    am_dlist_push_back(&me->domains[event->event.tick_domain], &event->item);
}

bool am_timer_disarm(struct am_event_timer *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));

    bool was_armed = am_dlist_pop(&event->item);
    event->shot_in_ticks = event->interval_ticks = 0;

    return was_armed;
}

bool am_timer_is_armed(const struct am_event_timer *event) {
    AM_ASSERT(event);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(event));
    return am_dlist_item_is_linked(&event->item);
}

bool am_timer_any_armed(int domain) {
    AM_ASSERT(domain >= 0);
    AM_ASSERT(domain <= AM_TICK_DOMAIN_MAX);

    struct timer *me = &m_timer;
    if (domain < AM_TICK_DOMAIN_MAX) {
        return !am_dlist_is_empty(&me->domains[domain]);
    }
    for (int i = 0; i < AM_COUNTOF(me->domains); ++i) {
        if (!am_dlist_is_empty(&me->domains[i])) {
            return true;
        }
    }
    return false;
}

void am_timer_tick(int domain) {
    struct timer *me = &m_timer;

    AM_ASSERT(domain < AM_COUNTOF(me->domains));

    struct am_dlist_iterator it;
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
}

int am_timer_ticks_to_ms(int ticks) {
    struct timer *me = &m_timer;
    AM_ASSERT(me->cfg.ticks_to_ms);
    return me->cfg.ticks_to_ms(ticks);
}
