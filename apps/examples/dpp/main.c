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
 * Dining philosophers problem (DPP)
 */

#include <stddef.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "event/event.h"
#include "ao/ao.h"

#include "philo.h"
#include "table.h"
#include "events.h"

static const struct am_event *m_queue_philo[PHILO_NUM][2 * PHILO_NUM];
static const struct am_event *m_queue_table[2 * PHILO_NUM];
static char m_event_pool[2 * PHILO_NUM][128] AM_ALIGNED(AM_ALIGNOF_MAX);
static struct am_ao_subscribe_list m_pubsub_list[AM_AO_EVT_PUB_MAX];

const char *event_to_str(int id) {
    if (EVT_DONE == id) return "DONE";
    if (EVT_EAT == id) return "EAT";
    if (EVT_TIMEOUT == id) return "TIMEOUT";
    if (EVT_HUNGRY == id) return "HUNGRY";
    AM_ASSERT(0);
}

int main(void) {
    struct am_ao_state_cfg cfg_ao = {0};
    am_ao_state_ctor(&cfg_ao);

    am_event_add_pool(
        m_event_pool,
        sizeof(m_event_pool),
        sizeof(m_event_pool[0]),
        AM_ALIGNOF_MAX
    );

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    for (int i = 0; i < PHILO_NUM; i++) {
        philo_ctor(i);
    }
    table_ctor(/*nsession=*/100);

    static const char *names[PHILO_NUM] = {
        "philo0", "philo1", "philo2", "philo3", "philo4"
    };

    am_ao_start(
        g_ao_table,
        /*prio=*/AM_AO_PRIO_MAX,
        /*queue=*/m_queue_table,
        /*nqueue=*/AM_COUNTOF(m_queue_table), /* NOLINT */
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"table",
        /*init_event=*/NULL
    );

    for (int i = 0; i < AM_COUNTOF(names); i++) {
        am_ao_start(
            g_ao_philo[i],
            /*prio=*/AM_AO_PRIO_MIN + i,
            /*queue=*/m_queue_philo[i],
            /*nqueue=*/AM_COUNTOF(m_queue_philo[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/names[i],
            /*init_event=*/NULL
        );
    }

    am_ao_run_all(/*loop=*/1);

    return 0;
}
