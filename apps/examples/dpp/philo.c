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

#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "event/event_common.h"
#include "pal/pal.h"
#include "timer/timer.h"
#include "ao/ao.h"

#include "events.h"
#include "philo.h"

static struct philo {
    struct am_hsm hsm;
    struct am_ao ao;
    int id;
    int cnt;
    struct am_ao* table;
    struct am_timer* timer;
    struct am_event_alloc* alloc;
    struct am_timer_event_x timeout;
} m_philo[PHILO_NUM];

static struct am_event event_stopped_ = {.id = EVT_STOPPED};

static enum am_rc philo_top(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc philo_thinking(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc philo_hungry(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc philo_eating(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc philo_top(struct am_hsm* hsm, const struct am_event* event) {
    struct philo* me = AM_CONTAINER_OF(hsm, struct philo, hsm);
    switch (event->id) {
    case EVT_STOP:
        am_timer_disarm(me->timer, &me->timeout.event);
        am_ao_post_fifo(me->table, &event_stopped_);
        am_ao_stop(&me->ao);
        return am_hsm_handled(hsm);
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc philo_thinking(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct philo* me = AM_CONTAINER_OF(hsm, struct philo, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("philo %d is thinking\n", me->id);
        ++me->cnt;
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/20, 0);
        return am_hsm_handled(hsm);

    case EVT_TIMEOUT: {
        struct hungry* msg = (struct hungry*)am_event_allocate(
            me->alloc, EVT_HUNGRY, sizeof(struct hungry)
        );
        msg->philo = me->id;
        am_ao_post_fifo(me->table, &msg->event);
        return am_hsm_tran(hsm, philo_hungry);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, philo_top);
}

static enum am_rc philo_hungry(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct philo* me = AM_CONTAINER_OF(hsm, struct philo, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("philo %d is hungry\n", me->id);
        return am_hsm_handled(hsm);

    case EVT_EAT: {
        const struct eat* eat = (const struct eat*)event;
        if (eat->philo == me->id) {
            return am_hsm_tran(hsm, philo_eating);
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, philo_top);
}

static enum am_rc philo_eating(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct philo* me = AM_CONTAINER_OF(hsm, struct philo, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("philo %d is eating\n", me->id);
        am_timer_arm(me->timer, &me->timeout.event, /*ticks=*/20, 0);
        return am_hsm_handled(hsm);

    case EVT_TIMEOUT: {
        am_printf("philo %d publishing DONE\n", me->id);
        struct done* msg = (struct done*)am_event_allocate(
            me->alloc, EVT_DONE, sizeof(struct done)
        );
        msg->philo = me->id;
        am_ao_publish(AM_CAST(const struct am_event*, msg));
        return am_hsm_tran(hsm, philo_thinking);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, philo_top);
}

static enum am_rc philo_init(struct am_hsm* hsm, const struct am_event* event) {
    struct philo* me = AM_CONTAINER_OF(hsm, struct philo, hsm);
    (void)event;
    am_ao_subscribe(&me->ao, EVT_EAT);
    am_ao_subscribe(&me->ao, EVT_STOP);
    return am_hsm_tran(hsm, philo_thinking);
}

void philo_create(
    int id,
    struct am_ao* table,
    struct am_timer* timer,
    struct am_event_alloc* alloc
) {
    AM_ASSERT(id >= 0);
    AM_ASSERT(id < AM_COUNTOF(m_philo));
    AM_ASSERT(alloc);

    struct philo* me = &m_philo[id];
    memset(me, 0, sizeof(*me));
    me->cnt = 0;
    me->id = id;
    am_ao_create(
        &me->ao, (am_ao_fn)am_hsm_start, (am_ao_fn)am_hsm_dispatch, me
    );
    am_hsm_create(&me->hsm, am_hsm_state_make(philo_init));

    me->table = table;
    me->timer = timer;
    me->timeout = am_timer_event_create_x(EVT_TIMEOUT, &me->ao);
    me->alloc = alloc;
}

struct am_ao* philo_get_obj(int id) {
    AM_ASSERT(id >= 0);
    AM_ASSERT(id < AM_COUNTOF(m_philo));
    return &m_philo[id].ao;
}
