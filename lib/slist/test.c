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
 * Singly linked list unit tests.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "common/macros.h"
#include "common/compiler.h"
#include "slist/slist.h"

static struct a1slist a1slist;

struct ut_data {
    struct a1slist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_init(struct a1slist *data) {
    a1slist_init(data);
    for (int i = 0; i < COUNTOF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_a1slist_empty(void) {
    test_init(&a1slist);

    ASSERT(a1slist_is_empty(&a1slist) == 1);

    a1slist_push_back(&a1slist, &test_data[0].hdr);
    ASSERT(a1slist_is_empty(&a1slist) == 0);

    a1slist_pop_front(&a1slist);
    ASSERT(a1slist_is_empty(&a1slist) == 1);
}

static void test_a1slist_push_after(void) {
    test_init(&a1slist);

    a1slist_push_front(&a1slist, &test_data[0].hdr);

    a1slist_push_after(&a1slist, &test_data[0].hdr, &test_data[1].hdr);
    struct a1slist_item *e = a1slist_peek_front(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    a1slist_pop_front(&a1slist);

    e = a1slist_peek_front(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_a1slist_push_after_2(void) {
    test_init(&a1slist);

    a1slist_push_back(&a1slist, &test_data[0].hdr);
    a1slist_push_back(&a1slist, &test_data[1].hdr);

    a1slist_push_after(&a1slist, &test_data[1].hdr, &test_data[2].hdr);
    struct a1slist_item *e = a1slist_peek_back(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);

    e = a1slist_peek_front(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_a1slist_push_after_3(void) {
    test_init(&a1slist);

    a1slist_push_back(&a1slist, &test_data[0].hdr);
    a1slist_push_back(&a1slist, &test_data[2].hdr);

    a1slist_push_after(&a1slist, &test_data[0].hdr, &test_data[1].hdr);
    struct a1slist_item *e = a1slist_peek_front(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1slist_peek_back(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_a1slist_pop(void) {
    test_init(&a1slist);

    a1slist_push_front(&a1slist, &test_data[2].hdr);
    a1slist_push_front(&a1slist, &test_data[1].hdr);
    a1slist_push_front(&a1slist, &test_data[0].hdr);

    struct a1slist_item *e = a1slist_pop_front(&a1slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = a1slist_pop_after(&a1slist, &test_data[1].hdr);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct a1slist_item *item) {
    int v = *(int *)context;
    struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_a1slist_find(void) {
    test_init(&a1slist);

    a1slist_push_front(&a1slist, &test_data[2].hdr);
    a1slist_push_front(&a1slist, &test_data[1].hdr);
    a1slist_push_front(&a1slist, &test_data[0].hdr);

    int v;
    struct a1slist_item *e;

    for (v = 0; v < 3; v++) {
        e = a1slist_find(&a1slist, predicate, &v);
        ASSERT(e != NULL);
        struct ut_data *d = (struct ut_data *)e;
        ASSERT(v == d->data);
    }

    v = 3;
    e = a1slist_find(&a1slist, predicate, &v);
    ASSERT(NULL == e);
}

static void test_a1slist_owns(void) {
    test_init(&a1slist);

    for (int i = 0; i < 2; i++) {
        a1slist_push_front(&a1slist, &test_data[i].hdr);
        ASSERT(a1slist_owns(&a1slist, &test_data[i].hdr));
    }

    a1slist_pop_front(&a1slist);

    ASSERT(0 == a1slist_owns(&a1slist, &test_data[1].hdr));
}

int main(void) {
    test_a1slist_empty();

    test_a1slist_push_after();
    test_a1slist_push_after_2();
    test_a1slist_push_after_3();

    test_a1slist_pop();
    test_a1slist_find();

    test_a1slist_owns();

    return 0;
}
