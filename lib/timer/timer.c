/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
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

void timer_ctor(struct timer *hnd, const struct timer_cfg *cfg) {
    ASSERT(cfg);
    ASSERT(cfg->post);

    memset(hnd, 0, sizeof(*hnd));
    for (int i = 0; i < COUNTOF(hnd->domains); ++i) {
        dlist_init(&hnd->domains[i]);
    }
    hnd->cfg = *cfg;
}

void timer_event_ctor(struct event_timer *event, int id, int domain) {
    ASSERT(event);
    ASSERT(id >= EVT_USER);
    ASSERT(domain < TICK_DOMAIN_MAX);

    memset(event, 0, sizeof(*event));
    dlist_item_init(&event->item);
    event->event.id = id;
    event->event.tick_domain = domain;
}

static void timer_arm(
    struct timer *hnd,
    struct event_timer *event,
    void *owner,
    int ticks,
    int interval
) {
    ASSERT(hnd);
    ASSERT(event);
    /* make sure it wasn't already armed */
    ASSERT(!dlist_item_is_linked(&event->item));
    ASSERT(EVENT_HAS_USER_ID(event));
    ASSERT(event->event.tick_domain < COUNTOF(hnd->domains));
    ASSERT(ticks >= 0);

    event->owner = owner;
    event->shot_in_ticks = ticks;
    event->interval_ticks = interval;
    event->event.pubsub_time = owner ? false : true;
    dlist_push_back(&hnd->domains[event->event.tick_domain], &event->item);
}

void timer_post_in_ticks(
    struct timer *hnd, struct event_timer *event, void *owner, int ticks
) {
    timer_arm(hnd, event, owner, ticks, /*interval=*/0);
}

void timer_publish_in_ticks(
    struct timer *hnd, struct event_timer *event, int ticks
) {
    timer_arm(hnd, event, /*owner=*/NULL, ticks, /*interval=*/0);
}

void timer_post_every_ticks(
    struct timer *hnd, struct event_timer *event, void *owner, int ticks
) {
    timer_arm(hnd, event, owner, ticks, /*interval=*/ticks);
}

void timer_publish_every_ticks(
    struct timer *hnd, struct event_timer *event, int ticks
) {
    timer_arm(hnd, event, /*owner=*/NULL, ticks, /*interval=*/ticks);
}

bool onc_timer_disarm(struct event_timer *event) {
    ASSERT(event);
    ASSERT(EVENT_HAS_USER_ID(event));

    bool was_armed = dlist_pop(&event->item);
    event->shot_in_ticks = event->interval_ticks = 0;

    return was_armed;
}

bool timer_is_armed(const struct event_timer *event) {
    ASSERT(event);
    ASSERT(EVENT_HAS_USER_ID(event));
    return dlist_item_is_linked(&event->item);
}

bool timer_any_armed(const struct timer *hnd, int domain) {
    ASSERT(hnd);
    ASSERT(domain >= 0);
    ASSERT(domain <= TICK_DOMAIN_MAX);

    if (domain < TICK_DOMAIN_MAX) {
        return !dlist_is_empty(&hnd->domains[domain]);
    }
    for (int i = 0; i < COUNTOF(hnd->domains); ++i) {
        if (!dlist_is_empty(&hnd->domains[i])) {
            return true;
        }
    }
    return false;
}

void timer_tick(struct timer *hnd, int domain) {
    ASSERT(domain < COUNTOF(hnd->domains));

    struct dlist_iterator it;
    dlist_iterator_init(
        &hnd->domains[domain], &it, /*direction=*/DLIST_FORWARD
    );

    struct dlist_item *p = NULL;
    while ((p = dlist_iterator_next(&it)) != NULL) {
        struct event_timer *timer = CONTAINER_OF(p, struct event_timer, item);

        ASSERT(timer->shot_in_ticks);
        --timer->shot_in_ticks;
        if (timer->shot_in_ticks) {
            continue;
        }
        struct event_timer *t = timer;
        if (hnd->cfg.update) {
            t = hnd->cfg.update(timer);
        }
        if (t->interval_ticks) {
            t->shot_in_ticks = t->interval_ticks;
        } else {
            dlist_iterator_pop(&it);
        }
        if (t->event.pubsub_time) {
            ASSERT(hnd->cfg.publish);
            hnd->cfg.publish(&t->event);
        } else {
            ASSERT(hnd->cfg.post);
            hnd->cfg.post(t->owner, &t->event);
        }
    }
}
