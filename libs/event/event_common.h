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
 * Event API declaration.
 */

#ifndef AM_EVENT_COMMON_H_INCLUDED
#define AM_EVENT_COMMON_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "common/types.h"
#include "onesize/onesize.h"
/* #include "event/event_pool.h" */

/** The maximum number of event handlers */
#define AM_EVT_HANDLERS_NUM_MAX 64

/**
 * Empty event.
 *
 * User event handlers should take care of not causing any side effects
 * when called with this event.
 *
 * The event handlers must return the AM_HSM_SUPER() in response
 * to this event in HSM library.
 */
#define AM_EVT_EMPTY 0

/**
 * Init event.
 *
 * Run optional initial transition from a given state.
 *
 * Always follows the #AM_EVT_ENTRY event.
 */
#define AM_EVT_INIT 1

/**
 * Entry event.
 *
 * Run entry action(s) for a given state.
 *
 * No state transition is allowed in response to this event.
 */
#define AM_EVT_ENTRY 2

/**
 * Exit event.
 *
 * Run exit action(s) for a given state.
 *
 * No state transition is allowed in response to this event.
 */
#define AM_EVT_EXIT 3

/**
 * The event IDs smaller than this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER 4

/**
 * Check if event has a valid user event ID.
 *
 * @param event   the event to check
 * @retval true   the event has valid user event ID
 * @retval false  the event does not have valid user event ID
 */
#define AM_EVENT_HAS_USER_ID(event) \
    (((const struct am_event*)(event))->id >= AM_EVT_USER)

/** Number of bits reserved for event reference counter. */
#define AM_EVENT_REF_COUNTER_BITS 7

/** Number of bits reserved for number of pools. */
#define AM_EVENT_POOL_INDEX_BITS 5

/** Number of bits reserved for least significant bits of event ID. */
#define AM_EVENT_ID_LSW_BITS 4

/** Event reference counter bit mask. */
#define AM_EVENT_REF_COUNTER_MASK \
    ((1U << (unsigned)AM_EVENT_REF_COUNTER_BITS) - 1U)
/** Maximum value of reference counter. */
#define AM_EVENT_REF_COUNTER_MAX AM_EVENT_REF_COUNTER_MASK

/** Event ID least significant word bit mask. */
#define AM_EVENT_ID_LSW_MASK ((1U << (unsigned)AM_EVENT_ID_LSW_BITS) - 1U)

/** Unknown publisher ID */
#define AM_EVENT_PUBLISHER_ID_NONE (-1)

/** Event descriptor. */
struct am_event {
    /** event ID */
    uint32_t id : 16;

    /** reference counter */
    uint32_t ref_counter : AM_EVENT_REF_COUNTER_BITS; /* 7 bits */
    /** if set to zero, then event is not event pool allocated */
    uint32_t pool_index_plus_one : AM_EVENT_POOL_INDEX_BITS; /* 5 bits */
    /**
     * Least significant bits of id for event pool allocated events.
     * Used for sanity checks.
     */
    uint32_t id_lsw : AM_EVENT_ID_LSW_BITS; /* 4 bits */
};

/** To use with AM_ALIGNOF() macro. */
typedef struct am_event am_event_t;

AM_ASSERT_STATIC(sizeof(struct am_event) == sizeof(uint32_t));

/** To use with AM_ALIGNOF() macro. */
typedef struct am_event* am_event_ptr_t;

/** To use AM_ALIGNOF(am_event_t) in application code. */
AM_ALIGNOF_DEFINE(am_event_t);

/** To use AM_ALIGNOF(am_event_ptr_t) in application code. */
AM_ALIGNOF_DEFINE(am_event_ptr_t);

/** The subscribe list for one event. */
struct am_event_subscribe_list {
    uint8_t list[AM_DIV_CEIL(AM_EVT_HANDLERS_NUM_MAX, 8)]; /**< the bitmask */
};

/**
 * Log event content callback type.
 *
 * Used as a parameter to am_event_alloc_log_unsafe() API.
 *
 * @param pool_index   pool index
 * @param event_index  event_index within the pool
 * @param event        event to log
 * @param size         the event size [bytes]
 */
typedef void (*am_event_log_fn)(
    int pool_index, int event_index, const struct am_event* event, int size
);

/** Event log context. */
struct am_event_log_ctx {
    am_event_log_fn cb; /**< event log callback */
    int pool_ind;       /**< event pool index */
};

#ifndef AM_EVENT_POOLS_NUM_MAX
/**
 * The max number of event pools.
 *
 * Can be redefined by user.
 */
#define AM_EVENT_POOLS_NUM_MAX 3
#endif

/** Event allocator */
struct am_event_alloc {
    /** user defined event memory pools  */
    struct am_onesize pools[AM_EVENT_POOLS_NUM_MAX];
    /** the number of user defined event memory pools */
    int npools;
};

/** enter critical section */
extern void (*am_event_crit_enter)(void);
/** exit critical section */
extern void (*am_event_crit_exit)(void);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate event (eXtended version).
 *
 * The event is allocated from one of the memory pools provided
 * with am_event_alloc_add_pool() function.
 *
 * Checks if there are more free memory blocks available than \p margin.
 * If not, then returns NULL. Otherwise allocates the event and returns it.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event returned
 * by this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param alloc   the event allocator
 * @param id      the event identifier
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to remain available after the allocation
 *
 * @return the newly allocated event
 */
struct am_event* am_event_allocate_x(
    struct am_event_alloc* alloc, int id, int size, int margin
);

/**
 * Allocate event.
 *
 * The event is allocated from one of the memory pools provided
 * with am_event_alloc_add_pool() function.
 *
 * The function asserts, if there is no memory left to accommodate the event.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event returned
 * by this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param alloc  the event allocator
 * @param id     the event identifier
 * @param size   the event size [bytes]
 *
 * @return the newly allocated event
 */
struct am_event* am_event_allocate(
    struct am_event_alloc* alloc, int id, int size
);

/**
 * Try to free the event.
 *
 * The event is either static or allocated earlier with
 * am_event_allocate(), am_event_dup() or am_event_dup_x() API calls.
 *
 * Decrements event reference counter by one and frees the event,
 * if the reference counter drops to zero.
 *
 * The function does nothing for statically allocated events -
 * the events for which am_event_is_static() returns true.
 *
 * Thread safe.
 *
 * @param alloc  the event allocator
 * @param event  the event to free
 */
void am_event_free(struct am_event_alloc* alloc, const struct am_event* event);

/**
 * Duplicate an event (eXtended version).
 *
 * Allocates it from memory pools provided by am_event_alloc_add_pool()
 * function.
 *
 * Checks if there are more free memory blocks available than \p margin.
 * If not, then returns NULL. Otherwise allocates memory block
 * and then copies the content of the given event to it.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event returned
 * by this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param alloc  the event allocator
 * @param event   the event to duplicate
 * @param size    the event size [bytes]
 * @param margin  free memory blocks to be available after the allocation
 *
 * @return the newly allocated copy of \p event
 */
struct am_event* am_event_dup_x(
    struct am_event_alloc* alloc,
    const struct am_event* event,
    int size,
    int margin
);

/**
 * Duplicate an event.
 *
 * Allocates it from memory pools provided by am_event_alloc_add_pool()
 * function.
 *
 * The function asserts, if there is no memory left to allocated the event.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event returned
 * by this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param alloc  the event allocator
 * @param event  the event to duplicate
 * @param size   the event size [bytes]
 *
 * @return the newly allocated copy of \p event
 */
struct am_event* am_event_dup(
    struct am_event_alloc* alloc, const struct am_event* event, int size
);

/**
 * Check if event is static.
 *
 * Thread safe.
 *
 * @param event  the event to check
 *
 * @retval true   the event is static
 * @retval false  the event is not static
 */
bool am_event_is_static(const struct am_event* event);

/**
 * Increment event reference counter by one.
 *
 * Incrementing the event reference prevents the automatic event disposal.
 * Used to hold on to the event.
 * Call am_event_dec_ref_cnt(), when the event is not needed anymore
 * and can be disposed.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param event  the event
 */
void am_event_inc_ref_cnt(const struct am_event* event);

/**
 * Decrement event reference counter by one.
 *
 * Frees the event, if the reference counter drops to zero.
 *
 * The function does nothing for statically allocated events -
 * the events for which am_event_is_static() returns true.
 *
 * Thread safe.
 *
 * There are limitations to what application code can do with the event after
 * calling this function. Please consult the
 * <a href="https://amast.readthedocs.io/event.html">Event Ownership Diagram</a>
 * to understand the limitations.
 *
 * @param alloc  the event allocator
 * @param event  the event
 */
void am_event_dec_ref_cnt(
    struct am_event_alloc* alloc, const struct am_event* event
);

/**
 * Return event reference counter.
 *
 * Thread safe.
 *
 * Use sparingly as the return value could be volatile due to multitasking.
 *
 * @param event  the event, which reference counter is to be returned
 *
 * @return the event reference counter
 */
int am_event_get_ref_cnt(const struct am_event* event);

/**
 * Register critical section APIs
 *
 * @param crit_enter  Enter critical section.
 * @param crit_exit   Exit critical section.
 */
void am_event_register_crit(void (*crit_enter)(void), void (*crit_exit)(void));

/**
 * Free event without using critical section APIs.
 *
 * @param alloc  the event allocator
 * @param event  The event to free
 */
void am_event_free_unsafe(
    struct am_event_alloc* alloc, const struct am_event* event
);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_COMMON_H_INCLUDED */
