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

#ifndef AM_EVENT_SYNC_H_INCLUDED
#define AM_EVENT_SYNC_H_INCLUDED

#include <stdbool.h>

#include "common/types.h"
#include "event_common.h"

/**
 * Event handler function type.
 *
 * @param ctx        event handler specific context
 * @param event      input event
 * @param out        output event.
 *                   The memory is provided by the caller and
 *                   should be sufficient to accommodate the output
 *                   event data. Optional. Can be NULL.
 * @param out_size   the size of memory behind \p out pointer [bytes]
 *
 * @return Return code.
 */
typedef enum am_rc (*am_event_sync_fn)(
    void* ctx, const struct am_event* event, struct am_event* out, int out_size
);

/** Synchronous event hub. */
struct am_event_sync_hub {
    /** User defined pub/sub list. */
    struct am_event_subscribe_list* sub;
    /** User defined pub/sub list length. */
    int nsub;

    /** The event allocator. */
    struct am_event_alloc* alloc;

    /** Synchronous event handlers */
    struct am_event_sync_handler {
        /** Event handler function */
        am_event_sync_fn fn;
        /** Event handler context */
        void* ctx;
    } handlers[AM_EVT_HANDLERS_NUM_MAX]; /**< event handlers */

    /**
     * safety net to catch missing subscribe list in am_event_sync_init()
     */
    bool subscribe_list_set;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize synchronous event hub.
 *
 * Must be called before calling any other synchronous event hub APIs.
 *
 * Thread unsafe.
 *
 * @param hub    synchronous event hub to initialize
 * @param sub    the array of subscription lists
 *               Optional. Only needed, if event pub/sub functionality is used.
 *               The pub/sub functionality is provided by
 *               am_event_sync_publish(),
 *               am_event_sync_subscribe(), am_event_sync_unsubscribe() and
 *               am_event_sync_unsubscribe_all() APIs.
 * @param nsub   the number of elements in sub array
 * @param alloc  the event allocator
 */
void am_event_sync_init(
    struct am_event_sync_hub* hub,
    struct am_event_subscribe_list* sub,
    int nsub,
    struct am_event_alloc* alloc
);

/**
 * Check whether pub/sub support is enabled for the synchronous event hub.
 *
 * Pub/sub support is enabled when am_event_sync_init() is called with a valid
 * array of subscription lists.
 *
 * @param hub  synchronous event hub
 * @return true if pub/sub support is enabled, otherwise false
 */
bool am_event_sync_is_pubsub_enabled(const struct am_event_sync_hub* hub);

/**
 * Subscribe event handler to \p event ID.
 *
 * The \p event ID must be smaller than the number of elements
 * in the array of subscription lists provided to
 * am_event_sync_init().
 *
 * @param hub         synchronous event hub
 * @param handler_id  event handler to subscribe
 *                    The ID is returned by am_event_sync_register()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 * @param event_id    the event ID to subscribe to.
 *                    Must be more or equal to AM_EVT_USER.
 *                    Used to compute an index into the array of subscription
 *                    lists passed to am_event_sync_init().
 *                    Passing an event ID beyond the size of the array
 *                    results in an assertion failure.
 */
void am_event_sync_subscribe(
    struct am_event_sync_hub* hub, int handler_id, int event_id
);

/**
 * Unsubscribe event handler from \p event ID.
 *
 * The \p event ID must be smaller than the number of elements
 * in the array of subscription lists provided to
 * am_event_sync_init().
 *
 * @param hub         synchronous event hub
 * @param handler_id  event handler to unsubscribe
 *                    The ID is returned by am_event_sync_register()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 * @param event_id    the event ID to unsubscribe from.
 *                    Must be more or equal to AM_EVT_USER.
 *                    Used to compute an index into the array of subscription
 *                    lists passed to am_event_sync_init().
 *                    Passing an event ID beyond the size of the array
 *                    results in an assertion failure.
 */
void am_event_sync_unsubscribe(
    struct am_event_sync_hub* hub, int handler_id, int event_id
);

/**
 * Unsubscribe event handler from all events.
 *
 * @param hub         synchronous event hub
 * @param handler_id  event handler to unsubscribe.
 *                    The ID is returned by am_event_sync_register()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 */
void am_event_sync_unsubscribe_all(
    struct am_event_sync_hub* hub, int handler_id
);

/**
 * Register event handler.
 *
 * @param hub         synchronous event hub
 * @param fn   the event handler function
 * @param ctx  the event handler function context
 * @return the event handler ID. To be used as a parameter to
 *         am_event_sync_subscribe(),
 *         am_event_sync_unsubscribe_all(),
 *         am_event_sync_unsubscribe(),
 *         am_event_sync_unregister()
 */
int am_event_sync_register(
    struct am_event_sync_hub* hub, am_event_sync_fn fn, void* ctx
);

/**
 * Unregister event handler by ID.
 *
 * The event handler must be registered with am_event_sync_register()
 * prior to calling this function.
 *
 * @param hub         synchronous event hub
 * @param handler_id  the ID of event handler to unregister.
 *                    The ID is returned by am_event_sync_register()
 *                    Passing an invalid event handler ID is a programming error
 *                    and results in an assertion failure.
 */
void am_event_sync_unregister(struct am_event_sync_hub* hub, int handler_id);

/**
 * Post event to a specific event handler.
 *
 * This function delivers \p event directly to the event handler identified by
 * \p dest_id.
 *
 * WARNING:
 * Posting an event to the currently executing handler may cause recursion.
 *
 * @param hub       synchronous event hub
 * @param dest_id   destination event handler ID returned by
 *                  am_event_sync_register()
 *                  Passing an invalid event handler ID is a programming error
 *                  and results in an assertion failure.
 * @param event     input event
 * @param out       output event. Optional. Can be NULL.
 *                  The memory is provided by the caller and must be sufficient
 *                  to accommodate the output event data.
 * @param out_size  the size of memory behind \p out pointer [bytes]
 * @return Return code.
 */
enum am_rc am_event_sync_post(
    struct am_event_sync_hub* hub,
    int dest_id,
    const struct am_event* event,
    struct am_event* out,
    int out_size
);

/** Enable event recursion */
#define AM_EVENT_SYNC_RECURSION 0x01U

/**
 * Publish event to subscribed event handlers.
 *
 * This function delivers \p event to all event handlers subscribed to the
 * event ID carried by \p event.
 *
 * Delivery to the publisher itself is controlled by
 * AM_EVENT_SYNC_RECURSION in \p policy.
 * If recursion is enabled and the publisher is subscribed to the event,
 * the event is also delivered to the publisher, resulting in recursion.
 *
 * @param hub           synchronous event hub
 * @param publisher_id  publisher event handler ID returned by
 *                      am_event_sync_register().
 *                      Passing an invalid event handler ID is a programming
 *                      error and results in an assertion failure.
 * @param event         input event
 * @param out           output event. Optional. Can be NULL.
 *                      The memory is provided by the caller and must be
 *                      sufficient to accommodate the output event data.
 * @param out_size      the size of memory behind \p out pointer [bytes]
 * @param policy        event handling policy. A bitwise OR of AM_EVENT_SYNC_*
 *                      flags, such as AM_EVENT_SYNC_RECURSION.
 * @return Return code.
 */
enum am_rc am_event_sync_publish(
    struct am_event_sync_hub* hub,
    int publisher_id,
    const struct am_event* event,
    struct am_event* out,
    int out_size,
    unsigned policy
);

#ifdef __cplusplus
}
#endif

#endif /* AM_EVENT_SYNC_H_INCLUDED */
