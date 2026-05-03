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
#include <stddef.h>
#include <stdint.h>

#include "common/macros.h"
#include "common/alignment.h"

#include "event_common.h"
#include "event_queue.h"
#include "event_async.h"
#include "event_pool.h"

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

static void test_am_event_queue(const int capacity, const int rdwr_num) {
    struct am_event_alloc alloc;
    am_event_alloc_init(&alloc);
    am_event_alloc_add_pool(
        &alloc, &buf1, sizeof(buf1), sizeof(buf1), AM_ALIGNOF(am_event_t)
    );

    const struct am_event* pool[capacity];

    struct am_event_queue q;
    am_event_queue_ctor(&q, pool, capacity, &alloc);
    AM_ASSERT(am_event_queue_is_empty(&q));

    if (!rdwr_num) {
        return;
    }
    AM_ASSERT(rdwr_num > 0);
    struct am_event events[rdwr_num];

    for (int i = 1; i < rdwr_num; ++i) {
        bool rc = am_event_queue_push_back(&q, &events[i]);
        AM_ASSERT(rc);
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == i);
        AM_ASSERT(!am_event_queue_is_empty(&q));
    }

    for (int i = 1; i <= rdwr_num; ++i) {
        const struct am_event* event = am_event_queue_pop_front(&q);
        AM_ASSERT(event == &events[i]);
    }

    for (int i = 1; i <= rdwr_num; ++i) {
        bool rc = am_event_queue_push_front(&q, &events[i]);
        AM_ASSERT(rc);
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) > 0);
        AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == i);
        AM_ASSERT(!am_event_queue_is_empty(&q));
    }

    for (int i = rdwr_num; i > 0; --i) {
        const struct am_event* event = am_event_queue_pop_front(&q);
        AM_ASSERT(&events[i] == event);
    }

    AM_ASSERT(am_event_queue_get_nbusy_unsafe(&q) == 0);
    AM_ASSERT(am_event_queue_is_empty(&q));
}

int main(void) {
    const int align = AM_ALIGNOF(am_event_t);
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);

        am_event_async_init(/*sub=*/NULL, /*nsub=*/0, &ea);

        test_allocate(&ea, sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(&ea, sizeof(buf1) - 1, /*pool_index_plus_one=*/1);
    }
    {
        struct am_event_alloc ea;
        am_event_alloc_init(&ea);
        am_event_alloc_add_pool(&ea, &buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_alloc_add_pool(&ea, &buf2, sizeof(buf2), sizeof(buf2), align);

        am_event_async_init(/*sub=*/NULL, /*nsub=*/0, &ea);

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

        am_event_async_init(/*sub=*/NULL, /*nsub=*/0, &ea);

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

        am_event_async_init(/*sub=*/NULL, /*nsub=*/0, &ea);

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

        am_event_async_init(/*sub=*/NULL, /*nsub=*/0, &ea);

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
    /* test_am_event_queue(/\*capacity=*\/1, /\*rdwr_num=*\/1); */
    /* test_am_event_queue(/\*capacity=*\/2, /\*rdwr_num=*\/1); */
    /* test_am_event_queue(/\*capacity=*\/3, /\*rdwr_num=*\/3); */

    return 0;
}
