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
 * Singly linked list interface.
 */

#ifndef AM_SLIST_H_INCLUDED
#define AM_SLIST_H_INCLUDED

#include <stdbool.h>

#if defined __cplusplus
extern "C" {
#endif

extern const int am_alignof_slist;
extern const int am_alignof_slist_item;

#define AM_ALIGNOF_SLIST am_alignof_slist
#define AM_ALIGNOF_SLIST_ITEM am_alignof_slist_item

/**
 * Singly linked item.
 * There are at least two ways to make an arbitrary structure `struct foo`
 * a singly linked list item:
 *
 * [1]
 * struct foo {int bar; struct am_slist_item list;}
 *
 * [2]
 * struct foo {int bar; }
 * struct foo_item {struct foo foo; struct am_slist_item list}
 *
 * Also `struct foo` can be part of several independent lists.
 * For example,
 *
 * struct foo {int bar; struct am_slist_item list1; struct am_slist_item list2;}
 */
struct am_slist_item {
    struct am_slist_item *next; /**< the next item in the list */
};

/** Singly linked list handler */
struct am_slist {
    struct am_slist_item sentinel; /**< the beginning of the list */
    struct am_slist_item *back;    /**< the end of the list */
};

/** Singly linked list iterator handler */
struct am_slist_iterator {
    struct am_slist *hnd;      /**< list handler */
    struct am_slist_item *cur; /**< current item of the list */
};

/**
 * Singly linked list initialization.
 * @param hnd  the list handler
 */
void am_slist_init(struct am_slist *hnd);

/**
 * Tell if list is empty.
 * @param hnd     the list handler
 * @retval true   the list is empty
 * @retval false  the list is not empty
 */
bool am_slist_is_empty(const struct am_slist *hnd);

/**
 * Push a new item after the item, which is already in the list.
 * @param hnd      the list handler
 * @param item     the new item is pushed after this item
 * @param newitem  the new item to be pushed to the list
 */
void am_slist_push_after(
    struct am_slist *hnd,
    struct am_slist_item *item,
    struct am_slist_item *newitem
);

/**
 * Pop the item after the given item.
 * The provided item must be part of the list.
 * Otherwise the behavior is undefined.
 * @param hnd   the list handler
 * @param item  the item after this item is popped
 * @return The popped item or NULL if nothing to remove
 */
struct am_slist_item *am_slist_pop_after(
    struct am_slist *hnd, struct am_slist_item *item
);

/**
 * Return next item.
 * @param hnd   the list handler
 * @param item  the next item after this one is returned
 * @return the next item or NULL if the \a item is the last item
 */
struct am_slist_item *am_slist_next(
    const struct am_slist *hnd, const struct am_slist_item *item
);

/**
 * Predicate callback type that tells if item is found.
 * @param context  the predicate context
 * @param item     item to analyze
 * @retval true    item was found
 * @retval false   item was not found
 */
typedef bool (*am_slist_item_found_cb_t)(
    void *context, struct am_slist_item *item
);

/**
 * Find an item in the list using the predicate function.
 * @param hnd          the list handler
 * @param is_found_cb  the predicate callback
 * @param context      the context, which is provided verbatim to predicate
 * @return The item, found by the predicate callback function.
 *         NULL, if nothing was found. The found item is not popped
 *         from the list.
 */
struct am_slist_item *am_slist_find(
    const struct am_slist *hnd,
    am_slist_item_found_cb_t is_found_cb,
    void *context
);

/**
 * Return the list item at the front (head) of the list.
 * The list item is not popped from the list.
 * @param hnd  the list handler
 * @return The item at the front of the list or NULL, if no
 *         item exists at the given index.
 */
struct am_slist_item *am_slist_peek_front(const struct am_slist *hnd);

/**
 * Return the list item at the back (tail) of the list.
 * The lists item is not popped from the list.
 * @param hnd  the list handler
 * @return The item at the back of the list or NULL, if no
 *         item exists at the given index.
 */
struct am_slist_item *am_slist_peek_back(const struct am_slist *hnd);

/**
 * Push an item to the front (head) of the list.
 * @param hnd   the list handler
 * @param item  the item to be pushed
 */
void am_slist_push_front(struct am_slist *hnd, struct am_slist_item *item);

/**
 * Pop an item in front (head) of the list.
 * @param hnd  the list handler
 * @return the popped item or NULL, if the list was empty
 */
struct am_slist_item *am_slist_pop_front(struct am_slist *hnd);

/**
 * Add a new item at the back (tail) of the list.
 * @param hnd   the list handler
 * @param item  the item to be added
 */
void am_slist_push_back(struct am_slist *hnd, struct am_slist_item *item);

/**
 * Check if the given item is part of the list.
 * @param hnd     the list handler
 * @param item    the item to be checked
 * @retval true   the given item belongs to the list
 * @retval false  the given item is not part of the list
 */
bool am_slist_owns(
    const struct am_slist *hnd, const struct am_slist_item *item
);

/**
 * Get next list item.
 * @param hnd     the list handler
 * @param item    the item to be checked
 * @return Next list item or NULL
 */
struct am_slist_item *am_slist_next_item(
    const struct am_slist *hnd, const struct am_slist_item *item
);

#if defined __cplusplus
}
#endif

#endif /* AM_SLIST_H_INCLUDED */
