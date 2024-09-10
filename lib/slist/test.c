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

static struct am_slist am_slist;

struct ut_data {
    struct am_slist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_init(struct am_slist *data) {
    am_slist_init(data);
    for (int i = 0; i < COUNTOF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_am_slist_empty(void) {
    test_init(&am_slist);

    ASSERT(am_slist_is_empty(&am_slist) == 1);

    am_slist_push_back(&am_slist, &test_data[0].hdr);
    ASSERT(am_slist_is_empty(&am_slist) == 0);

    am_slist_pop_front(&am_slist);
    ASSERT(am_slist_is_empty(&am_slist) == 1);
}

static void test_am_slist_push_after(void) {
    test_init(&am_slist);

    am_slist_push_front(&am_slist, &test_data[0].hdr);

    am_slist_push_after(&am_slist, &test_data[0].hdr, &test_data[1].hdr);
    struct am_slist_item *e = am_slist_peek_front(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    am_slist_pop_front(&am_slist);

    e = am_slist_peek_front(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_am_slist_push_after_2(void) {
    test_init(&am_slist);

    am_slist_push_back(&am_slist, &test_data[0].hdr);
    am_slist_push_back(&am_slist, &test_data[1].hdr);

    am_slist_push_after(&am_slist, &test_data[1].hdr, &test_data[2].hdr);
    struct am_slist_item *e = am_slist_peek_back(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);

    e = am_slist_peek_front(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_am_slist_push_after_3(void) {
    test_init(&am_slist);

    am_slist_push_back(&am_slist, &test_data[0].hdr);
    am_slist_push_back(&am_slist, &test_data[2].hdr);

    am_slist_push_after(&am_slist, &test_data[0].hdr, &test_data[1].hdr);
    struct am_slist_item *e = am_slist_peek_front(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = am_slist_peek_back(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_am_slist_pop(void) {
    test_init(&am_slist);

    am_slist_push_front(&am_slist, &test_data[2].hdr);
    am_slist_push_front(&am_slist, &test_data[1].hdr);
    am_slist_push_front(&am_slist, &test_data[0].hdr);

    struct am_slist_item *e = am_slist_pop_front(&am_slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = am_slist_pop_after(&am_slist, &test_data[1].hdr);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct am_slist_item *item) {
    int v = *(int *)context;
    struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_am_slist_find(void) {
    test_init(&am_slist);

    am_slist_push_front(&am_slist, &test_data[2].hdr);
    am_slist_push_front(&am_slist, &test_data[1].hdr);
    am_slist_push_front(&am_slist, &test_data[0].hdr);

    int v;
    struct am_slist_item *e;

    for (v = 0; v < 3; v++) {
        e = am_slist_find(&am_slist, predicate, &v);
        ASSERT(e != NULL);
        struct ut_data *d = (struct ut_data *)e;
        ASSERT(v == d->data);
    }

    v = 3;
    e = am_slist_find(&am_slist, predicate, &v);
    ASSERT(NULL == e);
}

static void test_am_slist_owns(void) {
    test_init(&am_slist);

    for (int i = 0; i < 2; i++) {
        am_slist_push_front(&am_slist, &test_data[i].hdr);
        ASSERT(am_slist_owns(&am_slist, &test_data[i].hdr));
    }

    am_slist_pop_front(&am_slist);

    ASSERT(0 == am_slist_owns(&am_slist, &test_data[1].hdr));
}

int main(void) {
    test_am_slist_empty();

    test_am_slist_push_after();
    test_am_slist_push_after_2();
    test_am_slist_push_after_3();

    test_am_slist_pop();
    test_am_slist_find();

    test_am_slist_owns();

    return 0;
}
