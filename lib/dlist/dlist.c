/*
 * The MIT License (MIT)
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
 * Doubly linked list implementation.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "dlist/dlist.h"

void dlist_init(struct dlist *p) {
    p->sentinel.next = p->sentinel.prev = &p->sentinel;
}

void dlist_item_init(struct dlist_item *p) { p->next = p->prev = NULL; }

struct dlist_item *dlist_next(
    const struct dlist *p, const struct dlist_item *item
) {
    ASSERT(p);
    ASSERT(item);
    if (&p->sentinel == item->next) {
        return NULL;
    }
    return item->next;
}

struct dlist_item *dlist_prev(
    const struct dlist *p, const struct dlist_item *item
) {
    ASSERT(p);
    ASSERT(item);
    if (&p->sentinel == item->prev) {
        return NULL;
    }
    return item->prev;
}

void dlist_push_after(struct dlist_item *item, struct dlist_item *new_item) {
    ASSERT(item);
    ASSERT(item->next);
    ASSERT(new_item);

    new_item->next = item->next;
    new_item->prev = item;
    item->next->prev = new_item;
    item->next = new_item;
}

void dlist_push_before(struct dlist_item *item, struct dlist_item *new_item) {
    ASSERT(item);
    ASSERT(item->prev);
    ASSERT(new_item);

    new_item->next = item;
    new_item->prev = item->prev;
    item->prev->next = new_item;
    item->prev = new_item;
}

void dlist_iterator_init(
    struct dlist *h, struct dlist_iterator *it, enum dlist_direction direction
) {
    it->p = h;
    it->cur = &h->sentinel;
    it->dir = direction;
}

struct dlist_item *dlist_iterator_next(struct dlist_iterator *it) {
    ASSERT(it);
    ASSERT(it->p);
    ASSERT(it->cur);

    if (dlist_forward == it->dir) {
        it->cur = it->cur->next;
    } else {
        it->cur = it->cur->prev;
    }

    if (it->cur == &it->p->sentinel) {
        it->cur = NULL;
        return NULL;
    }

    ASSERT(NULL != it->cur);

    return it->cur;
}

struct dlist_item *dlist_iterator_pop(struct dlist_iterator *it) {
    ASSERT(it->cur != &it->p->sentinel);
    struct dlist_item *pop = it->cur;
    it->cur = it->cur->prev;
    dlist_pop(pop);
    return pop;
}

void dlist_pop(struct dlist_item *item) {
    ASSERT(item);

    if ((item)->next) {
        (item)->next->prev = (item)->prev;
    }
    if ((item)->prev) {
        (item)->prev->next = (item)->next;
    }
    (item)->next = (item)->prev = NULL;
}

struct dlist_item *dlist_find(
    const struct dlist *p,
    int (*is_found)(void *context, struct dlist_item *item),
    void *context
) {
    ASSERT(p);
    ASSERT(is_found);

    struct dlist_item *candidate = p->sentinel.next;
    while ((candidate != &p->sentinel) && (!is_found(context, candidate))) {
        candidate = candidate->next;
    }
    if (candidate == &p->sentinel) {
        return NULL;
    }
    return candidate;
}

int dlist_size(const struct dlist *p) {
    ASSERT(p);

    if (dlist_is_empty(p)) {
        return 0;
    }

    int size = 0;
    struct dlist_item *item = p->sentinel.next;
    while ((item != &p->sentinel) && (item != NULL)) {
        size++;
        item = item->next;
    }
    ASSERT(item != NULL);

    return size;
}

bool dlist_owns(const struct dlist *p, const struct dlist_item *item) {
    ASSERT(p);
    ASSERT(item);

    struct dlist_item *next = p->sentinel.next;
    while (next != &p->sentinel) {
        if (item == next) {
            return true;
        }
        next = next->next;
    }

    return false;
}

void dlist_push_front(struct dlist *p, struct dlist_item *item) {
    ASSERT(p);
    ASSERT(item);
    dlist_push_after(&p->sentinel, item);
}

struct dlist_item *dlist_pop_front(struct dlist *p) {
    if (dlist_is_empty(p)) {
        return NULL;
    }
    struct dlist_item *ret = p->sentinel.next;
    dlist_pop(p->sentinel.next);
    return ret;
}

struct dlist_item *dlist_pop_back(struct dlist *p) {
    if (dlist_is_empty(p)) {
        return NULL;
    }
    struct dlist_item *ret = p->sentinel.prev;
    dlist_pop(p->sentinel.prev);
    return ret;
}

void dlist_push_back(struct dlist *p, struct dlist_item *item) {
    ASSERT(p);
    ASSERT(item);
    dlist_push_before(&p->sentinel, item);
}

struct dlist_item *dlist_peek_front(struct dlist *p) {
    ASSERT(p);
    return dlist_is_empty(p) ? NULL : p->sentinel.next;
}

struct dlist_item *dlist_peek_back(struct dlist *p) {
    ASSERT(p);
    return dlist_is_empty(p) ? NULL : p->sentinel.prev;
}

bool dlist_is_empty(const struct dlist *p) {
    ASSERT(p);
    return p->sentinel.next == &p->sentinel;
}

bool dlist_item_is_linked(const struct dlist_item *item) {
    ASSERT(item);
    return (NULL != item->next) && (NULL != item->prev);
}
