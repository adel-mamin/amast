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
 * Event library API implementation.
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "common/types.h"
#include "event/event_common.h"
#include "event/event_queue.h"

#include "common/macros.h"

void am_event_queue_ctor(
    struct am_event_queue* queue,
    const struct am_event* events[],
    int nevents,
    struct am_event_alloc* alloc
) {
    AM_ASSERT(queue);
    AM_ASSERT(events);
    AM_ASSERT(nevents > 0);

    memset(queue, 0, sizeof(*queue));

    queue->events = events;
    queue->capacity = nevents;
    queue->nfree = queue->nfree_min = queue->capacity;
    queue->ctor_called = true;
    queue->alloc = alloc;
}

void am_event_queue_dtor(struct am_event_queue* queue) {
    AM_ASSERT(queue);
    memset(queue, 0, sizeof(*queue));
}

bool am_event_queue_is_valid(const struct am_event_queue* queue) {
    return queue->ctor_called;
}

bool am_event_queue_is_empty_unsafe(const struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return (queue->rd == queue->wr) && !queue->full;
}

bool am_event_queue_is_empty(const struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    am_event_crit_enter();
    bool empty = am_event_queue_is_empty_unsafe(queue);
    am_event_crit_exit();

    return empty;
}

int am_event_queue_get_nbusy_unsafe(const struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->capacity - queue->nfree;
}

int am_event_queue_get_capacity(const struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    return queue->capacity;
}

const struct am_event* am_event_queue_pop_front_unsafe(
    struct am_event_queue* queue
) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    if (am_event_queue_is_empty_unsafe(queue)) {
        return NULL;
    }
    const struct am_event* event = queue->events[queue->rd];
    queue->rd = (queue->rd + 1) % queue->capacity;
    queue->full = 0;
    ++queue->nfree;

    return event;
}

const struct am_event* am_event_queue_pop_front(struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    am_event_crit_enter();
    const struct am_event* event = am_event_queue_pop_front_unsafe(queue);
    am_event_crit_exit();

    return event;
}

int am_event_queue_get_nfree_min(const struct am_event_queue* queue) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);

    am_event_crit_enter();
    int min = queue->nfree_min;
    am_event_crit_exit();

    return min;
}

enum am_rc am_event_queue_push_back(
    struct am_event_queue* queue, const struct am_event* event
) {
    return am_event_queue_push(queue, event, AM_EVENT_QUEUE_POLICY_DEFAULT);
}

enum am_rc am_event_queue_push_front(
    struct am_event_queue* queue, const struct am_event* event
) {
    struct am_event_queue_policy policy = {.lifo = 1};
    return am_event_queue_push(queue, event, policy);
}

enum am_rc am_event_queue_push(
    struct am_event_queue* queue,
    const struct am_event* event,
    struct am_event_queue_policy policy
) {
    am_event_crit_enter();
    enum am_rc rc = am_event_queue_push_unsafe(queue, event, policy);
    am_event_crit_exit();

    return rc;
}

enum am_rc am_event_queue_push_unsafe(
    struct am_event_queue* queue,
    const struct am_event* event,
    struct am_event_queue_policy policy
) {
    AM_ASSERT(queue);
    AM_ASSERT(queue->ctor_called);
    AM_ASSERT(event);
    AM_ASSERT(policy.margin >= 0);
    AM_ASSERT(policy.margin < queue->capacity);

    if (!am_event_is_static(event)) {
        /*
         * Check if event is valid.
         * If the below assert hits, then the reason is likely
         * a use after free condition.
         */
        AM_ASSERT(
            ((uint32_t)event->id & AM_EVENT_ID_LSW_MASK) == event->id_lsw
        );
    }

    struct am_event* e = AM_CAST(struct am_event*, event);

    if (queue->nfree <= policy.margin) {
        am_event_free_unsafe(queue->alloc, event);
        return AM_RC_ERR;
    }
    AM_ASSERT(queue->nfree > 0);
    AM_ASSERT(!queue->full);

    if (!am_event_is_static(e)) {
        AM_ASSERT(e->ref_counter < AM_EVENT_REF_COUNTER_MAX);
        ++e->ref_counter;
    }

    if (policy.lifo) {
        queue->rd = queue->rd ? (queue->rd - 1) : (queue->capacity - 1);
        queue->events[queue->rd] = event;
    } else {
        queue->events[queue->wr] = event;
        queue->wr = (queue->wr + 1) % queue->capacity;
    }
    if (queue->wr == queue->rd) {
        queue->full = 1;
    }

    bool was_empty = queue->capacity == queue->nfree;

    --queue->nfree;
    queue->nfree_min = AM_MIN(queue->nfree, queue->nfree_min);

    if (was_empty) {
        return AM_RC_QUEUE_WAS_EMPTY;
    }
    return AM_RC_OK;
}

enum am_rc am_event_queue_pop_front_with_cb(
    struct am_event_queue* queue, am_event_handler_fn cb, void* ctx
) {
    AM_ASSERT(queue);

    const struct am_event* event = am_event_queue_pop_front(queue);
    if (!event) {
        return AM_RC_ERR;
    }
    const int id = event->id;

    if (cb) {
        cb(ctx, event);
    }

    /*
     * Event was freed / corrupted ?
     *
     * One possible reason could be the following usage scenario:
     *
     *  const struct am_event *e = am_event_allocate(alloc, id, size);
     *  am_event_inc_ref_cnt(e); <-- THIS IS MISSING
     *  am_hsm_dispatch(hsm, e);
     *      am_event_queue_push_XXX(...) & am_event_queue_pop_front_with_cb(...)
     *      OR
     *      am_event_inc_ref_cnt(e) & am_event_dec_ref_cnt(alloc, e)
     *  am_event_free(&e);
     */
    AM_ASSERT(id == event->id); /* cppcheck-suppress knownArgument */

    if (am_event_is_static(event)) {
        return AM_RC_OK;
    }

    am_event_free(queue->alloc, event);

    return AM_RC_OK;
}

int am_event_queue_flush(struct am_event_queue* queue) {
    int cnt = 0;
    const struct am_event* e = NULL;
    while ((e = am_event_queue_pop_front(queue)) != NULL) {
        ++cnt;
        AM_ASSERT(cnt <= queue->capacity);
        am_event_free(queue->alloc, e);
    }
    return cnt;
}
