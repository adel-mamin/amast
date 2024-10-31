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

#include <stdlib.h>
#include <stdbool.h>

#include "common/compiler.h" /* IWYU pragma: keep */
#include "common/macros.h"
#include "slist/slist.h"

void am_slist_init(struct am_slist *hnd) {
    AM_ASSERT(hnd);
    hnd->sentinel.next = &hnd->sentinel;
    hnd->back = &hnd->sentinel;
}

bool am_slist_is_empty(const struct am_slist *hnd) {
    AM_ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

void am_slist_push_after(
    struct am_slist *hnd,
    struct am_slist_item *item,
    struct am_slist_item *newitem
) {
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

    if (am_slist_is_empty(hnd)) {
        return NULL;
    }

    struct am_slist_item *candidate = hnd->sentinel.next;
    while ((candidate != &hnd->sentinel) && (!is_found_cb(context, candidate))
    ) {
        candidate = candidate->next;
    }
    return (candidate == &hnd->sentinel) ? NULL : candidate;
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
