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

#ifdef AMAST_AO_COOPERATIVE

#include <stdbool.h>
#include <stddef.h>

#include "bit/bit.h"
#include "blk/blk.h"
#include "common/alignment.h"
#include "common/macros.h"
#include "hsm/hsm.h"
#include "queue/queue.h"
#include "event/event.h"
#include "ao/ao.h"
#include "state.h"

bool am_ao_run_all(bool loop) {
    struct am_ao_state *me = &g_am_ao_state;
    bool processed = false;
    do {
        me->crit_enter();

        if (am_bit_u64_is_empty(&me->ready_aos)) {
            me->on_idle();
            me->crit_exit();
            continue;
        }
        int msb = am_bit_u64_msb(&me->ready_aos);
        struct am_ao *ao = me->ao[msb];
        AM_ASSERT(ao);

        me->crit_exit();

        const struct am_event *e = am_event_pop_front(ao, &ao->event_queue);
        me->debug(ao, e);
        AM_ATOMIC_STORE_N(&ao->last_event, e->id);
        am_hsm_dispatch(&ao->hsm, e);
        AM_ATOMIC_STORE_N(&ao->last_event, AM_EVT_INVALID);
        am_event_free(&e);
        processed = true;
    } while (loop);

    return processed;
}

void am_ao_start(
    struct am_ao *ao,
    int prio,
    const struct am_event *queue[],
    int queue_size,
    void *stack,
    int stack_size,
    const char *name,
    const struct am_event *init_event
) {
    (void)stack;
    (void)stack_size;

    AM_ASSERT(ao);
    AM_ASSERT(prio >= 0);
    AM_ASSERT(prio < AM_AO_NUM_MAX);
    AM_ASSERT(queue);
    AM_ASSERT(queue_size > 0);

    struct am_blk blk = {
        .ptr = queue, .size = (int)sizeof(struct am_event *) * queue_size
    };

    am_queue_init(
        &ao->event_queue,
        sizeof(struct am_event *),
        AM_ALIGNOF(struct am_event *),
        &blk
    );

    ao->prio = prio;
    ao->name = name;

    struct am_ao_state *me = &g_am_ao_state;
    AM_ASSERT(NULL == me->ao[prio]);
    me->ao[prio] = ao;
    am_hsm_init(&ao->hsm, init_event);
}

void am_ao_notify(void *ao) {
    am_bit_u64_set(&g_am_ao_state.ready_aos, ((struct am_ao *)ao)->prio);
}

void am_ao_notify_event_queue_empty(void *ao) {
    am_bit_u64_clear(&g_am_ao_state.ready_aos, ((struct am_ao *)ao)->prio);
}

#endif /* AMAST_AO_COOPERATIVE */
