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

#ifndef AM_AO_STATE_H_INCLUDED
#define AM_AO_STATE_H_INCLUDED

#include <stdbool.h>

#include "ao/ao.h"
#include "bit/bit.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if event ID belongs to pubsub range.
 * @param event   event to check
 * @retval true   the event has pubsub event ID
 * @retval false  the event does not have pubsub event ID
 */
#define AM_EVENT_HAS_PUBSUB_ID(event) \
    (((const struct am_event *)(event))->id < am_ao_state_.nsub)

/** Active object library internal state. */
struct am_ao_state {
    /** User defined pubsub list. */
    struct am_ao_subscribe_list *sub;
    /** User defined pubsub list length. */
    int nsub;
    /** User defined active objects, or NULL, if not defined. */
    struct am_ao *aos[AM_AO_NUM_MAX];
    /** Number of runnings AOs */
    int aos_cnt;
    /** Ensure simultaneous start of all active objects. */
    int startup_complete_mutex;
    /** User callback on idle state, when no AO is running. */
    void (*on_idle)(void);
    /** Debug callback. */
    void (*debug)(const struct am_ao *ao, const struct am_event *e);

    /** Enter critical section. */
    void (*crit_enter)(void);
    /** Exit critical section. */
    void (*crit_exit)(void);

    /**
     * The priority of the currently running AO.
     * Only valid for cooperative AO builds.
     */
    struct am_ao_prio running_ao_prio;

    /** am_ao_start() calls were completed for all active objects */
    bool startup_complete;
    /** safety net to catch missing am_ao_init_subscribe_list() call */
    bool subscribe_list_set;
};

extern struct am_ao_state am_ao_state_;

/**
 * Notify active object about availability of new event(s).
 *
 * Thread safe.
 *
 * @param ao  the active object to notify
 */
void am_ao_notify(const struct am_ao *ao);

/**
 * Notify active object about availability of new event(s).
 *
 * Thread unsafe.
 *
 * @param ao  the active object to notify
 */
void am_ao_notify_unsafe(const struct am_ao *ao);

/**
 * Internal active object library state constructor.
 * Has different implementation for cooperative and preemptive
 * active object library builds.
 */
void am_ao_state_ctor_(void);

#ifdef __cplusplus
}
#endif

#endif /* AM_AO_STATE_H_INCLUDED */
