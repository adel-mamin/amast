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
 * Queue API unit tests.
 */

#include <stdbool.h>

#include "common/macros.h"
#include "common/types.h"
#include "queue/queue.h"

static void test_am_queue(const int capacity, const int rdwr_num) {
    int pool[capacity];
    struct am_blk blk = {.ptr = pool, .size = (int)sizeof(pool)};

    struct am_queue q;
    am_queue_init(
        &q,
        /*isize=*/sizeof(pool[0]),
        /*alignment=*/sizeof(int),
        &blk
    );
    AM_ASSERT(am_queue_is_empty(&q));

    if (!rdwr_num) {
        return;
    }
    AM_ASSERT(rdwr_num > 0);

    for (int i = 1; i <= rdwr_num; i++) {
        bool rc = am_queue_push_back(&q, &i, (int)sizeof(int));
        AM_ASSERT(true == rc);
        AM_ASSERT(am_queue_length(&q) == i);
        AM_ASSERT(!am_queue_is_empty(&q));
    }

    for (int i = 1; i <= rdwr_num; i++) {
        void *ptr = am_queue_pop_front(&q);
        AM_ASSERT(ptr);
        AM_ASSERT(i == *(int *)ptr);
    }

    for (int i = 1; i <= rdwr_num; i++) {
        bool rc = am_queue_push_front(&q, &i, (int)sizeof(i));
        AM_ASSERT(true == rc);
        AM_ASSERT(am_queue_length(&q) > 0);
        AM_ASSERT(am_queue_length(&q) == i);
        AM_ASSERT(!am_queue_is_empty(&q));
    }

    for (int i = rdwr_num; i > 0; i--) {
        void *ptr = am_queue_pop_front(&q);
        AM_ASSERT(ptr);
        AM_ASSERT(i == *(int *)ptr);
    }

    AM_ASSERT(am_queue_length(&q) == 0);
    AM_ASSERT(am_queue_is_empty(&q));
}

int main(void) {
    test_am_queue(/*capacity=*/1, /*rdwr_num=*/0);
    test_am_queue(/*capacity=*/2, /*rdwr_num=*/1);
    test_am_queue(/*capacity=*/3, /*rdwr_num=*/3);
    return 0;
}
