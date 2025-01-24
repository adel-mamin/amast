/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 *
 * Source: https://github.com/adel-mamin/amast
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

#include <stddef.h>
#include <stdbool.h>

#include "common/macros.h"
#include "slist/slist.h"

struct am_alignof_slist {
    char c;            /* cppcheck-suppress unusedStructMember */
    struct am_slist d; /* cppcheck-suppress unusedStructMember */
};
const int am_alignof_slist = offsetof(struct am_alignof_slist, d);

struct am_alignof_slist_item {
    char c;                 /* cppcheck-suppress unusedStructMember */
    struct am_slist_item d; /* cppcheck-suppress unusedStructMember */
};
const int am_alignof_slist_item = offsetof(struct am_alignof_slist_item, d);

void am_slist_init(struct am_slist *hnd) {
    AM_ASSERT(hnd);
    hnd->sentinel.next = &hnd->sentinel;
    hnd->back = &hnd->sentinel;
}

bool am_slist_is_empty(const struct am_slist *hnd) {
    AM_ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

bool am_slist_item_is_linked(const struct am_slist_item *item) {
    AM_ASSERT(item);
    return item->next != NULL;
}

void am_slist_item_init(struct am_slist_item *item) {
    AM_ASSERT(item);
    item->next = NULL;
}

void am_slist_push_after(
    struct am_slist *hnd,
    struct am_slist_item *item,
    struct am_slist_item *newitem
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(newitem);

    newitem->next = item->next;
    item->next = newitem;
    if (hnd->back == item) {
        hnd->back = newitem;
    }
}

struct am_slist_item *am_slist_pop_after(
    struct am_slist *hnd, struct am_slist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(item->next->next);

    struct am_slist_item *pop = item->next;
    if (pop == &hnd->sentinel) {
        return NULL;
    }
    if (hnd->back == pop) {
        hnd->back = item;
    }
    item->next = pop->next;

    return pop;
}

struct am_slist_item *am_slist_next_item(
    const struct am_slist *hnd, const struct am_slist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);
    return (item->next == &hnd->sentinel) ? NULL : item->next;
}

struct am_slist_item *am_slist_find(
    const struct am_slist *hnd,
    am_slist_item_found_cb_t is_found_cb,
    void *context
) {
    AM_ASSERT(hnd);
    AM_ASSERT(is_found_cb);

    struct am_slist_item *item = hnd->sentinel.next;
    while ((item != &hnd->sentinel) && !is_found_cb(context, item)) {
        item = item->next;
    }
    return (item == &hnd->sentinel) ? NULL : item;
}

struct am_slist_item *am_slist_peek_front(const struct am_slist *hnd) {
    AM_ASSERT(hnd);
    return am_slist_is_empty(hnd) ? NULL : hnd->sentinel.next;
}

struct am_slist_item *am_slist_peek_back(const struct am_slist *hnd) {
    AM_ASSERT(hnd);
    return am_slist_is_empty(hnd) ? NULL : hnd->back;
}

bool am_slist_owns(
    const struct am_slist *hnd, const struct am_slist_item *item
) {
    AM_ASSERT(hnd);
    AM_ASSERT(item);

    struct am_slist_item *next = hnd->sentinel.next;
    while (next != &hnd->sentinel) {
        if (next == item) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void am_slist_push_front(struct am_slist *hnd, struct am_slist_item *item) {
    am_slist_push_after(hnd, &hnd->sentinel, item);
}

struct am_slist_item *am_slist_pop_front(struct am_slist *hnd) {
    return am_slist_pop_after(hnd, &hnd->sentinel);
}

void am_slist_push_back(struct am_slist *hnd, struct am_slist_item *item) {
    am_slist_push_after(hnd, hnd->back, item);
}

void am_slist_append(struct am_slist *to, struct am_slist *from) {
    AM_ASSERT(to);
    AM_ASSERT(from);
    if (am_slist_is_empty(from)) {
        return;
    }
    to->back->next = from->sentinel.next;
    to->back = from->back;
    from->back->next = &to->sentinel;

    am_slist_init(from);
}

void am_slist_iterator_init(
    struct am_slist *hnd, struct am_slist_iterator *it
) {
    AM_ASSERT(hnd);
    AM_ASSERT(it);

    it->hnd = hnd;
    it->cur = &hnd->sentinel;
    it->prev = NULL;
}

struct am_slist_item *am_slist_iterator_next(struct am_slist_iterator *it) {
    AM_ASSERT(it);
    AM_ASSERT(it->cur);

    it->prev = it->cur;
    it->cur = it->cur->next;

    return (it->cur == &it->hnd->sentinel) ? NULL : it->cur;
}

struct am_slist_item *am_slist_iterator_pop(struct am_slist_iterator *it) {
    AM_ASSERT(it->prev);
    AM_ASSERT(it->cur);

    struct am_slist_item *pop = it->cur;
    it->prev->next = pop->next;
    if (it->hnd->back == pop) {
        it->hnd->back = it->prev;
    }
    it->cur = it->prev;
    it->prev = NULL;

    pop->next = NULL;

    return pop;
}
