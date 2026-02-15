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

#include <stdbool.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "pal/pal.h"
#include "ao/ao.h"

#include "events.h"
#include "table.h"
#include "philo.h"

enum { PHILO_DONE, PHILO_HUNGRY, PHILO_EATING };

static struct table {
    /*
     * Must be the first member of the structure.
     * See https://amast.readthedocs.io/hsm.html#hsm-coding-rules for details
     */
    struct am_hsm hsm;
    struct am_ao ao;
    int philo[PHILO_NUM];
    int nsessions;
    int nstops;
} m_table;

struct am_ao *g_ao_table = &m_table.ao;

static struct am_event event_stop_ = {.id = EVT_STOP};

static int philo_is_eating(int philo) {
    AM_ASSERT(philo >= 0);
    AM_ASSERT(philo < AM_COUNTOF(m_table.philo));
    return PHILO_EATING == m_table.philo[philo];
}

static int philo_is_hungry(int philo) {
    AM_ASSERT(philo >= 0);
    AM_ASSERT(philo < AM_COUNTOF(m_table.philo));
    return PHILO_HUNGRY == m_table.philo[philo];
}

#define LEFT(n) ((n) ? ((n) - 1) : (PHILO_NUM - 1))
#define RIGHT(n) (((PHILO_NUM - 1) == (n)) ? 0 : ((n) + 1))

static void philo_mark_done(int philo) {
    AM_ASSERT(philo >= 0);
    AM_ASSERT(philo < AM_COUNTOF(m_table.philo));
    m_table.philo[philo] = PHILO_DONE;
}

static void philo_mark_hungry(int philo) {
    AM_ASSERT(philo >= 0);
    AM_ASSERT(philo < AM_COUNTOF(m_table.philo));
    m_table.philo[philo] = PHILO_HUNGRY;
}

static void philo_mark_eating(int philo) {
    AM_ASSERT(philo >= 0);
    AM_ASSERT(philo < AM_COUNTOF(m_table.philo));
    m_table.philo[philo] = PHILO_EATING;
}

static int table_can_serve(int philo) {
    if (philo_is_eating(LEFT(philo))) {
        return 0;
    }
    if (philo_is_eating(RIGHT(philo))) {
        return 0;
    }
    return 1;
}

static void table_serve(int philo) {
    struct eat *eat =
        (struct eat *)am_event_allocate(EVT_EAT, sizeof(struct eat));
    eat->philo = philo;
    am_printf("table serving philo %d\n", philo);
    am_ao_publish(AM_CAST(const struct am_event *, eat));
    philo_mark_eating(philo);

    if (m_table.nsessions) {
        --m_table.nsessions;
        am_printf("table session %d\n", m_table.nsessions);
    }
}

static enum am_rc table_stopping(
    struct table *me, const struct am_event *event
) {
    switch (event->id) {
    case EVT_STOPPED: {
        ++me->nstops;
        if (me->nstops == PHILO_NUM) {
            am_ao_stop(&me->ao);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static bool table_sessions_are_over(const struct table *me) {
    return me->nsessions == 0;
}

static enum am_rc table_serving(
    struct table *me, const struct am_event *event
) {
    switch (event->id) {
    case EVT_HUNGRY: {
        const struct hungry *hungry = (const struct hungry *)event;
        AM_ASSERT(!philo_is_hungry(hungry->philo));
        if (table_can_serve(hungry->philo)) {
            table_serve(hungry->philo);
            if (table_sessions_are_over(me)) {
                am_ao_publish(&event_stop_);
                return AM_HSM_TRAN(table_stopping);
            }
            return AM_HSM_HANDLED();
        }
        philo_mark_hungry(hungry->philo);
        return AM_HSM_HANDLED();
    }
    case EVT_DONE: {
        const struct done *done = (const struct done *)event;
        AM_ASSERT(philo_is_eating(done->philo));
        am_printf("table: philo %d is done\n", done->philo);
        philo_mark_done(done->philo);
        int left = LEFT(done->philo);
        if (philo_is_hungry(left)) {
            if (table_can_serve(left)) {
                table_serve(left);
            }
        }
        int right = RIGHT(done->philo);
        if (philo_is_hungry(right)) {
            if (table_can_serve(right)) {
                table_serve(right);
            }
        }
        if (table_sessions_are_over(me)) {
            am_ao_publish(&event_stop_);
            return AM_HSM_TRAN(table_stopping);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc table_init(struct table *me, const struct am_event *event) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_DONE);
    return AM_HSM_TRAN(table_serving);
}

void table_ctor(int nsessions) {
    struct table *me = &m_table;
    memset(me, 0, sizeof(*me));
    for (int i = 0; i < AM_COUNTOF(me->philo); ++i) {
        me->philo[i] = PHILO_DONE;
    }
    am_ao_ctor(&me->ao, (am_ao_fn)am_hsm_init, (am_ao_fn)am_hsm_dispatch, me);
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(table_init));
    me->nsessions = nsessions;
}
