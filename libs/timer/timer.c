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
 * Timer API implementation.
 */

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "common/macros.h"
#include "slist/slist.h"
#include "timer/timer.h"
#include "pal/pal.h"

/** Timer library state descriptor. */
struct am_timer_state {
    /**
     * Timer event domains.
     * Each domain comprises a list of the timer events,
     * which belong to this domain.
     * Each domain has a unique tick rate.
     */
    struct am_slist domains[AM_PAL_TICK_DOMAIN_MAX];
    /**
     * Pending timer event domains.
     *
     * Each armed timer is first placed into corresponding
     * list in this array and then moved to struct am_timer::domains
     * in next am_timer_tick() call.
     *
     * This is done to limit struct am_timer::domains[] list operations
     * exclusively to am_timer_tick() call to avoid race conditions
     * between timer owners the ticker thread/ISR.
     */
    struct am_slist domains_pend[AM_PAL_TICK_DOMAIN_MAX];
    /** timer library configuration */
    struct am_timer_state_cfg cfg;
};

static struct am_timer_state am_timer_;

void am_timer_state_ctor(const struct am_timer_state_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->post || cfg->publish);
    AM_ASSERT(cfg->crit_enter);
    AM_ASSERT(cfg->crit_exit);

    struct am_timer_state *me = &am_timer_;
    memset(me, 0, sizeof(*me));
    for (int i = 0; i < AM_COUNTOF(me->domains); ++i) {
        am_slist_ctor(&me->domains[i]);
        am_slist_ctor(&me->domains_pend[i]);
    }
    me->cfg = *cfg;
}

void am_timer_ctor(struct am_timer *timer, int id, int domain, void *owner) {
    AM_ASSERT(timer);
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(domain < AM_PAL_TICK_DOMAIN_MAX);

    /* timer events are never deallocated */
    memset(timer, 0, sizeof(*timer));
    am_slist_item_ctor(&timer->item);
    timer->event.id = id;
    timer->event.tick_domain = (unsigned)domain & AM_EVENT_TICK_DOMAIN_MASK;
    timer->owner = owner;
}

void am_timer_arm_ticks(struct am_timer *timer, int ticks, int interval) {
    struct am_timer_state *me = &am_timer_;

    AM_ASSERT(timer);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(timer));
    AM_ASSERT(timer->event.id > 0);
    AM_ASSERT(timer->event.tick_domain < AM_COUNTOF(me->domains));
    AM_ASSERT(ticks >= 0);
    if (timer->owner) {
        AM_ASSERT(me->cfg.post);
    } else {
        AM_ASSERT(me->cfg.publish);
    }

    me->cfg.crit_enter();

    timer->shot_in_ticks = AM_MAX(ticks, 1);
    timer->interval_ticks = interval;
    timer->event.pubsub_time = (timer->owner == NULL);
    timer->disarm_pending = 0;

    if (!am_slist_item_is_linked(&timer->item)) {
        am_slist_push_back(
            &me->domains_pend[timer->event.tick_domain], &timer->item
        );
    }

    me->cfg.crit_exit();
}

void am_timer_arm_ms(struct am_timer *timer, int ms, int interval) {
    AM_ASSERT(timer);
    AM_ASSERT(ms >= 0);
    AM_ASSERT(interval >= 0);
    int domain = timer->event.tick_domain;
    int ticks = (int)am_pal_time_get_tick_from_ms(domain, (uint32_t)ms);
    int interval_ticks =
        (int)am_pal_time_get_tick_from_ms(domain, (uint32_t)interval);
    am_timer_arm_ticks(timer, ticks, interval_ticks);
}

bool am_timer_disarm(struct am_timer *timer) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(timer));

    struct am_timer_state *me = &am_timer_;

    me->cfg.crit_enter();

    bool was_armed = am_slist_item_is_linked(&timer->item);
    timer->shot_in_ticks = timer->interval_ticks = 0;
    timer->disarm_pending = 1;

    me->cfg.crit_exit();

    return was_armed;
}

bool am_timer_is_armed(const struct am_timer *timer) {
    AM_ASSERT(timer);
    AM_ASSERT(AM_EVENT_HAS_USER_ID(timer));
    struct am_timer_state *me = &am_timer_;

    me->cfg.crit_enter();

    bool armed = am_slist_item_is_linked(&timer->item);
    bool disarm_pending = timer->disarm_pending;

    me->cfg.crit_exit();

    return armed && !disarm_pending;
}

void am_timer_tick(int domain) {
    struct am_timer_state *me = &am_timer_;

    AM_ASSERT(domain < AM_COUNTOF(me->domains));

    struct am_slist_iterator it;

    me->cfg.crit_enter();
    if (!am_slist_is_empty(&me->domains_pend[domain])) {
        am_slist_append(&me->domains[domain], &me->domains_pend[domain]);
    }
    am_slist_iterator_ctor(&me->domains[domain], &it);

    struct am_slist_item *p = NULL;
    while ((p = am_slist_iterator_next(&it)) != NULL) {
        struct am_timer *timer = AM_CONTAINER_OF(p, struct am_timer, item);

        if (timer->disarm_pending) {
            am_slist_iterator_pop(&it);
            timer->disarm_pending = 0;
            me->cfg.crit_exit();
            me->cfg.crit_enter();
            continue;
        }

        AM_ASSERT(timer->shot_in_ticks);
        --timer->shot_in_ticks;
        if (timer->shot_in_ticks) {
            me->cfg.crit_exit();
            me->cfg.crit_enter();
            continue;
        }
        struct am_timer *t = timer;
        if (me->cfg.update) {
            me->cfg.crit_exit();
            t = me->cfg.update(timer);
            me->cfg.crit_enter();
        }
        if (t->interval_ticks) {
            t->shot_in_ticks = t->interval_ticks;
        } else {
            am_slist_iterator_pop(&it);
        }
        if (t->event.pubsub_time) {
            AM_ASSERT(me->cfg.publish);
            me->cfg.crit_exit();
            me->cfg.publish(&t->event);
            me->cfg.crit_enter();
        } else if (!timer->disarm_pending) {
            AM_ASSERT(me->cfg.post);
            me->cfg.post(t->owner, &t->event);
        }
    }
    me->cfg.crit_exit();
}

struct am_timer *am_timer_allocate(int id, int size, int domain, void *owner) {
    AM_ASSERT(size >= (int)sizeof(struct am_timer));
    struct am_event *e = am_event_allocate(id, size);
    AM_DISABLE_WARNING(AM_W_CAST_ALIGN);
    struct am_timer *te = (struct am_timer *)e;
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN);
    am_timer_ctor(te, id, domain, owner);

    return te;
}

bool am_timer_domain_is_empty(int domain) {
    struct am_timer_state *me = &am_timer_;
    AM_ASSERT(domain >= 0);
    AM_ASSERT(domain < AM_COUNTOF(me->domains));

    bool empty = am_slist_is_empty(&me->domains[domain]);
    bool empty_pend = am_slist_is_empty(&me->domains_pend[domain]);

    return empty && empty_pend;
}

int am_timer_get_ticks(const struct am_timer *timer) {
    struct am_timer_state *me = &am_timer_;

    me->cfg.crit_enter();
    int ticks = timer->shot_in_ticks;
    me->cfg.crit_exit();

    return ticks;
}

int am_timer_get_interval(const struct am_timer *timer) {
    struct am_timer_state *me = &am_timer_;

    me->cfg.crit_enter();
    int interval = timer->interval_ticks;
    me->cfg.crit_exit();

    return interval;
}
