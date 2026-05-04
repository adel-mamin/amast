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
 * @file
 *
 * Dining philosophers problem (DPP)
 */

#include <stddef.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/constants.h"
#include "common/macros.h"
#include "event/event_async.h"
#include "event/event_common.h"
#include "event/event_pool.h"
#include "timer/timer.h"
#include "pal/pal.h"
#include "ao/ao.h"

#include "philo.h"
#include "table.h"
#include "events.h"

static struct am_event_alloc alloc;

const char* event_to_str(int id) {
    if (EVT_DONE == id) {
        return "DONE";
    }
    if (EVT_EAT == id) {
        return "EAT";
    }
    if (EVT_TIMEOUT == id) {
        return "TIMEOUT";
    }
    if (EVT_HUNGRY == id) {
        return "HUNGRY";
    }
    AM_ASSERT(0);
}

static void log_pool(
    int pool_index, int event_index, const struct am_event* event, int size
) {
    (void)size;
    am_printf(
        "pool %d index %d event %s (%p)\n",
        pool_index,
        event_index,
        event_to_str(event->id),
        (const void*)event
    );
}

static void log_queue(
    const char* name, int i, int len, int cap, const struct am_event* event
) {
    am_printf(
        "name %s, index %d, len %d cap %d event %s\n",
        name,
        i,
        len,
        cap,
        event ? event_to_str(event->id) : "NULL"
    );
}

AM_NORETURN void am_assert_failure(
    const char* assertion, const char* file, int line
) {
    static int nasserts = 0;
    if (AM_ATOMIC_LOAD_N(&nasserts)) {
        __builtin_trap();
    }
    AM_ATOMIC_FETCH_ADD(&nasserts, 1);
    am_printf_unsafe(
        AM_COLOR_RED "ASSERT: %s (%s:%d)(task %d)\n" AM_COLOR_RESET,
        assertion,
        file,
        line,
        am_task_get_own_id()
    );
    am_event_alloc_log_unsafe(&alloc, /*num=*/-1, log_pool);
    am_ao_crash_dump_event_queues_unsafe(/*num=*/-1, log_queue);
    am_pal_flush();
    __builtin_trap();
}

static void ticker_task(void* param) {
    struct am_timer* timer = param;

    am_task_startup_gate_wait();

    while (am_ao_get_cnt() > 0) {
        am_sleep_ticks(AM_TICK_DOMAIN_DEFAULT, /*ticks=*/1);

        am_timer_tick_iterator_init(timer);
        struct am_timer_event* fired = NULL;
        while ((fired = am_timer_tick_iterator_next(timer)) != NULL) {
            void* owner = AM_CAST(struct am_timer_event_x*, fired)->ctx;
            if (owner) {
                am_ao_post_fifo(owner, &fired->event);
            } else {
                am_ao_publish(&fired->event);
            }
        }
    }
}

int main(void) {
    am_pal_ctor(/*arg=*/NULL);

    am_event_alloc_init(&alloc);

    struct am_timer timer;
    am_timer_ctor(&timer);
    am_timer_register_cbs(&timer, am_crit_enter, am_crit_exit);

    struct am_event_subscribe_list pubsub_list[AM_AO_EVT_PUB_MAX];
    am_event_async_init(pubsub_list, AM_COUNTOF(pubsub_list), &alloc);

    char event_pool[3 * PHILO_NUM][128] AM_ALIGNED(AM_ALIGN_MAX);
    am_event_alloc_add_pool(
        &alloc,
        event_pool,
        sizeof(event_pool),
        sizeof(event_pool[0]),
        AM_ALIGN_MAX
    );

    struct am_ao_state_cfg cfg = {
        .crit_enter = am_crit_enter, .crit_exit = am_crit_exit, .alloc = &alloc
    };
    am_ao_state_ctor(&cfg);

    for (int i = 0; i < PHILO_NUM; ++i) {
        philo_ctor(i, table_get_obj(), &timer, &alloc);
    }
    table_ctor(/*nsessions=*/100, &alloc);

    const struct am_event* queue_table[2 * PHILO_NUM];

    am_ao_start(
        table_get_obj(),
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/queue_table,
        /*queue_size=*/AM_COUNTOF(queue_table), /* NOLINT */
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"table",
        /*init_event=*/NULL
    );

    static const char* names[PHILO_NUM] = {
        "philo0", "philo1", "philo2", "philo3", "philo4"
    };

    const struct am_event* queue_philo[PHILO_NUM][2 * PHILO_NUM];

    for (int i = 0; i < AM_COUNTOF(names); ++i) {
        unsigned char prio = (unsigned char)(AM_AO_PRIO_MIN + i);
        am_ao_start(
            philo_get_obj(i),
            (struct am_ao_prio){.ao = prio, .task = prio},
            /*queue=*/queue_philo[i],
            /*queue_size=*/AM_COUNTOF(queue_philo[i]),
            /*stack=*/NULL,
            /*stack_size=*/0,
            /*name=*/names[i],
            /*init_event=*/NULL
        );
    }

    am_task_create(
        "ticker",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*flags=*/0,
        /*arg=*/&timer
    );

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ao_state_dtor();

    am_pal_dtor();

    return 0;
}
