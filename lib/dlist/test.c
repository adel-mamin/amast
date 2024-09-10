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
 * Doubly linked list unit tests.
 */

#include <stddef.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "dlist/dlist.h"

static struct am_dlist dlist;

struct ut_data {
    struct am_dlist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_setup(struct am_dlist *list) {
    am_dlist_init(list);
    for (int i = 0; i < AM_COUNTOF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_am_dlist_empty(void) {
    test_setup(&dlist);

    AM_ASSERT(am_dlist_is_empty(&dlist));

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    AM_ASSERT(!am_dlist_is_empty(&dlist));

    am_dlist_pop(&test_data[0].hdr);
    AM_ASSERT(am_dlist_is_empty(&dlist));

    AM_ASSERT(NULL == am_dlist_peek_back(&dlist));
    AM_ASSERT(NULL == am_dlist_peek_front(&dlist));
}

static void test_am_dlist_push_after(void) {
    test_setup(&dlist);

    am_dlist_push_front(&dlist, &test_data[0].hdr);

    am_dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);

    AM_ASSERT(am_dlist_item_is_linked(&test_data[0].hdr));
    AM_ASSERT(am_dlist_item_is_linked(&test_data[1].hdr));

    AM_ASSERT(&test_data[1].hdr == am_dlist_peek_back(&dlist));
    AM_ASSERT(&test_data[0].hdr == am_dlist_peek_front(&dlist));

    AM_ASSERT(am_dlist_item_is_linked(&test_data[0].hdr));
    AM_ASSERT(am_dlist_item_is_linked(&test_data[1].hdr));

    struct am_dlist_item *e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);

    AM_ASSERT(am_dlist_item_is_linked(&test_data[0].hdr));
    AM_ASSERT(!am_dlist_item_is_linked(&test_data[1].hdr));

    AM_ASSERT(&test_data[0].hdr == am_dlist_peek_back(&dlist));
    AM_ASSERT(&test_data[0].hdr == am_dlist_peek_front(&dlist));
}

static void test_am_dlist_push_after2(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[1].hdr);

    am_dlist_push_after(&test_data[1].hdr, &test_data[2].hdr);
    struct am_dlist_item *e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_am_dlist_push_after3(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    am_dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);
    struct am_dlist_item *e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
    e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);
    e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_am_dlist_push_before(void) {
    test_setup(&dlist);

    am_dlist_push_front(&dlist, &test_data[1].hdr);

    am_dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);
    struct am_dlist_item *e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 0);

    e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_am_dlist_push_before2(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    am_dlist_push_before(&test_data[2].hdr, &test_data[1].hdr);
    struct am_dlist_item *e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 0);

    e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);

    e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_am_dlist_push_before3(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[1].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    am_dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);

    struct am_dlist_item *e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 0);

    e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);

    e = am_dlist_pop_back(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_am_dlist_iterator_forward(void) {
    test_setup(&dlist);

    struct am_dlist_iterator it;
    am_dlist_iterator_init(&dlist, &it, AM_DLIST_FORWARD);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[1].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    struct am_dlist_item *e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 0);
    e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 1);
    e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 2);

    e = am_dlist_iterator_next(&it);
    AM_ASSERT(NULL == e);
}

static void test_am_dlist_iterator_backward(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[1].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    struct am_dlist_iterator it;
    am_dlist_iterator_init(&dlist, &it, AM_DLIST_BACKWARD);

    struct am_dlist_item *e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
    e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 1);
    e = am_dlist_iterator_next(&it);
    AM_ASSERT(((struct ut_data *)e)->data == 0);

    e = am_dlist_iterator_next(&it);
    AM_ASSERT(NULL == e);
}

static void test_am_dlist_pop(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[1].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    struct am_dlist_item *e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 0);

    e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 1);

    e = am_dlist_pop_front(&dlist);
    AM_ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct am_dlist_item *item) {
    int v = *(int *)context;
    const struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_am_dlist_find(void) {
    test_setup(&dlist);

    am_dlist_push_back(&dlist, &test_data[0].hdr);
    am_dlist_push_back(&dlist, &test_data[1].hdr);
    am_dlist_push_back(&dlist, &test_data[2].hdr);

    int v = 0;
    struct am_dlist_item *e = NULL;
    for (v = 0; v < 3; v++) {
        e = am_dlist_find(&dlist, predicate, &v);
        AM_ASSERT(e != NULL);
        const struct ut_data *d = (struct ut_data *)e;
        AM_ASSERT(v == d->data);
    }

    v = 3;
    e = am_dlist_find(&dlist, predicate, &v);
    AM_ASSERT(NULL == e);
}

static void test_am_dlist_size(void) {
    test_setup(&dlist);

    for (int i = 0; i < 10; i++) {
        am_dlist_push_back(&dlist, &test_data[i].hdr);
        AM_ASSERT((i + 1) == am_dlist_size(&dlist));
    }
}

static void test_am_dlist_owns(void) {
    test_setup(&dlist);

    for (int i = 0; i < 10; i++) {
        am_dlist_push_back(&dlist, &test_data[i].hdr);
        AM_ASSERT(am_dlist_owns(&dlist, &test_data[i].hdr));
    }
}

static void test_am_dlist_back(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < AM_COUNTOF(test_data); i++) {
        am_dlist_push_front(&dlist, &test_data[i].hdr);
    }

    for (i = 0; i < AM_COUNTOF(test_data); i++) {
        const struct ut_data *e = (struct ut_data *)am_dlist_pop_back(&dlist);
        AM_ASSERT(e);
        AM_ASSERT(test_data[i].data == e->data);
    }

    AM_ASSERT(0 == am_dlist_size(&dlist));
}

static void test_am_dlist_front(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < AM_COUNTOF(test_data); i++) {
        am_dlist_push_front(&dlist, &test_data[i].hdr);
    }

    for (i = AM_COUNTOF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)am_dlist_pop_front(&dlist);
        AM_ASSERT(e);
        AM_ASSERT(test_data[i - 1].data == e->data);
    }

    AM_ASSERT(0 == am_dlist_size(&dlist));
}

static void test_am_dlist_back2(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < AM_COUNTOF(test_data); i++) {
        am_dlist_push_back(&dlist, &test_data[i].hdr);
    }

    for (i = AM_COUNTOF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)am_dlist_pop_back(&dlist);
        AM_ASSERT(e);
        AM_ASSERT(test_data[i - 1].data == e->data);
    }

    AM_ASSERT(0 == am_dlist_size(&dlist));
}

static void test_am_dlist_next_prev_item(void) {
    test_setup(&dlist);
    struct am_dlist_item *item = &test_data[0].hdr;
    am_dlist_push_back(&dlist, item);
    AM_ASSERT(NULL == am_dlist_next(&dlist, item));
    AM_ASSERT(NULL == am_dlist_prev(&dlist, item));

    struct am_dlist_item *item2 = &test_data[1].hdr;
    am_dlist_push_front(&dlist, item2);
    AM_ASSERT(NULL != am_dlist_next(&dlist, item2));
    AM_ASSERT(NULL == am_dlist_prev(&dlist, item2));
}

int main(void) {
    test_am_dlist_empty();

    test_am_dlist_push_after();
    test_am_dlist_push_after2();
    test_am_dlist_push_after3();

    test_am_dlist_push_before();
    test_am_dlist_push_before2();
    test_am_dlist_push_before3();

    test_am_dlist_iterator_forward();
    test_am_dlist_iterator_backward();

    test_am_dlist_pop();

    test_am_dlist_find();

    test_am_dlist_size();

    test_am_dlist_owns();

    test_am_dlist_front();

    test_am_dlist_back();

    test_am_dlist_back2();

    test_am_dlist_next_prev_item();

    return 0;
}
