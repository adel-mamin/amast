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
 * Doubly linked list implementation.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "common/macros.h"
#include "dlist/dlist.h"

void am_dlist_init(struct am_dlist *list) {
    list->sentinel.next = list->sentinel.prev = &list->sentinel;
}

void am_dlist_item_init(struct am_dlist_item *item) {
    item->next = item->prev = NULL;
}

struct am_dlist_item *am_dlist_next(
    const struct am_dlist *list, const struct am_dlist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    return (&list->sentinel == item->next) ? NULL : item->next;
}

struct am_dlist_item *am_dlist_prev(
    const struct am_dlist *list, const struct am_dlist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    return (&list->sentinel == item->prev) ? NULL : item->prev;
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
    struct am_dlist *list,
    struct am_dlist_iterator *it,
    enum am_dlist_direction dir
) {
    it->list = list;
    it->cur = &list->sentinel;
    it->dir = dir;
}

struct am_dlist_item *am_dlist_iterator_next(struct am_dlist_iterator *it) {
    AM_ASSERT(it);
    AM_ASSERT(it->list);
    AM_ASSERT(it->cur);

    if (AM_DLIST_FORWARD == it->dir) {
        it->cur = it->cur->next;
    } else {
        it->cur = it->cur->prev;
    }

    if (it->cur == &it->list->sentinel) {
        it->cur = NULL;
    }
    return it->cur;
}

struct am_dlist_item *am_dlist_iterator_pop(struct am_dlist_iterator *it) {
    AM_ASSERT(it->cur != &it->list->sentinel);
    struct am_dlist_item *pop = it->cur;

    if (AM_DLIST_FORWARD == it->dir) {
        it->cur = it->cur->prev;
    } else {
        it->cur = it->cur->next;
    }
    am_dlist_pop(pop);
    return pop;
}

void am_dlist_pop(struct am_dlist_item *item) {
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(item->prev);

    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->next = item->prev = NULL;
}

struct am_dlist_item *am_dlist_find(
    const struct am_dlist *list, am_dlist_item_found_fn is_found, void *context
) {
    AM_ASSERT(list);
    AM_ASSERT(is_found);

    struct am_dlist_item *item = list->sentinel.next;
    while ((item != &list->sentinel) && !is_found(context, item)) {
        item = item->next;
    }
    return (item == &list->sentinel) ? NULL : item;
}

bool am_dlist_owns(
    const struct am_dlist *list, const struct am_dlist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);

    struct am_dlist_item *next = list->sentinel.next;
    while (next != &list->sentinel) {
        if (item == next) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void am_dlist_push_front(struct am_dlist *list, struct am_dlist_item *item) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    am_dlist_push_after(&list->sentinel, item);
}

struct am_dlist_item *am_dlist_pop_front(struct am_dlist *list) {
    if (am_dlist_is_empty(list)) {
        return NULL;
    }
    struct am_dlist_item *ret = list->sentinel.next;
    am_dlist_pop(ret);
    return ret;
}

struct am_dlist_item *am_dlist_pop_back(struct am_dlist *list) {
    if (am_dlist_is_empty(list)) {
        return NULL;
    }
    struct am_dlist_item *ret = list->sentinel.prev;
    am_dlist_pop(ret);
    return ret;
}

void am_dlist_push_back(struct am_dlist *list, struct am_dlist_item *item) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    am_dlist_push_before(&list->sentinel, item);
}

struct am_dlist_item *am_dlist_peek_front(struct am_dlist *list) {
    AM_ASSERT(list);
    return am_dlist_is_empty(list) ? NULL : list->sentinel.next;
}

struct am_dlist_item *am_dlist_peek_back(struct am_dlist *list) {
    AM_ASSERT(list);
    return am_dlist_is_empty(list) ? NULL : list->sentinel.prev;
}

bool am_dlist_is_empty(const struct am_dlist *list) {
    AM_ASSERT(list);
    return list->sentinel.next == &list->sentinel;
}

bool am_dlist_item_is_linked(const struct am_dlist_item *item) {
    AM_ASSERT(item);
    return item->next && item->prev;
}
