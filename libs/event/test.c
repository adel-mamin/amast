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

#include <stdint.h>

#include "common/macros.h"
#include "common/alignment.h"

#include "event.h"

struct buf1 {
    struct am_event e;
    int64_t _;
} buf1;

struct buf2 {
    struct am_event e;
    int64_t _[2];
} buf2;

struct buf3 {
    struct am_event e;
    int64_t _[3];
} buf3;

struct buf4 {
    struct am_event e;
    int64_t _[4];
} buf4;

struct buf5 {
    struct am_event e;
    int64_t _[5];
} buf5;

static void crit_stub(void) {}

static void test_allocate(int size, int pool_index_plus_one) {
    const struct am_event *e = am_event_allocate(AM_EVT_USER, size);
    AM_ASSERT(e->pool_index_plus_one == pool_index_plus_one);
    am_event_free(e);
}

int main(void) {
    const int align = AM_ALIGNOF(am_event_t);
    static const struct am_event_state_cfg cfg = {
        .crit_enter = crit_stub, .crit_exit = crit_stub
    };
    {
        am_event_state_ctor(&cfg);
        am_event_add_pool(&buf1, sizeof(buf1), sizeof(buf1), align);

        test_allocate(sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(sizeof(buf1) - 1, /*pool_index_plus_one=*/1);
    }
    {
        am_event_state_ctor(&cfg);
        am_event_add_pool(&buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_add_pool(&buf2, sizeof(buf2), sizeof(buf2), align);

        test_allocate(sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
    }
    {
        am_event_state_ctor(&cfg);
        am_event_add_pool(&buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_add_pool(&buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_add_pool(&buf3, sizeof(buf3), sizeof(buf3), align);

        test_allocate(sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3), /*pool_index_plus_one=*/3);
    }
    {
        am_event_state_ctor(&cfg);
        am_event_add_pool(&buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_add_pool(&buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_add_pool(&buf3, sizeof(buf3), sizeof(buf3), align);
        am_event_add_pool(&buf4, sizeof(buf4), sizeof(buf4), align);

        test_allocate(sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3), /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3) + 1, /*pool_index_plus_one=*/4);
        test_allocate(sizeof(buf4) - 1, /*pool_index_plus_one=*/4);
        test_allocate(sizeof(buf4), /*pool_index_plus_one=*/4);
    }
    {
        am_event_state_ctor(&cfg);
        am_event_add_pool(&buf1, sizeof(buf1), sizeof(buf1), align);
        am_event_add_pool(&buf2, sizeof(buf2), sizeof(buf2), align);
        am_event_add_pool(&buf3, sizeof(buf3), sizeof(buf3), align);
        am_event_add_pool(&buf4, sizeof(buf4), sizeof(buf4), align);
        am_event_add_pool(&buf5, sizeof(buf5), sizeof(buf5), align);

        test_allocate(sizeof(buf1), /*pool_index_plus_one=*/1);
        test_allocate(sizeof(buf1) + 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2), /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) - 1, /*pool_index_plus_one=*/2);
        test_allocate(sizeof(buf2) + 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3) - 1, /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3), /*pool_index_plus_one=*/3);
        test_allocate(sizeof(buf3) + 1, /*pool_index_plus_one=*/4);
        test_allocate(sizeof(buf4) - 1, /*pool_index_plus_one=*/4);
        test_allocate(sizeof(buf4), /*pool_index_plus_one=*/4);
        test_allocate(sizeof(buf4) + 1, /*pool_index_plus_one=*/5);
        test_allocate(sizeof(buf5) - 1, /*pool_index_plus_one=*/5);
        test_allocate(sizeof(buf5), /*pool_index_plus_one=*/5);
    }

    return 0;
}
