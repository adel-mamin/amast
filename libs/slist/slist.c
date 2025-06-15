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
 * Singly linked list implementation.
 */

#include <stddef.h>
#include <stdbool.h>

#include "common/macros.h"
#include "slist/slist.h"

void am_slist_ctor(struct am_slist *list) {
    AM_ASSERT(list);
    list->sentinel.next = &list->sentinel;
    list->back = &list->sentinel;
}

bool am_slist_is_empty(const struct am_slist *list) {
    AM_ASSERT(list);
    return list->sentinel.next == &list->sentinel;
}

bool am_slist_item_is_linked(const struct am_slist_item *item) {
    AM_ASSERT(item);
    return item->next != NULL;
}

void am_slist_item_ctor(struct am_slist_item *item) {
    AM_ASSERT(item);
    item->next = NULL;
}

void am_slist_push_after(
    struct am_slist *list,
    struct am_slist_item *item,
    struct am_slist_item *newitem
) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(newitem);

    newitem->next = item->next;
    item->next = newitem;
    if (list->back == item) {
        list->back = newitem;
    }
}

struct am_slist_item *am_slist_pop_after(
    struct am_slist *list, struct am_slist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    AM_ASSERT(item->next);
    AM_ASSERT(item->next->next);

    struct am_slist_item *pop = item->next;
    if (pop == &list->sentinel) {
        return NULL;
    }
    if (list->back == pop) {
        list->back = item;
    }
    item->next = pop->next;

    return pop;
}

struct am_slist_item *am_slist_next_item(
    const struct am_slist *list, const struct am_slist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);
    return (item->next == &list->sentinel) ? NULL : item->next;
}

struct am_slist_item *am_slist_find(
    const struct am_slist *list, am_slist_item_found_fn is_found, void *context
) {
    AM_ASSERT(list);
    AM_ASSERT(is_found);

    struct am_slist_item *item = list->sentinel.next;
    while ((item != &list->sentinel) && !is_found(context, item)) {
        item = item->next;
    }
    return (item == &list->sentinel) ? NULL : item;
}

struct am_slist_item *am_slist_peek_front(const struct am_slist *list) {
    AM_ASSERT(list);
    return am_slist_is_empty(list) ? NULL : list->sentinel.next;
}

struct am_slist_item *am_slist_peek_back(const struct am_slist *list) {
    AM_ASSERT(list);
    return am_slist_is_empty(list) ? NULL : list->back;
}

bool am_slist_owns(
    const struct am_slist *list, const struct am_slist_item *item
) {
    AM_ASSERT(list);
    AM_ASSERT(item);

    struct am_slist_item *next = list->sentinel.next;
    while (next != &list->sentinel) {
        if (next == item) {
            return true;
        }
        next = next->next;
    }
    return false;
}

void am_slist_push_front(struct am_slist *list, struct am_slist_item *item) {
    am_slist_push_after(list, &list->sentinel, item);
}

struct am_slist_item *am_slist_pop_front(struct am_slist *list) {
    return am_slist_pop_after(list, &list->sentinel);
}

void am_slist_push_back(struct am_slist *list, struct am_slist_item *item) {
    am_slist_push_after(list, list->back, item);
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

    am_slist_ctor(from);
}

void am_slist_iterator_ctor(
    struct am_slist *list, struct am_slist_iterator *it
) {
    AM_ASSERT(list);
    AM_ASSERT(it);

    it->list = list;
    it->cur = &list->sentinel;
    it->prev = NULL;
}

struct am_slist_item *am_slist_iterator_next(struct am_slist_iterator *it) {
    AM_ASSERT(it);
    AM_ASSERT(it->cur);

    it->prev = it->cur;
    it->cur = it->cur->next;

    return (it->cur == &it->list->sentinel) ? NULL : it->cur;
}

struct am_slist_item *am_slist_iterator_pop(struct am_slist_iterator *it) {
    AM_ASSERT(it->prev);
    AM_ASSERT(it->cur);

    struct am_slist_item *pop = it->cur;
    it->prev->next = pop->next;
    if (it->list->back == pop) {
        it->list->back = it->prev;
    }
    it->cur = it->prev;
    it->prev = NULL;

    pop->next = NULL;

    return pop;
}
