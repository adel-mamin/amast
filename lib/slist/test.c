/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2016-2018 Adel Mamin
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

static struct slist slist;

struct ut_data {
    struct slist_item hdr;
    int data;
};

static struct ut_data test_data[10];

static void test_init(struct slist *data) {
    slist_init(data);
    for (int i = 0; i < COUNTOF(test_data); i++) {
        test_data[i].data = i;
    }
}

static void test_slist_empty(void) {
    test_init(&slist);

    ASSERT(slist_is_empty(&slist) == 1);

    slist_push_back(&slist, &test_data[0].hdr);
    ASSERT(slist_is_empty(&slist) == 0);

    slist_pop_front(&slist);
    ASSERT(slist_is_empty(&slist) == 1);
}

static void test_slist_push_after(void) {
    test_init(&slist);

    slist_push_front(&slist, &test_data[0].hdr);

    slist_push_after(&slist, &test_data[0].hdr, &test_data[1].hdr);
    struct slist_item *e = slist_peek_front(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    slist_pop_front(&slist);

    e = slist_peek_front(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 1);
}

static void test_slist_push_after_2(void) {
    test_init(&slist);

    slist_push_back(&slist, &test_data[0].hdr);
    slist_push_back(&slist, &test_data[1].hdr);

    slist_push_after(&slist, &test_data[1].hdr, &test_data[2].hdr);
    struct slist_item *e = slist_peek_back(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);

    e = slist_peek_front(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);
}

static void test_slist_push_after_3(void) {
    test_init(&slist);

    slist_push_back(&slist, &test_data[0].hdr);
    slist_push_back(&slist, &test_data[2].hdr);

    slist_push_after(&slist, &test_data[0].hdr, &test_data[1].hdr);
    struct slist_item *e = slist_peek_front(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = slist_peek_back(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static void test_slist_pop(void) {
    test_init(&slist);

    slist_push_front(&slist, &test_data[2].hdr);
    slist_push_front(&slist, &test_data[1].hdr);
    slist_push_front(&slist, &test_data[0].hdr);

    struct slist_item *e = slist_pop_front(&slist);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 0);

    e = slist_pop_after(&slist, &test_data[1].hdr);
    ASSERT(e);
    ASSERT(((struct ut_data *)e)->data == 2);
}

static bool predicate(void *context, struct slist_item *item) {
    int v = *(int *)context;
    struct ut_data *data = (struct ut_data *)item;

    return (v == data->data);
}

static void test_slist_find(void) {
    test_init(&slist);

    slist_push_front(&slist, &test_data[2].hdr);
    slist_push_front(&slist, &test_data[1].hdr);
    slist_push_front(&slist, &test_data[0].hdr);

    int v;
    struct slist_item *e;

    for (v = 0; v < 3; v++) {
        e = slist_find(&slist, predicate, &v);
        ASSERT(e != NULL);
        struct ut_data *d = (struct ut_data *)e;
        ASSERT(v == d->data);
    }

    v = 3;
    e = slist_find(&slist, predicate, &v);
    ASSERT(NULL == e);
}

static void test_slist_owns(void) {
    test_init(&slist);

    for (int i = 0; i < 2; i++) {
        slist_push_front(&slist, &test_data[i].hdr);
        ASSERT(slist_owns(&slist, &test_data[i].hdr));
    }

    slist_pop_front(&slist);

    ASSERT(0 == slist_owns(&slist, &test_data[1].hdr));
}

int main(void) {
    test_slist_empty();

    test_slist_push_after();
    test_slist_push_after_2();
    test_slist_push_after_3();

    test_slist_pop();
    test_slist_find();

    test_slist_owns();

    return 0;
}
