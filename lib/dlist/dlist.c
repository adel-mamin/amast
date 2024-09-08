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
 * Doubly linked list implementation.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "dlist/dlist.h"

void a1dlist_init(struct a1dlist *hnd) {
    hnd->sentinel.next = hnd->sentinel.prev = &hnd->sentinel;
}

void a1dlist_item_init(struct a1dlist_item *item) {
    item->next = item->prev = NULL;
}

struct a1dlist_item *a1dlist_next(
    const struct a1dlist *hnd, const struct a1dlist_item *item
) {
    ASSERT(hnd);
    ASSERT(item);
    return (&hnd->sentinel == item->next) ? NULL : item->next;
}

struct a1dlist_item *a1dlist_prev(
    const struct a1dlist *hnd, const struct a1dlist_item *item
) {
    ASSERT(hnd);
    ASSERT(item);
    return (&hnd->sentinel == item->prev) ? NULL : item->prev;
}

void a1dlist_push_after(
    struct a1dlist_item *item, struct a1dlist_item *new_item
) {
    ASSERT(item);
    ASSERT(item->next);
    ASSERT(new_item);

    new_item->next = item->next;
    new_item->prev = item;
    item->next->prev = new_item;
    item->next = new_item;
}

void a1dlist_push_before(
    struct a1dlist_item *item, struct a1dlist_item *new_item
) {
    ASSERT(item);
    ASSERT(item->prev);
    ASSERT(new_item);

    new_item->next = item;
    new_item->prev = item->prev;
    item->prev->next = new_item;
    item->prev = new_item;
}

void a1dlist_iterator_init(
    struct a1dlist *hnd,
    struct a1dlist_iterator *it,
    enum a1dlist_direction direction
) {
    it->hnd = hnd;
    it->cur = &hnd->sentinel;
    it->dir = direction;
}

struct a1dlist_item *a1dlist_iterator_next(struct a1dlist_iterator *it) {
    ASSERT(it);
    ASSERT(it->hnd);
    ASSERT(it->cur);

    if (A1DLIST_FORWARD == it->dir) {
        it->cur = it->cur->next;
    } else {
        it->cur = it->cur->prev;
    }

    if (it->cur == &it->hnd->sentinel) {
        it->cur = NULL;
        return NULL;
    }

    ASSERT(NULL != it->cur);

    return it->cur;
}

struct a1dlist_item *a1dlist_iterator_pop(struct a1dlist_iterator *it) {
    ASSERT(it->cur != &it->hnd->sentinel);
    struct a1dlist_item *pop = it->cur;
    it->cur = it->cur->prev;
    a1dlist_pop(pop);
    return pop;
}

bool a1dlist_pop(struct a1dlist_item *item) {
    ASSERT(item);

    if (item->next) {
        item->next->prev = item->prev;
    }
    if (item->prev) {
        item->prev->next = item->next;
    }
    bool popped = item->next && item->prev;
    item->next = item->prev = NULL;
    return popped;
}

struct a1dlist_item *a1dlist_find(
    const struct a1dlist *hnd,
    a1dlist_item_found_cb_t is_found_cb,
    void *context
) {
    ASSERT(hnd);
    ASSERT(is_found_cb);

    struct a1dlist_item *candidate = hnd->sentinel.next;
    while ((candidate != &hnd->sentinel) && (!is_found_cb(context, candidate))
    ) {
        candidate = candidate->next;
    }
    return (candidate == &hnd->sentinel) ? NULL : candidate;
}

int a1dlist_size(const struct a1dlist *hnd) {
    ASSERT(hnd);

    if (a1dlist_is_empty(hnd)) {
        return 0;
    }

    int size = 0;
    struct a1dlist_item *item = hnd->sentinel.next;
    while ((item != &hnd->sentinel) && (item != NULL)) {
        size++;
        item = item->next;
    }
    ASSERT(item != NULL);

    return size;
}

bool a1dlist_owns(const struct a1dlist *hnd, const struct a1dlist_item *item) {
    ASSERT(hnd);
    ASSERT(item);

    struct a1dlist_item *next = hnd->sentinel.next;
    while (next != &hnd->sentinel) {
        if (item == next) {
            return true;
        }
        next = next->next;
    }

    return false;
}

void a1dlist_push_front(struct a1dlist *hnd, struct a1dlist_item *item) {
    ASSERT(hnd);
    ASSERT(item);
    a1dlist_push_after(&hnd->sentinel, item);
}

struct a1dlist_item *a1dlist_pop_front(struct a1dlist *hnd) {
    if (a1dlist_is_empty(hnd)) {
        return NULL;
    }
    struct a1dlist_item *ret = hnd->sentinel.next;
    a1dlist_pop(hnd->sentinel.next);
    return ret;
}

struct a1dlist_item *a1dlist_pop_back(struct a1dlist *hnd) {
    if (a1dlist_is_empty(hnd)) {
        return NULL;
    }
    struct a1dlist_item *ret = hnd->sentinel.prev;
    a1dlist_pop(hnd->sentinel.prev);
    return ret;
}

void a1dlist_push_back(struct a1dlist *hnd, struct a1dlist_item *item) {
    ASSERT(hnd);
    ASSERT(item);
    a1dlist_push_before(&hnd->sentinel, item);
}

struct a1dlist_item *a1dlist_peek_front(struct a1dlist *hnd) {
    ASSERT(hnd);
    return a1dlist_is_empty(hnd) ? NULL : hnd->sentinel.next;
}

struct a1dlist_item *a1dlist_peek_back(struct a1dlist *hnd) {
    ASSERT(hnd);
    return a1dlist_is_empty(hnd) ? NULL : hnd->sentinel.prev;
}

bool a1dlist_is_empty(const struct a1dlist *hnd) {
    ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

bool a1dlist_item_is_linked(const struct a1dlist_item *item) {
    ASSERT(item);
    return (NULL != item->next) && (NULL != item->prev);
}
