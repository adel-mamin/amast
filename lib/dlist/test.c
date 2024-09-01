/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2022 Adel Mamin
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

static struct dlist dlist;

struct ut_data {
    struct dlist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_setup(struct dlist *list) {
    dlist_init(list);
    for (int i = 0; i < COUNT_OF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_dlist_empty(void) {
    test_setup(&dlist);

    ASSERT(dlist_is_empty(&dlist));

    dlist_push_back(&dlist, &test_data[0].hdr);
    ASSERT(!dlist_is_empty(&dlist));

    dlist_pop(&test_data[0].hdr);
    ASSERT(dlist_is_empty(&dlist));

    ASSERT(NULL == dlist_peek_back(&dlist));
    ASSERT(NULL == dlist_peek_front(&dlist));
}

static void test_dlist_push_after(void) {
    test_setup(&dlist);

    dlist_push_front(&dlist, &test_data[0].hdr);

    dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);

    ASSERT(dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(dlist_item_is_linked(&test_data[1].hdr));

    ASSERT(&test_data[1].hdr == dlist_peek_back(&dlist));
    ASSERT(&test_data[0].hdr == dlist_peek_front(&dlist));

    ASSERT(dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(dlist_item_is_linked(&test_data[1].hdr));

    struct dlist_item *e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    ASSERT(dlist_item_is_linked(&test_data[0].hdr));
    ASSERT(!dlist_item_is_linked(&test_data[1].hdr));

    ASSERT(&test_data[0].hdr == dlist_peek_back(&dlist));
    ASSERT(&test_data[0].hdr == dlist_peek_front(&dlist));
}

static void test_dlist_push_after2(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[1].hdr);

    dlist_push_after(&test_data[1].hdr, &test_data[2].hdr);
    struct dlist_item *e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_dlist_push_after3(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    dlist_push_after(&test_data[0].hdr, &test_data[1].hdr);
    struct dlist_item *e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
    e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_dlist_push_before(void) {
    test_setup(&dlist);

    dlist_push_front(&dlist, &test_data[1].hdr);

    dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);
    struct dlist_item *e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_dlist_push_before2(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    dlist_push_before(&test_data[2].hdr, &test_data[1].hdr);
    struct dlist_item *e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_dlist_push_before3(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[1].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    dlist_push_before(&test_data[1].hdr, &test_data[0].hdr);

    struct dlist_item *e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = dlist_pop_back(&dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_dlist_iterator_forward(void) {
    test_setup(&dlist);

    struct dlist_iterator it;
    dlist_iterator_init(&dlist, &it, dlist_forward);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[1].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    struct dlist_item *e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 0);
    e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 2);

    e = dlist_iterator_next(&it);
    ASSERT(NULL == e);
}

static void test_dlist_iterator_backward(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[1].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    struct dlist_iterator it;
    dlist_iterator_init(&dlist, &it, dlist_backward);

    struct dlist_item *e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 2);
    e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 1);
    e = dlist_iterator_next(&it);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = dlist_iterator_next(&it);
    ASSERT(NULL == e);
}

static void test_dlist_pop(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[1].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    struct dlist_item *e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 1);

    e = dlist_pop_front(&dlist);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct dlist_item *item) {
    int v = *(int *)context;
    const struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_dlist_find(void) {
    test_setup(&dlist);

    dlist_push_back(&dlist, &test_data[0].hdr);
    dlist_push_back(&dlist, &test_data[1].hdr);
    dlist_push_back(&dlist, &test_data[2].hdr);

    int v = 0;
    struct dlist_item *e = NULL;
    for (v = 0; v < 3; v++) {
        e = dlist_find(&dlist, predicate, &v);
        ASSERT(e != NULL);
        const struct ut_data *d = (struct ut_data *)e;
        ASSERT(v == d->data);
    }

    v = 3;
    e = dlist_find(&dlist, predicate, &v);
    ASSERT(NULL == e);
}

static void test_dlist_size(void) {
    test_setup(&dlist);

    for (int i = 0; i < 10; i++) {
        dlist_push_back(&dlist, &test_data[i].hdr);
        ASSERT((i + 1) == dlist_size(&dlist));
    }
}

static void test_dlist_owns(void) {
    test_setup(&dlist);

    for (int i = 0; i < 10; i++) {
        dlist_push_back(&dlist, &test_data[i].hdr);
        ASSERT(dlist_owns(&dlist, &test_data[i].hdr));
    }
}

static void test_dlist_back(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < COUNT_OF(test_data); i++) {
        dlist_push_front(&dlist, &test_data[i].hdr);
    }

    for (i = 0; i < COUNT_OF(test_data); i++) {
        const struct ut_data *e = (struct ut_data *)dlist_pop_back(&dlist);
        ASSERT(e);
        ASSERT(test_data[i].data == e->data);
    }

    ASSERT(0 == dlist_size(&dlist));
}

static void test_dlist_front(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < COUNT_OF(test_data); i++) {
        dlist_push_front(&dlist, &test_data[i].hdr);
    }

    for (i = COUNT_OF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)dlist_pop_front(&dlist);
        ASSERT(e);
        ASSERT(test_data[i - 1].data == e->data);
    }

    ASSERT(0 == dlist_size(&dlist));
}

static void test_dlist_back2(void) {
    test_setup(&dlist);

    int i = 0;
    for (i = 0; i < COUNT_OF(test_data); i++) {
        dlist_push_back(&dlist, &test_data[i].hdr);
    }

    for (i = COUNT_OF(test_data); i > 0; i--) {
        const struct ut_data *e = (struct ut_data *)dlist_pop_back(&dlist);
        ASSERT(e);
        ASSERT(test_data[i - 1].data == e->data);
    }

    ASSERT(0 == dlist_size(&dlist));
}

static void test_dlist_next_prev_item(void) {
    test_setup(&dlist);
    struct dlist_item *item = &test_data[0].hdr;
    dlist_push_back(&dlist, item);
    ASSERT(NULL == dlist_next(&dlist, item));
    ASSERT(NULL == dlist_prev(&dlist, item));

    struct dlist_item *item2 = &test_data[1].hdr;
    dlist_push_front(&dlist, item2);
    ASSERT(NULL != dlist_next(&dlist, item2));
    ASSERT(NULL == dlist_prev(&dlist, item2));
}

int main(void) {
    test_dlist_empty();

    test_dlist_push_after();
    test_dlist_push_after2();
    test_dlist_push_after3();

    test_dlist_push_before();
    test_dlist_push_before2();
    test_dlist_push_before3();

    test_dlist_iterator_forward();
    test_dlist_iterator_backward();

    test_dlist_pop();

    test_dlist_find();

    test_dlist_size();

    test_dlist_owns();

    test_dlist_front();

    test_dlist_back();

    test_dlist_back2();

    test_dlist_next_prev_item();

    return 0;
}
