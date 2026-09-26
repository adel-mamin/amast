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
 * Asynchronous event API declaration.
 */

#ifndef AM_EVENT_ASYNC_H_INCLUDED
#define AM_EVENT_ASYNC_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "event_common.h"
#include "event_queue.h"

/**
 * Asynchronous event handler function type.
 *
 * Called from am_event_async_post() and am_event_async_publish()
 * inside an event critical section. The handler must not call APIs
 * that may enter the same critical section, block, or perform long work.
 *
 * @param ctx     event handler specific context
 * @param event   event to handle
 * @param policy  the event queue handling policy
 *
 * @return true on success, false otherwise
 */
typedef bool (*am_event_async_fn)(
    void* ctx, const struct am_event* event, struct am_event_queue_policy policy
);

/** Asynchronous event hub. */
struct am_event_async_hub {
    /** User defined pubsub list. */
    struct am_event_subscribe_list* sub;
    /** User defined pubsub list length. */
    int nsub;

    /** Asynchronous event handlers */
    struct am_event_async_handler {
        /** Event handler function */
        am_event_async_fn fn;
        /** Event handler context */
        void* ctx;
    } handlers[AM_EVT_HANDLERS_NUM_MAX]; /**< event handlers */

    struct am_event_alloc* alloc; /**< event allocator */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize asynchronous event module.
 *
 * Must be called before calling any other asynchronous event API.
 *
 * @param hub   asynchronous event hub to initialize
 * @param sub   event subscribe list
 * @param nsub  number of entries in the event subscribe list
 * @param alloc event allocator
 *
 * Thread unsafe.
 */
void am_event_async_init(
    struct am_event_async_hub* hub,
    struct am_event_subscribe_list* sub,
    int nsub,
    struct am_event_alloc* alloc
);

/**
 * Check whether pub/sub support is enabled for asynchronous events.
 *
 * Pub/sub support is enabled when am_event_async_init() is called with a
 * valid array of subscription lists.
 *
 * @param hub  asynchronous event hub
 *
 * @return true if pub/sub support is enabled, otherwise false
 */
bool am_event_async_is_pubsub_enabled(struct am_event_async_hub* hub);

/**
 * Subscribe event handler to @p event ID.
 *
 * The @p event ID must be smaller than the number of elements
 * in the array of subscription lists provided to
 * am_event_async_init().
 *
 * @param hub         asynchronous event hub
 * @param handler_id  event handler to subscribe
 *                    The ID provided to am_event_async_register_with_id()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 * @param event_id    the event ID to subscribe to.
 *                    Must be more or equal to AM_EVT_USER.
 *                    Used to compute an index into the array of subscription
 *                    lists passed to am_event_async_init().
 *                    Passing an event ID beyond the size of the array
 *                    results in an assertion failure.
 */
void am_event_async_subscribe(
    struct am_event_async_hub* hub, int handler_id, uint16_t event_id
);

/**
 * Unsubscribe event handler from @p event ID.
 *
 * The @p event ID must be smaller than the number of elements
 * in the array of subscription lists provided to
 * am_event_async_init().
 *
 * @param hub         asynchronous event hub
 * @param handler_id  event handler to unsubscribe
 *                    The ID provided to am_event_async_register_with_id()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 * @param event_id    the event ID to unsubscribe from.
 *                    Must be more or equal to AM_EVT_USER.
 *                    Used to compute an index into the array of subscription
 *                    lists passed to am_event_async_init().
 *                    Passing an event ID beyond the size of the array
 *                    results in an assertion failure.
 */
void am_event_async_unsubscribe(
    struct am_event_async_hub* hub, int handler_id, uint16_t event_id
);

/**
 * Unsubscribe event handler from all events.
 *
 * @param hub         asynchronous event hub
 * @param handler_id  event handler to unsubscribe.
 *                    The ID provided to am_event_async_register_with_id()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 */
void am_event_async_unsubscribe_all(
    struct am_event_async_hub* hub, int handler_id
);

/**
 * Register event handler with ID.
 *
 * @param hub  asynchronous event hub
 * @param fn   the event handler function.
 *             Called by am_event_async_post() and am_event_async_publish()
 *             inside a critical section.
 *             So, only unsafe (not using critical section)
 *             event handlers are allowed.
 * @param ctx  the event handler function context
 * @param handler_id  the event handler ID to register with.
 *                    To be used as a parameter to
 *                    am_event_async_subscribe(),
 *                    am_event_async_unsubscribe_all(),
 *                    am_event_async_unsubscribe(),
 *                    am_event_async_unregister()
 */
void am_event_async_register_with_id(
    struct am_event_async_hub* hub,
    am_event_async_fn fn,
    void* ctx,
    int handler_id
);

/**
 * Unregister event handler by ID.
 *
 * The event handler must be registered with am_event_async_register_with_id()
 * prior to calling this function.
 *
 * @param hub         asynchronous event hub
 * @param handler_id  the ID of event handler to unregister.
 *                    The ID provided to am_event_async_register_with_id()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 */
void am_event_async_unregister(struct am_event_async_hub* hub, int handler_id);

/**
 * Post event to a specific event handler.
 *
 * This function delivers @p event directly to the event handler identified by
 * @p dest_id.
 *
 * @param hub       asynchronous event hub
 * @param dest_id   destination event handler ID provided to
 *                  am_event_async_register_with_id()
 *                  Passing an invalid event handler ID is a programming error
 *                  and results in an assertion failure.
 * @param event     input event
 * @param policy    event queue posting policy
 * @return true on success, false otherwise
 */
bool am_event_async_post(
    struct am_event_async_hub* hub,
    int dest_id,
    const struct am_event* event,
    struct am_event_queue_policy policy
);

/**
 * Publish event to subscribed event handlers.
 *
 * This function delivers @p event to all event handlers subscribed to the
 * event ID carried by @p event.
 *
 * @param hub           asynchronous event hub
 * @param event         input event
 * @param policy        event queue posting policy
 * @return true if published to all subscribers, false otherwise
 */
bool am_event_async_publish(
    struct am_event_async_hub* hub,
    const struct am_event* event,
    struct am_event_queue_policy policy
);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_ASYNC_H_INCLUDED */
