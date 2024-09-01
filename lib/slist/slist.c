/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018,2024 Adel Mamin
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

void slist_init(struct slist *hnd) {
    ASSERT(hnd);
    hnd->sentinel.next = &hnd->sentinel;
    hnd->back = &hnd->sentinel;
}

bool slist_is_empty(const struct slist *hnd) {
    ASSERT(hnd);
    return hnd->sentinel.next == &hnd->sentinel;
}

void slist_push_after(
    struct slist *hnd, struct slist_item *item, struct slist_item *newitem
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

struct slist_item *slist_pop_after(struct slist *hnd, struct slist_item *item) {
    ASSERT(hnd);
    ASSERT(item);
    ASSERT(item->next);
    ASSERT(item->next->next);

    struct slist_item *pop = item->next;
    if (pop == &hnd->sentinel) {
        return NULL;
    }
    if (hnd->back == pop) {
        hnd->back = item;
    }
    item->next = pop->next;

    return pop;
}

struct slist_item *slist_next_item(
    const struct slist *hnd, const struct slist_item *item
) {
    ASSERT(hnd);
    ASSERT(item);
    return (item->next == &hnd->sentinel) ? NULL : item->next;
}

struct slist_item *slist_find(
    const struct slist *hnd, slist_item_found_cb_t is_found_cb, void *context
) {
    ASSERT(hnd);
    ASSERT(is_found_cb);

    if (slist_is_empty(hnd)) {
        return NULL;
    }

    struct slist_item *candidate = hnd->sentinel.next;
    while ((candidate != &hnd->sentinel) && (!is_found_cb(context, candidate))
    ) {
        candidate = candidate->next;
    }
    return (candidate == &hnd->sentinel) ? NULL : candidate;
}

struct slist_item *slist_peek_front(const struct slist *hnd) {
    ASSERT(hnd);
    return slist_is_empty(hnd) ? NULL : hnd->sentinel.next;
}

struct slist_item *slist_peek_back(const struct slist *hnd) {
    ASSERT(hnd);
    return slist_is_empty(hnd) ? NULL : hnd->back;
}

bool slist_owns(const struct slist *hnd, const struct slist_item *item) {
    ASSERT(hnd);
    ASSERT(item);

    struct slist_item *next = hnd->sentinel.next;
    while (next != &hnd->sentinel) {
        if (next == item) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void slist_push_front(struct slist *hnd, struct slist_item *item) {
    slist_push_after(hnd, &hnd->sentinel, item);
}

struct slist_item *slist_pop_front(struct slist *hnd) {
    return slist_pop_after(hnd, &hnd->sentinel);
}

void slist_move_to_head(
    struct slist *from, struct slist *to, struct slist_item *item
) {
    ASSERT(from);
    ASSERT(to);
    ASSERT(item);

    struct slist_item *prev = &from->sentinel;
    struct slist_item *cur = slist_peek_front(from);
    while (cur) {
        if (cur == item) {
            slist_pop_after(from, prev);
            slist_push_front(to, cur);
            return;
        }
        struct slist_item *tmp = cur;
        cur = slist_next_item(from, prev);
        prev = tmp;
    }
}

void slist_push_back(struct slist *hnd, struct slist_item *item) {
    slist_push_after(hnd, hnd->back, item);
}
