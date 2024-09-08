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
 * Singly linked list implementation.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "common/macros.h"
#include "common/compiler.h"
#include "slist/slist.h"

void a1slist_init(struct a1slist *hnd) {
    ASSERT(hnd);
    hnd->sentinel.next = &hnd->sentinel;
    hnd->back = &hnd->sentinel;
}

bool a1slist_is_empty(const struct a1slist *hnd) {
    ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

void a1slist_push_after(
    struct a1slist *hnd, struct a1slist_item *item, struct a1slist_item *newitem
) {
    ASSERT(item);
    ASSERT(item->next);
    ASSERT(newitem);

    newitem->next = item->next;
    item->next = newitem;
    if (hnd->back == item) {
        hnd->back = newitem;
    }
}

struct a1slist_item *a1slist_pop_after(
    struct a1slist *hnd, struct a1slist_item *item
) {
    ASSERT(hnd);
    ASSERT(item);
    ASSERT(item->next);
    ASSERT(item->next->next);

    struct a1slist_item *pop = item->next;
    if (pop == &hnd->sentinel) {
        return NULL;
    }
    if (hnd->back == pop) {
        hnd->back = item;
    }
    item->next = pop->next;

    return pop;
}

struct a1slist_item *a1slist_next_item(
    const struct a1slist *hnd, const struct a1slist_item *item
) {
    ASSERT(hnd);
    ASSERT(item);
    return (item->next == &hnd->sentinel) ? NULL : item->next;
}

struct a1slist_item *a1slist_find(
    const struct a1slist *hnd,
    a1slist_item_found_cb_t is_found_cb,
    void *context
) {
    ASSERT(hnd);
    ASSERT(is_found_cb);

    if (a1slist_is_empty(hnd)) {
        return NULL;
    }

    struct a1slist_item *candidate = hnd->sentinel.next;
    while ((candidate != &hnd->sentinel) && (!is_found_cb(context, candidate))
    ) {
        candidate = candidate->next;
    }
    return (candidate == &hnd->sentinel) ? NULL : candidate;
}

struct a1slist_item *a1slist_peek_front(const struct a1slist *hnd) {
    ASSERT(hnd);
    return a1slist_is_empty(hnd) ? NULL : hnd->sentinel.next;
}

struct a1slist_item *a1slist_peek_back(const struct a1slist *hnd) {
    ASSERT(hnd);
    return a1slist_is_empty(hnd) ? NULL : hnd->back;
}

bool a1slist_owns(const struct a1slist *hnd, const struct a1slist_item *item) {
    ASSERT(hnd);
    ASSERT(item);

    struct a1slist_item *next = hnd->sentinel.next;
    while (next != &hnd->sentinel) {
        if (next == item) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void a1slist_push_front(struct a1slist *hnd, struct a1slist_item *item) {
    a1slist_push_after(hnd, &hnd->sentinel, item);
}

struct a1slist_item *a1slist_pop_front(struct a1slist *hnd) {
    return a1slist_pop_after(hnd, &hnd->sentinel);
}

void a1slist_move_to_head(
    struct a1slist *from, struct a1slist *to, struct a1slist_item *item
) {
    ASSERT(from);
    ASSERT(to);
    ASSERT(item);

    struct a1slist_item *prev = &from->sentinel;
    struct a1slist_item *cur = a1slist_peek_front(from);
    while (cur) {
        if (cur == item) {
            a1slist_pop_after(from, prev);
            a1slist_push_front(to, cur);
            return;
        }
        struct a1slist_item *tmp = cur;
        cur = a1slist_next_item(from, prev);
        prev = tmp;
    }
}

void a1slist_push_back(struct a1slist *hnd, struct a1slist_item *item) {
    a1slist_push_after(hnd, hnd->back, item);
}
