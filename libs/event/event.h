/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2024 Adel Mamin
 * Copyright (c) 2019 Ryan Hartlage (documentation)
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

#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

/**
 * The event IDs below this value are reserved
 * and should not be used for user events.
 */
#define AM_EVT_USER 5

/**
 * Check if event has a valid user event ID
 * @param event   event to check
 * @retval true   the event has user event ID
 * @retval false  the event does not have user event ID
 */
#define AM_EVENT_HAS_USER_ID(event) \
    (((struct am_event*)(event))->id >= AM_EVT_USER)

#define AM_EVENT_TICK_DOMAIN_BITS 3

#define AM_EVT_CTOR(id_) ((struct am_event){.id = (id_)})

/** Event descriptor */
struct am_event {
    /** event ID */
    int id;

    /**
     * The flags below have special purpose in active objects framework (AOF).
     * Otherwise are unused.
     */

    /** reference counter */
    unsigned ref_counter : 6;
    /** if set to zero, then event is statically allocated */
    unsigned pool_index : 5;
    /** tick domain for time events */
    unsigned tick_domain : AM_EVENT_TICK_DOMAIN_BITS;
    /** PUB/SUB time event */
    unsigned pubsub_time : 1;
    /** n/a */
    unsigned reserved : 1;
};

#endif /* EVENT_H_INCLUDED */
