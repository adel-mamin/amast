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

#include <stdlib.h>
#include <stdbool.h>

#include "common/compiler.h" /* IWYU pragma: keep */
#include "common/macros.h"
#include "slist/slist.h"

static struct am_slist slist;

struct test_slist {
    struct am_slist_item hdr;
    int data;
};

static struct test_slist test_slist[10];

static void test_slist_init(struct am_slist *data) {
    am_slist_init(data);
    for (int i = 0; i < AM_COUNTOF(test_slist); i++) {
        test_slist[i].data = i;
    }
}

static void test_am_slist_empty(void) {
    test_slist_init(&slist);

    AM_ASSERT(am_slist_is_empty(&slist) == 1);

    am_slist_push_back(&slist, &test_slist[0].hdr);
    AM_ASSERT(am_slist_is_empty(&slist) == 0);

    am_slist_pop_front(&slist);
    AM_ASSERT(am_slist_is_empty(&slist) == 1);
}

static void test_am_slist_push_after(void) {
    test_slist_init(&slist);

    am_slist_push_front(&slist, &test_slist[0].hdr);

    am_slist_push_after(&slist, &test_slist[0].hdr, &test_slist[1].hdr);
    struct am_slist_item *e = am_slist_peek_front(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 0);

    am_slist_pop_front(&slist);

    e = am_slist_peek_front(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 1);
}

static void test_am_slist_push_after_2(void) {
    test_slist_init(&slist);

    am_slist_push_back(&slist, &test_slist[0].hdr);
    am_slist_push_back(&slist, &test_slist[1].hdr);

    am_slist_push_after(&slist, &test_slist[1].hdr, &test_slist[2].hdr);
    struct am_slist_item *e = am_slist_peek_back(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 2);

    e = am_slist_peek_front(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 0);
}

static void test_am_slist_push_after_3(void) {
    test_slist_init(&slist);

    am_slist_push_back(&slist, &test_slist[0].hdr);
    am_slist_push_back(&slist, &test_slist[2].hdr);

    am_slist_push_after(&slist, &test_slist[0].hdr, &test_slist[1].hdr);
    struct am_slist_item *e = am_slist_peek_front(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 0);

    e = am_slist_peek_back(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 2);
}

static void test_am_slist_pop(void) {
    test_slist_init(&slist);

    am_slist_push_front(&slist, &test_slist[2].hdr);
    am_slist_push_front(&slist, &test_slist[1].hdr);
    am_slist_push_front(&slist, &test_slist[0].hdr);

    struct am_slist_item *e = am_slist_pop_front(&slist);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 0);

    e = am_slist_pop_after(&slist, &test_slist[1].hdr);
    AM_ASSERT(e);
    AM_ASSERT(((struct test_slist *)e)->data == 2);
}

static bool predicate_slist(void *context, struct am_slist_item *item) {
    int v = *(int *)context;
    const struct test_slist *data = (struct test_slist *)item;

    return (v == data->data);
}

static void test_am_slist_find(void) {
    test_slist_init(&slist);

    am_slist_push_front(&slist, &test_slist[2].hdr);
    am_slist_push_front(&slist, &test_slist[1].hdr);
    am_slist_push_front(&slist, &test_slist[0].hdr);

    int v;
    struct am_slist_item *e;

    for (v = 0; v < 3; v++) {
        e = am_slist_find(&slist, predicate_slist, &v);
        AM_ASSERT(e != NULL);
        const struct test_slist *d = (struct test_slist *)e;
        AM_ASSERT(v == d->data);
    }

    v = 3;
    e = am_slist_find(&slist, predicate_slist, &v);
    AM_ASSERT(NULL == e);
}

static void test_am_slist_owns(void) {
    test_slist_init(&slist);

    for (int i = 0; i < 2; i++) {
        am_slist_push_front(&slist, &test_slist[i].hdr);
        AM_ASSERT(am_slist_owns(&slist, &test_slist[i].hdr));
    }

    am_slist_pop_front(&slist);

    AM_ASSERT(0 == am_slist_owns(&slist, &test_slist[1].hdr));
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
