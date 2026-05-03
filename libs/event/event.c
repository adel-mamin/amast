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

#include "event/event_common.h"
#include "common/macros.h"
#include "onesize/onesize.h"

/** Event reference counter bit mask. */
#define AM_EVENT_REF_COUNTER_MASK \
    ((1U << (unsigned)AM_EVENT_REF_COUNTER_BITS) - 1U)
/** Maximum value of reference counter. */
#define AM_EVENT_REF_COUNTER_MAX AM_EVENT_REF_COUNTER_MASK

/** Event ID least significant word bit mask. */
#define AM_EVENT_ID_LSW_MASK ((1U << (unsigned)AM_EVENT_ID_LSW_BITS) - 1U)

/** Pool index bit mask. */
#define AM_EVENT_POOL_INDEX_MASK \
    ((1U << (unsigned)AM_EVENT_POOL_INDEX_BITS) - 1U)
/** Maximum pool index value */
#define AM_EVENT_POOL_INDEX_MAX ((int)AM_EVENT_POOL_INDEX_MASK)
/** Is AM_EVENT_POOLS_NUM_MAX too large? */
AM_ASSERT_STATIC(AM_EVENT_POOLS_NUM_MAX <= (AM_EVENT_POOL_INDEX_MAX + 1));

static void am_event_crit_stub(void) {}

/** enter critical section */
void (*am_event_crit_enter)(void) = am_event_crit_stub;
/** exit critical section */
void (*am_event_crit_exit)(void) = am_event_crit_stub;

void am_event_register_crit(void (*crit_enter)(void), void (*crit_exit)(void)) {
    AM_ASSERT(crit_enter);
    AM_ASSERT(crit_exit);

    am_event_crit_enter = crit_enter;
    am_event_crit_exit = crit_exit;
}

struct am_event* am_event_allocate_x(
    struct am_event_alloc* alloc, int id, int size, int margin
) {
    AM_ASSERT(size > 0);
    AM_ASSERT(alloc);
    AM_ASSERT(alloc->npools > 0);
    AM_ASSERT(alloc->npools <= AM_EVENT_POOL_INDEX_MAX);
    int maxind = alloc->npools - 1;
    AM_ASSERT(size <= am_onesize_get_block_size(&alloc->pools[maxind]));
    AM_ASSERT(id >= AM_EVT_USER);
    AM_ASSERT(margin >= 0);

    /* find allocator using binary search */
    int left = 0;
    int right = maxind;
    while (left < right) {
        int mid = (left + right) / 2;
        int onesize = am_onesize_get_block_size(&alloc->pools[mid]);
        if (size > onesize) {
            left = mid + 1;
        } else if (size < onesize) {
            right = mid;
        } else {
            left = mid;
            break;
        }
    }
    am_event_crit_enter();
    struct am_event* event = am_onesize_allocate_x(&alloc->pools[left], margin);
    am_event_crit_exit();

    if (!event) { /* cppcheck-suppress knownConditionTrueFalse */
        return NULL;
    }

    memset(event, 0, sizeof(*event));
    event->id = (uint16_t)id;
    event->id_lsw = (uint16_t)id & AM_EVENT_ID_LSW_MASK;
    event->pool_index_plus_one =
        (unsigned)(left + 1) & AM_EVENT_POOL_INDEX_MASK;

    return event;
}

struct am_event* am_event_allocate(
    struct am_event_alloc* alloc, int id, int size
) {
    struct am_event* event = am_event_allocate_x(alloc, id, size, /*margin=*/0);
    AM_ASSERT(event);

    return event;
}

void am_event_free_unsafe(
    struct am_event_alloc* alloc, const struct am_event* event
) {
    if (am_event_is_static(event)) {
        return; /* the event is statically allocated */
    }

    AM_ASSERT(alloc);

    struct am_event* e = AM_CAST(struct am_event*, event);
    AM_ASSERT(e->pool_index_plus_one <= AM_EVENT_POOLS_NUM_MAX);
    /*
     * Check if event is valid.
     * If the below assert hits, then the reason is likely
     * a double free condition.
     */
    AM_ASSERT(((uint32_t)e->id & AM_EVENT_ID_LSW_MASK) == e->id_lsw);

    if (e->ref_counter > 1) {
        --e->ref_counter;
        return;
    }

    am_onesize_free(&alloc->pools[e->pool_index_plus_one - 1], e);
}

void am_event_free(struct am_event_alloc* alloc, const struct am_event* event) {
    AM_ASSERT(event);

    am_event_crit_enter();
    am_event_free_unsafe(alloc, event);
    am_event_crit_exit();
}

struct am_event* am_event_dup_x(
    struct am_event_alloc* alloc,
    const struct am_event* event,
    int size,
    int margin
) {
    AM_ASSERT(event);
    AM_ASSERT(size >= (int)sizeof(struct am_event));
    AM_ASSERT(alloc);
    AM_ASSERT(alloc->npools > 0);
    AM_ASSERT(event->id >= AM_EVT_USER);
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
    AM_ASSERT(margin >= 0);

    struct am_event* dup = am_event_allocate_x(alloc, event->id, size, margin);
    if (dup && (size > (int)sizeof(struct am_event))) {
        char* dst = (char*)&dup[1];
        const char* src = (const char*)&event[1];
        size_t sz = (size_t)size - sizeof(struct am_event);
        memcpy(dst, src, sz);
    }

    return dup;
}

struct am_event* am_event_dup(
    struct am_event_alloc* alloc, const struct am_event* event, int size
) {
    struct am_event* dup = am_event_dup_x(alloc, event, size, /*margin=*/0);
    AM_ASSERT(dup);

    return dup;
}

bool am_event_is_static(const struct am_event* event) {
    AM_ASSERT(event);

    return (0 == (event->pool_index_plus_one & AM_EVENT_POOL_INDEX_MASK));
}

void am_event_inc_ref_cnt(const struct am_event* event) {
    AM_ASSERT(event);

    if (am_event_is_static(event)) {
        return;
    }

    /*
     * Check if event is valid.
     * If the below assert hits, then the reason is likely
     * a use after free condition.
     */
    AM_ASSERT(((uint32_t)event->id & AM_EVENT_ID_LSW_MASK) == event->id_lsw);

    struct am_event* e = AM_CAST(struct am_event*, event);

    am_event_crit_enter();

    AM_ASSERT(event->ref_counter < AM_EVENT_REF_COUNTER_MAX);
    ++e->ref_counter;

    am_event_crit_exit();
}

void am_event_dec_ref_cnt(
    struct am_event_alloc* alloc, const struct am_event* event
) {
    AM_ASSERT(event);
    am_event_free(alloc, event);
}

int am_event_get_ref_cnt(
    struct am_event_alloc* alloc, const struct am_event* event
) {
    (void)alloc;
    AM_ASSERT(event);

    am_event_crit_enter();
    int cnt = event->ref_counter;
    am_event_crit_exit();

    return cnt;
}
