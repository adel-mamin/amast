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

void am_dlist_init(struct am_dlist *hnd) {
    hnd->sentinel.next = hnd->sentinel.prev = &hnd->sentinel;
}

void am_dlist_item_init(struct am_dlist_item *item) {
    item->next = item->prev = NULL;
}

struct am_dlist_item *am_dlist_next(
    const struct am_dlist *hnd, const struct am_dlist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    return (&hnd->sentinel == item->next) ? NULL : item->next;
}

struct am_dlist_item *am_dlist_prev(
    const struct am_dlist *hnd, const struct am_dlist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    return (&hnd->sentinel == item->prev) ? NULL : item->prev;
}

void am_dlist_push_after(
    struct am_dlist_item *item, struct am_dlist_item *new_item
) {
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(new_item);

    new_item->next = item->next;
    new_item->prev = item;
    item->next->prev = new_item;
    item->next = new_item;
}

void am_dlist_push_before(
    struct am_dlist_item *item, struct am_dlist_item *new_item
) {
    AM_ASSERT(item);
    AM_ASSERT(item->prev);
    AM_ASSERT(new_item);

    new_item->next = item;
    new_item->prev = item->prev;
    item->prev->next = new_item;
    item->prev = new_item;
}

void am_dlist_iterator_init(
    struct am_dlist *hnd,
    struct am_dlist_iterator *it,
    enum am_dlist_direction direction
) {
    it->hnd = hnd;
    it->cur = &hnd->sentinel;
    it->dir = direction;
}

struct am_dlist_item *am_dlist_iterator_next(struct am_dlist_iterator *it) {
    AM_ASSERT(it);
    AM_ASSERT(it->hnd);
    AM_ASSERT(it->cur);

    if (AM_DLIST_FORWARD == it->dir) {
        it->cur = it->cur->next;
    } else {
        it->cur = it->cur->prev;
    }

    if (it->cur == &it->hnd->sentinel) {
        it->cur = NULL;
        return NULL;
    }

    AM_ASSERT(NULL != it->cur);

    return it->cur;
}

struct am_dlist_item *am_dlist_iterator_pop(struct am_dlist_iterator *it) {
    AM_ASSERT(it->cur != &it->hnd->sentinel);
    struct am_dlist_item *pop = it->cur;
    it->cur = it->cur->prev;
    am_dlist_pop(pop);
    return pop;
}

bool am_dlist_pop(struct am_dlist_item *item) {
    AM_ASSERT(item);

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

struct am_dlist_item *am_dlist_find(
    const struct am_dlist *hnd,
    am_dlist_item_found_cb_t is_found_cb,
    void *context
) {
    AM_ASSERT(hnd);
    AM_ASSERT(is_found_cb);

    struct am_dlist_item *candidate = hnd->sentinel.next;
    while ((candidate != &hnd->sentinel) && (!is_found_cb(context, candidate))
    ) {
        candidate = candidate->next;
    }
    return (candidate == &hnd->sentinel) ? NULL : candidate;
}

int am_dlist_size(const struct am_dlist *hnd) {
    AM_ASSERT(hnd);

    if (am_dlist_is_empty(hnd)) {
        return 0;
    }

    int size = 0;
    struct am_dlist_item *item = hnd->sentinel.next;
    while ((item != &hnd->sentinel) && (item != NULL)) {
        size++;
        item = item->next;
    }
    AM_ASSERT(item != NULL);

    return size;
}

bool am_dlist_owns(
    const struct am_dlist *hnd, const struct am_dlist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);

    struct am_dlist_item *next = hnd->sentinel.next;
    while (next != &hnd->sentinel) {
        if (item == next) {
            return true;
        }
        next = next->next;
    }

    return false;
}

void am_dlist_push_front(struct am_dlist *hnd, struct am_dlist_item *item) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    am_dlist_push_after(&hnd->sentinel, item);
}

struct am_dlist_item *am_dlist_pop_front(struct am_dlist *hnd) {
    if (am_dlist_is_empty(hnd)) {
        return NULL;
    }
    struct am_dlist_item *ret = hnd->sentinel.next;
    am_dlist_pop(hnd->sentinel.next);
    return ret;
}

struct am_dlist_item *am_dlist_pop_back(struct am_dlist *hnd) {
    if (am_dlist_is_empty(hnd)) {
        return NULL;
    }
    struct am_dlist_item *ret = hnd->sentinel.prev;
    am_dlist_pop(hnd->sentinel.prev);
    return ret;
}

void am_dlist_push_back(struct am_dlist *hnd, struct am_dlist_item *item) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    am_dlist_push_before(&hnd->sentinel, item);
}

struct am_dlist_item *am_dlist_peek_front(struct am_dlist *hnd) {
    AM_ASSERT(hnd);
    return am_dlist_is_empty(hnd) ? NULL : hnd->sentinel.next;
}

struct am_dlist_item *am_dlist_peek_back(struct am_dlist *hnd) {
    AM_ASSERT(hnd);
    return am_dlist_is_empty(hnd) ? NULL : hnd->sentinel.prev;
}

bool am_dlist_is_empty(const struct am_dlist *hnd) {
    AM_ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

bool am_dlist_item_is_linked(const struct am_dlist_item *item) {
    AM_ASSERT(item);
    return (NULL != item->next) && (NULL != item->prev);
}
