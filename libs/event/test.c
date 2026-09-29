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
 * Event allocation unit tests.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common/macros.h"
#include "common/types.h"
#include "common/alignment.h"

#include "event_common.h"
#include "event_queue.h"
#include "event_pool.h"
#include "event_async.h"
#include "event_sync.h"
#include "common/compiler.h"
#include "onesize/onesize.h"

static struct buf1 {
    struct am_event e;
    int64_t _;
} buf1;

static struct buf2 {
    struct am_event e;
    int64_t _[2];
} buf2;

static struct buf3 {
    struct am_event e;
    int64_t _[3];
} buf3;

static struct buf4 {
    struct am_event e;
    int64_t _[4];
} buf4;

static struct buf5 {
    struct am_event e;
    int64_t _[5];
} buf5;

static void test_allocate(
    struct am_event_alloc* alloc, int size, int pool_index_plus_one
) {
    const struct am_event* e = am_event_allocate(alloc, AM_EVT_USER, size);
    AM_ASSERT(e->pool_index_plus_one == pool_index_plus_one);
    am_event_free(alloc, e);
}

static bool test_event_pop_handle(
    const void* ctx, const struct am_event* event
) {
    AM_ASSERT(ctx);
    AM_ASSERT(event);

    const struct am_event* event_expected = ctx;
    AM_ASSERT(event == event_expected);

    return true;
}

static void test_am_event_queue(const int capacity, const int rdwr_num) {
    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);
    am_event_alloc_add_pool(
        &alloc, &buf1, sizeof(buf1), sizeof(buf1), AM_ALIGNOF(am_event_t)
    );

    const struct am_event* pool[capacity];

    struct am_event_queue q;
    am_event_queue_init(&q, pool, capacity, &alloc);
    AM_ASSERT(am_event_queue_is_empty(&q));

    if (!rdwr_num) {
        return;
    }
    AM_ASSERT(rdwr_num > 0);
    struct am_event events[rdwr_num];

    memset(events, 0, sizeof(events));

    for (int i = 0; i < rdwr_num; ++i) {
        enum am_rc rc = am_event_queue_push_back(&q, &events[i]);
        if (i == 0) {
            AM_ASSERT(rc == AM_RC_QUEUE_WAS_EMPTY);
        } else {
            AM_ASSERT(rc == AM_RC_OK);
        }
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == (i + 1));
        AM_ASSERT(!am_event_queue_is_empty(&q));
    }

    for (int i = 0; i < rdwr_num; ++i) {
        am_event_queue_pop_front_with_cb(
            &q, (am_event_handler_fn)test_event_pop_handle, &events[i]
        );
    }

    for (int i = 0; i < rdwr_num; ++i) {
        enum am_rc rc = am_event_queue_push_front(&q, &events[i]);
        if (i == 0) {
            AM_ASSERT(rc == AM_RC_QUEUE_WAS_EMPTY);
        } else {
            AM_ASSERT(rc == AM_RC_OK);
        }
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) > 0);
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == (i + 1));
        AM_ASSERT(!am_event_queue_is_empty(&q));
    }

    for (int i = (rdwr_num - 1); i >= 0; --i) {
        am_event_queue_pop_front_with_cb(
            &q, (am_event_handler_fn)test_event_pop_handle, &events[i]
        );
    }

    AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == 0);
    AM_ASSERT(am_event_queue_is_empty(&q));
}

enum {
    EVT_TEST = AM_EVT_USER,
    EVT_PUB_MAX,
};

static bool test_event_enqueue(
    void* ctx, const struct am_event* event, struct am_event_queue_policy policy
) {
    struct am_event_queue* queue = ctx;
    enum am_rc rc = am_event_queue_push_unsafe(queue, event, policy);
    return rc != AM_RC_ERR;
}

static bool test_event_proc(void* ctx, const struct am_event* event) {
    (void)ctx;
    AM_ASSERT(event);
    AM_ASSERT(event->id == EVT_TEST);
    return true;
}

static void test_event_async_publish_two_succeed_two_fail(void) {
    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);

    char pool[2 * AM_POOL_BLOCK_SIZEOF(struct am_event)] AM_ALIGNED(
        AM_ALIGN_MAX
    );
    const int align = AM_ALIGNOF(am_event_t);
    const int block_size = AM_POOL_BLOCK_SIZEOF(struct am_event);
    am_event_alloc_add_pool(&alloc, &pool, sizeof(pool), block_size, align);

    AM_ASSERT(2 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    struct am_event_queue queue[2];

    struct am_event_async_hub hub;
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX - AM_EVT_USER];
    am_event_async_init(&hub, pubsub_list, AM_COUNTOF(pubsub_list), &alloc);

    { /* async event consumer 1 */
        static const struct am_event* queue_pool[2];
        am_event_queue_init(
            &queue[0], queue_pool, AM_COUNTOF(queue_pool), &alloc
        );

        const int handler_id = 0;
        am_event_async_register_with_id(
            &hub, test_event_enqueue, &queue[0], handler_id
        );

        am_event_async_subscribe(&hub, handler_id, EVT_TEST);
    }
    { /* async event consumer 2 */
        static const struct am_event* queue_pool[2];
        am_event_queue_init(
            &queue[1], queue_pool, AM_COUNTOF(queue_pool), &alloc
        );

        const int handler_id = 1;
        am_event_async_register_with_id(
            &hub, test_event_enqueue, &queue[1], handler_id
        );

        am_event_async_subscribe(&hub, handler_id, EVT_TEST);
    }

    /* publish events */

    struct am_event_queue_policy policy = {.margin = 1};

    const struct am_event* e = NULL;
    e = am_event_allocate(&alloc, EVT_TEST, sizeof(struct am_event));

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    bool published_all = am_event_async_publish(&hub, e, policy);
    AM_ASSERT(published_all);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    e = am_event_allocate(&alloc, EVT_TEST, sizeof(struct am_event));

    published_all = am_event_async_publish(&hub, e, policy);
    AM_ASSERT(!published_all);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    /* consume events */

    enum am_rc rc = AM_RC_OK;

    rc = am_event_queue_pop_front_with_cb(
        &queue[0], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_OK);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[0], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_ERR);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[1], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_OK);

    AM_ASSERT(2 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[1], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_ERR);

    AM_ASSERT(2 == am_event_alloc_get_nfree(&alloc, /*index=*/0));
}

static void test_event_async_publish_one_fail_one_succeeds(void) {
    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);

    char pool[2 * AM_POOL_BLOCK_SIZEOF(struct am_event)] AM_ALIGNED(
        AM_ALIGN_MAX
    );
    const int align = AM_ALIGNOF(am_event_t);
    const int block_size = AM_POOL_BLOCK_SIZEOF(struct am_event);
    am_event_alloc_add_pool(&alloc, &pool, sizeof(pool), block_size, align);

    AM_ASSERT(2 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    struct am_event_queue queue[2];

    struct am_event_async_hub hub;
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX - AM_EVT_USER];
    am_event_async_init(&hub, pubsub_list, AM_COUNTOF(pubsub_list), &alloc);

    { /* async event consumer 1 */
        static const struct am_event* queue_pool[2];
        am_event_queue_init(
            &queue[0], queue_pool, AM_COUNTOF(queue_pool), &alloc
        );

        const int handler_id = 0;
        am_event_async_register_with_id(
            &hub, test_event_enqueue, &queue[0], handler_id
        );

        am_event_async_subscribe(&hub, handler_id, EVT_TEST);
    }
    { /* async event consumer 2 */
        static const struct am_event* queue_pool[3];
        am_event_queue_init(
            &queue[1], queue_pool, AM_COUNTOF(queue_pool), &alloc
        );

        const int handler_id = 1;
        am_event_async_register_with_id(
            &hub, test_event_enqueue, &queue[1], handler_id
        );

        am_event_async_subscribe(&hub, handler_id, EVT_TEST);
    }

    /* publish event */

    struct am_event_queue_policy policy = {.margin = 1};

    const struct am_event* e = NULL;
    e = am_event_allocate(&alloc, EVT_TEST, sizeof(struct am_event));

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    bool published_all = am_event_async_publish(&hub, e, policy);
    AM_ASSERT(published_all);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    e = am_event_allocate(&alloc, EVT_TEST, sizeof(struct am_event));

    published_all = am_event_async_publish(&hub, e, policy);
    AM_ASSERT(!published_all);

    AM_ASSERT(0 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    /* consume events */

    enum am_rc rc = AM_RC_OK;

    rc = am_event_queue_pop_front_with_cb(
        &queue[0], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_OK);

    AM_ASSERT(0 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[1], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_OK);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[0], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_ERR);

    AM_ASSERT(1 == am_event_alloc_get_nfree(&alloc, /*index=*/0));

    rc = am_event_queue_pop_front_with_cb(
        &queue[1], test_event_proc, /*ctx=*/NULL
    );
    AM_ASSERT(rc == AM_RC_OK);

    AM_ASSERT(2 == am_event_alloc_get_nfree(&alloc, /*index=*/0));
}

struct test_event_sync_unregister_ctx {
    struct am_event_sync_hub* hub;
    int handler_id;
    int called;
};

static bool test_event_sync_handler(
    void* ctx, const struct am_event* event, void* out, int out_size
) {
    (void)event;
    (void)out;
    (void)out_size;

    int* called = ctx;
    ++*called;

    return true;
}

static bool test_event_sync_unregister_handler(
    void* ctx, const struct am_event* event, void* out, int out_size
) {
    (void)event;
    (void)out;
    (void)out_size;

    struct test_event_sync_unregister_ctx* test_ctx = ctx;
    ++test_ctx->called;

    am_event_sync_unregister(test_ctx->hub, test_ctx->handler_id);

    return true;
}

struct test_event_sync_reuse_ctx {
    struct am_event_sync_hub* hub;
    int handler_id;
    int called;
    int replacement_called;
};

static bool test_event_sync_unregister_and_reuse_handler(
    void* ctx, const struct am_event* event, void* out, int out_size
) {
    (void)event;
    (void)out;
    (void)out_size;

    struct test_event_sync_reuse_ctx* test_ctx = ctx;
    ++test_ctx->called;

    am_event_sync_unregister(test_ctx->hub, test_ctx->handler_id);

    int handler_id = am_event_sync_register(
        test_ctx->hub,
        "replacement",
        test_event_sync_handler,
        &test_ctx->replacement_called
    );
    AM_ASSERT(handler_id == test_ctx->handler_id);

    return true;
}

static void test_event_sync_publish_handler_unregisters_later_handler(void) {
    struct am_event_sync_hub hub;
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX - AM_EVT_USER];

    am_event_sync_init(&hub, pubsub_list, AM_COUNTOF(pubsub_list));

    int later_called = 0;

    /*
     * Register the target first so it gets the lower handler ID.
     *
     * Synchronous publish processes the most significant subscribed handler
     * bit first, therefore the handler registered below executes before this
     * one.
     */
    int later_id = am_event_sync_register(
        &hub, "later", test_event_sync_handler, &later_called
    );

    struct test_event_sync_unregister_ctx ctx = {
        .hub = &hub,
        .handler_id = later_id,
    };

    int first_id = am_event_sync_register(
        &hub, "first", test_event_sync_unregister_handler, &ctx
    );

    AM_ASSERT(first_id > later_id);

    am_event_sync_subscribe(&hub, later_id, EVT_TEST);
    am_event_sync_subscribe(&hub, first_id, EVT_TEST);

    const struct am_event event = {
        .id = EVT_TEST,
    };

    bool published_all = am_event_sync_publish(&hub, &event);

    AM_ASSERT(published_all);
    AM_ASSERT(ctx.called == 1);
    AM_ASSERT(later_called == 0);
}

static void test_event_sync_publish_handler_unregisters_and_reuses_later_id(
    void
) {
    struct am_event_sync_hub hub;
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX - AM_EVT_USER];

    am_event_sync_init(&hub, pubsub_list, AM_COUNTOF(pubsub_list));

    int old_handler_called = 0;

    int later_id = am_event_sync_register(
        &hub, "old", test_event_sync_handler, &old_handler_called
    );

    struct test_event_sync_reuse_ctx ctx = {
        .hub = &hub,
        .handler_id = later_id,
    };

    int first_id = am_event_sync_register(
        &hub, "first", test_event_sync_unregister_and_reuse_handler, &ctx
    );

    AM_ASSERT(first_id > later_id);

    am_event_sync_subscribe(&hub, later_id, EVT_TEST);
    am_event_sync_subscribe(&hub, first_id, EVT_TEST);

    const struct am_event event = {
        .id = EVT_TEST,
    };

    bool published_all = am_event_sync_publish(&hub, &event);

    AM_ASSERT(published_all);
    AM_ASSERT(ctx.called == 1);

    /*
     * Neither the old occupant nor the new occupant of later_id may receive
     * the event from the stale publication snapshot.
     */
    AM_ASSERT(old_handler_called == 0);
    AM_ASSERT(ctx.replacement_called == 0);
}

int main(void) {
    const int align = AM_ALIGNOF(am_event_t);
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf1) - 1, /*pool_index_plus_one=*/1);
    }
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_alloc_add_pool(&ea, &buf2, sizeof(buf2), sizeof(buf2), align);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
    }
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_alloc_add_pool(&ea, &buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_alloc_add_pool(&ea, &buf3, sizeof(buf3), sizeof(buf3), align);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3), /*pool_index_plus_one=*/3);
    }
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_alloc_add_pool(&ea, &buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_alloc_add_pool(&ea, &buf3, sizeof(buf3), sizeof(buf3), align);
        am_event_alloc_add_pool(&ea, &buf4, sizeof(buf4), sizeof(buf4), align);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3), /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3) + 1, /*pool_index_plus_one=*/4);
        test_allocate(&ea, sizeof(buf4) - 1, /*pool_index_plus_one=*/4);
        test_allocate(&ea, sizeof(buf4), /*pool_index_plus_one=*/4);
    }
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_alloc_add_pool(&ea, &buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_alloc_add_pool(&ea, &buf3, sizeof(buf3), sizeof(buf3), align);
        am_event_alloc_add_pool(&ea, &buf4, sizeof(buf4), sizeof(buf4), align);
        am_event_alloc_add_pool(&ea, &buf5, sizeof(buf5), sizeof(buf5), align);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(&ea, sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3), /*pool_index_plus_one=*/3);
        test_allocate(&ea, sizeof(buf3) + 1, /*pool_index_plus_one=*/4);
        test_allocate(&ea, sizeof(buf4) - 1, /*pool_index_plus_one=*/4);
        test_allocate(&ea, sizeof(buf4), /*pool_index_plus_one=*/4);
        test_allocate(&ea, sizeof(buf4) + 1, /*pool_index_plus_one=*/5);
        test_allocate(&ea, sizeof(buf5) - 1, /*pool_index_plus_one=*/5);
        test_allocate(&ea, sizeof(buf5), /*pool_index_plus_one=*/5);
    }

    test_am_event_queue(/*capacity=*/1, /*rdwr_num=*/0);
    test_am_event_queue(/*capacity=*/1, /*rdwr_num=*/1);
    test_am_event_queue(/*capacity=*/2, /*rdwr_num=*/1);
    test_am_event_queue(/*capacity=*/3, /*rdwr_num=*/3);

    test_event_async_publish_two_succeed_two_fail();
    test_event_async_publish_one_fail_one_succeeds();

    test_event_sync_publish_handler_unregisters_later_handler();
    test_event_sync_publish_handler_unregisters_and_reuses_later_id();

    return 0;
}
