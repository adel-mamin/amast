/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "blk/blk.h"
#include "queue/queue.h"

static void test_a1queue(const int capacity, const int rdwr_num) {
    int pool[capacity + 1];
    struct blk blk = {.ptr = pool, .size = (int)sizeof(pool)};

    struct a1queue q;
    a1queue_init(
        &q,
        /*isize=*/sizeof(pool[0]),
        /*alignment=*/sizeof(int),
        &blk
    );
    ASSERT(a1queue_is_empty(&q));

    if (!rdwr_num) {
        return;
    }

    for (int i = 0; i < rdwr_num; i++) {
        bool rc = a1queue_push_back(&q, &i, (int)sizeof(int));
        ASSERT(true == rc);
        ASSERT(a1queue_length(&q) == (i + 1));
        ASSERT(a1queue_capacity(&q) >= a1queue_length(&q));
        ASSERT(!a1queue_is_empty(&q));
    }

    for (int i = 0; i < rdwr_num; i++) {
        void *ptr = a1queue_pop_front(&q);
        ASSERT(ptr);
        A1DISABLE_WARNING(A1W_NULL_DEREFERENCE)
        ASSERT(i == *(int *)ptr);
        A1ENABLE_WARNING(A1W_NULL_DEREFERENCE)
    }

    for (int i = 0; i < rdwr_num; i++) {
        bool rc = a1queue_push_front(&q, &i, (int)sizeof(i));
        ASSERT(true == rc);
        ASSERT(a1queue_length(&q) == (i + 1));
        ASSERT(a1queue_capacity(&q) >= a1queue_length(&q));
        ASSERT(!a1queue_is_empty(&q));
    }

    for (int i = rdwr_num - 1; i >= 0; i--) {
        void *ptr = a1queue_pop_front(&q);
        ASSERT(ptr);
        A1DISABLE_WARNING(A1W_NULL_DEREFERENCE)
        ASSERT(i == *(int *)ptr);
        A1ENABLE_WARNING(A1W_NULL_DEREFERENCE)
    }

    ASSERT(a1queue_length(&q) == 0);
    ASSERT(a1queue_is_empty(&q));
}

int main(void) {
    test_a1queue(/*capacity=*/1, /*rdwr_num=*/0);
    test_a1queue(/*capacity=*/2, /*rdwr_num=*/1);
    test_a1queue(/*capacity=*/3, /*rdwr_num=*/3);
    return 0;
}
