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

#include "common/compiler.h"
#include "common/macros.h"
#include "hsm/hsm.h"
#include "event/event.h"
#include "pal/pal.h"
#include "timer/timer.h"
#include "ao/ao.h"

#include "events.h"
#include "philo.h"
#include "table.h"

static struct philo {
    struct am_ao ao;
    int id;
    int cnt;
    struct am_timer *timer;
} m_philo[PHILO_NUM];

struct am_ao *g_ao_philo[PHILO_NUM] = {
    &m_philo[0].ao,
    &m_philo[1].ao,
    &m_philo[2].ao,
    &m_philo[3].ao,
    &m_philo[4].ao,
};

static struct am_event event_stopped_ = {.id = EVT_STOPPED};

static int philo_top(struct philo *me, const struct am_event *event);
static int philo_thinking(struct philo *me, const struct am_event *event);
static int philo_hungry(struct philo *me, const struct am_event *event);
static int philo_eating(struct philo *me, const struct am_event *event);

static int philo_top(struct philo *me, const struct am_event *event) {
    switch (event->id) {
    case EVT_STOP:
        am_timer_disarm(me->timer);
        am_ao_post_fifo(g_ao_table, &event_stopped_);
        am_ao_stop(&me->ao);
        return AM_HSM_HANDLED();
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static int philo_thinking(struct philo *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_pal_printf("philo %d is thinking\n", me->id);
        ++me->cnt;
        am_timer_arm_ms(me->timer, /*ms=*/20, /*interval=*/0);
        return AM_HSM_HANDLED();

    case EVT_TIMEOUT: {
        struct hungry *msg = (struct hungry *)am_event_allocate(
            EVT_HUNGRY, sizeof(struct hungry)
        );
        msg->philo = me->id;
        am_ao_post_fifo(g_ao_table, &msg->event);
        return AM_HSM_TRAN(philo_hungry);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(philo_top);
}

static int philo_hungry(struct philo *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_pal_printf("philo %d is hungry\n", me->id);
        return AM_HSM_HANDLED();

    case EVT_EAT: {
        const struct eat *eat = (const struct eat *)event;
        if (eat->philo == me->id) {
            return AM_HSM_TRAN(philo_eating);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(philo_top);
}

static int philo_eating(struct philo *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_pal_printf("philo %d is eating\n", me->id);
        am_timer_arm_ms(me->timer, /*ms=*/20, /*interval=*/0);
        return AM_HSM_HANDLED();

    case EVT_TIMEOUT: {
        am_pal_printf("philo %d publishing DONE\n", me->id);
        struct done *msg =
            (struct done *)am_event_allocate(EVT_DONE, sizeof(struct done));
        msg->philo = me->id;
        am_ao_publish(AM_CAST(const struct am_event *, msg));
        return AM_HSM_TRAN(philo_thinking);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(philo_top);
}

static int philo_init(struct philo *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_EAT);
    am_ao_subscribe(&me->ao, EVT_STOP);
    return AM_HSM_TRAN(philo_thinking);
}

void philo_ctor(int id) {
    AM_ASSERT(id >= 0);
    AM_ASSERT(id < AM_COUNTOF(m_philo));

    struct philo *me = &m_philo[id];
    memset(me, 0, sizeof(*me));
    me->cnt = 0;
    me->id = id;
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(philo_init));

    me->timer = (struct am_timer *)am_timer_allocate(
        /*id=*/EVT_TIMEOUT,
        /*size=*/sizeof(struct am_timer),
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT,
        &me->ao
    );
}
