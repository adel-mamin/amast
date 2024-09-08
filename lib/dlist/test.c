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

static struct a1dlist a1dlist;

struct ut_data {
    struct a1dlist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_setup(struct a1dlist *list) {
    a1dlist_init(list);
    for (int i = 0; i < COUNTOF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_a1dlist_empty(void) {
    test_setup(&a1dlist);

    ASSERT(a1dlist_is_empty(&a1dlist));

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    ASSERT(!a1dlist_is_empty(&a1dlist));

    a1dlist_pop(&test_data[0].hdr);
    ASSERT(a1dlist_is_empty(&a1dlist));

    ASSERT(NULL == a1dlist_peek_back(&a1dlist));
    ASSERT(NULL == a1dlist_peek_front(&a1dlist));
}

static void test_a1dlist_push_after(void) {
    test_setup(&a1dlist);

    a1dlist_push_front(&a1dlist, &test_data[0].hdr);

    a1dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);

    ASSERT(a1dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(a1dlist_item_is_linked(&test_data[1].hdr));

    ASSERT(&test_data[1].hdr == a1dlist_peek_back(&a1dlist));
    ASSERT(&test_data[0].hdr == a1dlist_peek_front(&a1dlist));

    ASSERT(a1dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(a1dlist_item_is_linked(&test_data[1].hdr));

    struct a1dlist_item *e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    ASSERT(a1dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(!a1dlist_item_is_linked(&test_data[1].hdr));

    ASSERT(&test_data[0].hdr == a1dlist_peek_back(&a1dlist));
    ASSERT(&test_data[0].hdr == a1dlist_peek_front(&a1dlist));
}

static void test_a1dlist_push_after2(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[1].hdr);

    a1dlist_push_after(&test_data[1].hdr, &test_data[2].hdr);
    struct a1dlist_item *e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_a1dlist_push_after3(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    a1dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);
    struct a1dlist_item *e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
    e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_a1dlist_push_before(void) {
    test_setup(&a1dlist);

    a1dlist_push_front(&a1dlist, &test_data[1].hdr);

    a1dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);
    struct a1dlist_item *e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_a1dlist_push_before2(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    a1dlist_push_before(&test_data[2].hdr, &test_data[1].hdr);
    struct a1dlist_item *e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_a1dlist_push_before3(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[1].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    a1dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);

    struct a1dlist_item *e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = a1dlist_pop_back(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_a1dlist_iterator_forward(void) {
    test_setup(&a1dlist);

    struct a1dlist_iterator it;
    a1dlist_iterator_init(&a1dlist, &it, A1DLIST_FORWARD);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[1].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    struct a1dlist_item *e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 0);
    e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 2);

    e = a1dlist_iterator_next(&it);
    ASSERT(NULL == e);
}

static void test_a1dlist_iterator_backward(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[1].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    struct a1dlist_iterator it;
    a1dlist_iterator_init(&a1dlist, &it, A1DLIST_BACKWARD);

    struct a1dlist_item *e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 2);
    e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = a1dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1dlist_iterator_next(&it);
    ASSERT(NULL == e);
}

static void test_a1dlist_pop(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[1].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    struct a1dlist_item *e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = a1dlist_pop_front(&a1dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct a1dlist_item *item) {
    int v = *(int *)context;
    const struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_a1dlist_find(void) {
    test_setup(&a1dlist);

    a1dlist_push_back(&a1dlist, &test_data[0].hdr);
    a1dlist_push_back(&a1dlist, &test_data[1].hdr);
    a1dlist_push_back(&a1dlist, &test_data[2].hdr);

    int v = 0;
    struct a1dlist_item *e = NULL;
    for (v = 0; v < 3; v++) {
        e = a1dlist_find(&a1dlist, predicate, &v);
        ASSERT(e != NULL);
        const struct ut_data *d = (struct ut_data *)e;
        ASSERT(v == d->data);
    }

    v = 3;
    e = a1dlist_find(&a1dlist, predicate, &v);
    ASSERT(NULL == e);
}

static void test_a1dlist_size(void) {
    test_setup(&a1dlist);

    for (int i = 0; i < 10; i++) {
        a1dlist_push_back(&a1dlist, &test_data[i].hdr);
        ASSERT((i + 1) == a1dlist_size(&a1dlist));
    }
}

static void test_a1dlist_owns(void) {
    test_setup(&a1dlist);

    for (int i = 0; i < 10; i++) {
        a1dlist_push_back(&a1dlist, &test_data[i].hdr);
        ASSERT(a1dlist_owns(&a1dlist, &test_data[i].hdr));
    }
}

static void test_a1dlist_back(void) {
    test_setup(&a1dlist);

    int i = 0;
    for (i = 0; i < COUNTOF(test_data); i++) {
        a1dlist_push_front(&a1dlist, &test_data[i].hdr);
    }

    for (i = 0; i < COUNTOF(test_data); i++) {
        const struct ut_data *e = (struct ut_data *)a1dlist_pop_back(&a1dlist);
        ASSERT(e);
        ASSERT(test_data[i].data == e->data);
    }

    ASSERT(0 == a1dlist_size(&a1dlist));
}

static void test_a1dlist_front(void) {
    test_setup(&a1dlist);

    int i = 0;
    for (i = 0; i < COUNTOF(test_data); i++) {
        a1dlist_push_front(&a1dlist, &test_data[i].hdr);
    }

    for (i = COUNTOF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)a1dlist_pop_front(&a1dlist);
        ASSERT(e);
        ASSERT(test_data[i - 1].data == e->data);
    }

    ASSERT(0 == a1dlist_size(&a1dlist));
}

static void test_a1dlist_back2(void) {
    test_setup(&a1dlist);

    int i = 0;
    for (i = 0; i < COUNTOF(test_data); i++) {
        a1dlist_push_back(&a1dlist, &test_data[i].hdr);
    }

    for (i = COUNTOF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)a1dlist_pop_back(&a1dlist);
        ASSERT(e);
        ASSERT(test_data[i - 1].data == e->data);
    }

    ASSERT(0 == a1dlist_size(&a1dlist));
}

static void test_a1dlist_next_prev_item(void) {
    test_setup(&a1dlist);
    struct a1dlist_item *item = &test_data[0].hdr;
    a1dlist_push_back(&a1dlist, item);
    ASSERT(NULL == a1dlist_next(&a1dlist, item));
    ASSERT(NULL == a1dlist_prev(&a1dlist, item));

    struct a1dlist_item *item2 = &test_data[1].hdr;
    a1dlist_push_front(&a1dlist, item2);
    ASSERT(NULL != a1dlist_next(&a1dlist, item2));
    ASSERT(NULL == a1dlist_prev(&a1dlist, item2));
}

int main(void) {
    test_a1dlist_empty();

    test_a1dlist_push_after();
    test_a1dlist_push_after2();
    test_a1dlist_push_after3();

    test_a1dlist_push_before();
    test_a1dlist_push_before2();
    test_a1dlist_push_before3();

    test_a1dlist_iterator_forward();
    test_a1dlist_iterator_backward();

    test_a1dlist_pop();

    test_a1dlist_find();

    test_a1dlist_size();

    test_a1dlist_owns();

    test_a1dlist_front();

    test_a1dlist_back();

    test_a1dlist_back2();

    test_a1dlist_next_prev_item();

    return 0;
}
