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
 * Singly linked list interface.
 */

#ifndef AM_SLIST_H_INCLUDED
#define AM_SLIST_H_INCLUDED

#include <stdbool.h>

/**
 * Singly linked item.
 *
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
    struct am_slist_item *next; /**< next item in the list */
};

/** Singly linked list handler */
struct am_slist {
    struct am_slist_item sentinel; /**< beginning of the list */
    struct am_slist_item *back;    /**< end of the list */
};

/** Singly linked list iterator handler */
struct am_slist_iterator {
    struct am_slist *list;      /**< the list */
    struct am_slist_item *cur;  /**< current item of the list */
    struct am_slist_item *prev; /**< previous item of the list */
};

#ifdef __cplusplus
extern "C" {
#endif

extern const int am_alignof_slist;
extern const int am_alignof_slist_item;

#define AM_ALIGNOF_SLIST am_alignof_slist
#define AM_ALIGNOF_SLIST_ITEM am_alignof_slist_item

/**
 * Singly linked list initialization.
 *
 * @param list  the list
 */
void am_slist_init(struct am_slist *list);

/**
 * Check if list is empty.
 *
 * @param list  the list
 *
 * @retval true   the list is empty
 * @retval false  the list is not empty
 */
bool am_slist_is_empty(const struct am_slist *list);

/**
 * Check if given list item is part of ANY list.
 *
 * @param[in] item  the list item to check
 *
 * @retval true   item IS part of SOME list
 * @retval false  item IS NOT part of ANY list
 */
bool am_slist_item_is_linked(const struct am_slist_item *item);

/**
 * Initialize list item.
 *
 * @param item  list item to initialize.
 */
void am_slist_item_init(struct am_slist_item *item);

/**
 * Push new item after the item, which is already in list.
 *
 * @param list     the list
 * @param item     the new item is pushed after this item
 * @param newitem  the new item to be pushed to the list
 */
void am_slist_push_after(
    struct am_slist *list,
    struct am_slist_item *item,
    struct am_slist_item *newitem
);

/**
 * Pop item after given item.
 *
 * The provided item must be part of the list.
 * Otherwise the behavior is undefined.
 *
 * @param list  the list
 * @param item  the item after this item is popped
 *
 * @return the popped item or NULL if nothing to remove
 */
struct am_slist_item *am_slist_pop_after(
    struct am_slist *list, struct am_slist_item *item
);

/**
 * Predicate callback type that tells if item is found.
 *
 * @param context  the predicate context
 * @param item     item to analyze
 *
 * @retval true    item was found
 * @retval false   item was not found
 */
typedef bool (*am_slist_item_found_fn)(
    void *context, struct am_slist_item *item
);

/**
 * Find item in list using predicate function.
 *
 * @param list      the list
 * @param is_found  the predicate callback
 * @param context   the context, which is provided verbatim to predicate
 *
 * @return the item, found by the predicate callback function
 *         NULL, if nothing was found. The found item is not popped
 *         from the list.
 */
struct am_slist_item *am_slist_find(
    const struct am_slist *list, am_slist_item_found_fn is_found, void *context
);

/**
 * Return list item at front (head) of list.
 *
 * The list item is not popped from the list.
 *
 * @param list  the list
 *
 * @return the item at the front of the list or NULL, if no
 *         item exists at the given index
 */
struct am_slist_item *am_slist_peek_front(const struct am_slist *list);

/**
 * Return list item at back (tail) of list.
 *
 * The lists item is not popped from the list.
 *
 * @param list  the list
 *
 * @return the item at the back of the list or NULL, if no
 *         item exists at the given index.
 */
struct am_slist_item *am_slist_peek_back(const struct am_slist *list);

/**
 * Push item to front (head) of list.
 *
 * @param list  the list
 * @param item  the item to be pushed
 */
void am_slist_push_front(struct am_slist *list, struct am_slist_item *item);

/**
 * Pop item in front (head) of list.
 *
 * @param list  the list
 *
 * @return the popped item or NULL, if the list was empty
 */
struct am_slist_item *am_slist_pop_front(struct am_slist *list);

/**
 * Add new item at back (tail) of list.
 *
 * @param list  the list
 * @param item  the item to be added
 */
void am_slist_push_back(struct am_slist *list, struct am_slist_item *item);

/**
 * Check if given item is part of list.
 *
 * @param list  the list
 * @param item  the item to be checked
 *
 * @retval true   the given item belongs to the list
 * @retval false  the given item is not part of the list
 */
bool am_slist_owns(
    const struct am_slist *list, const struct am_slist_item *item
);

/**
 * Get next list item.
 *
 * @param list  the list
 * @param item  the item to be checked
 *
 * @return next list item or NULL
 */
struct am_slist_item *am_slist_next_item(
    const struct am_slist *list, const struct am_slist_item *item
);

/**
 * Append one list to another.
 *
 * @param to    append to this list
 * @param from  append this list
 *              If 'from' list is not empty, then it is initialized after
 *              it is appended to 'to' list.
 */
void am_slist_append(struct am_slist *to, struct am_slist *from);

/**
 * Initialize new iterator.
 *
 * Must be called before calling am_slist_iterator_next().
 * If the iterator is used to traverse the list once, then
 * it must be re-initialized by calling this function in order to
 * be used with am_slist_iterator_next() again.
 * The only valid operation with the iterator after this one is
 * am_slist_iterator_next() or am_slist_iterator_pop().
 * Otherwise the behavior is undefined.
 *
 * @param list  the list
 * @param it    the iterator to be initialized
 */
void am_slist_iterator_init(
    struct am_slist *list, struct am_slist_iterator *it
);

/**
 * Iterate over list.
 *
 * Is supposed to be called in iterative way to traverse part or full list.
 * The iteration can visit each item only once. When all items of
 * the list were visited, the next invocation of the function returns NULL.
 * The current visited item can only be popped with am_slist_iterator_pop().
 *
 * @param it  the iterator initialized by am_slist_iterator_init()
 * @return the visited item or NULL if the iteration is over
 *         The item is not popped from the list.
 */
struct am_slist_item *am_slist_iterator_next(struct am_slist_iterator *it);

/**
 * Pop item pointed by iterator.
 *
 * The popped item is returned. The iterator is still usable after the removal.
 * At least one am_slist_iterator_next() call is expected for the iterator
 * before this function is called. Otherwise the behavior is undefined.
 * The valid operations possible after this call are am_slist_iterator_next()
 * or am_slist_iterator_init(). Otherwise the behavior is undefined.
 *
 * @param it  the iterator
 *
 * @retval the popped item
 */
struct am_slist_item *am_slist_iterator_pop(struct am_slist_iterator *it);

#ifdef __cplusplus
}
#endif

#endif /* AM_SLIST_H_INCLUDED */
