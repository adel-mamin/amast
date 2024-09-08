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
 * Doubly linked list interface.
 */

#ifndef DLIST_H_INCLUDED
#define DLIST_H_INCLUDED

#include <stdbool.h>

#if defined __cplusplus
extern "C" {
#endif

/**
 * List iterator traverse direction.
 * Used together with list iterator API
 * a1dlist_iterator_init() to specify the direction
 * in which doubly linked list iterator traverses the list.
 */
enum a1dlist_direction {
    A1DLIST_FORWARD, /**< forward list traverse */
    A1DLIST_BACKWARD /**< backward list traverse */
};

/**
 * Doubly linked item header.
 * There are at least two ways to make an arbitrary structure `struct foo`
 * a doubly linked list item:
 *
 * [1]
 * struct foo {int bar; struct a1dlist_item list;}
 *
 * [2]
 * struct foo {int bar; }
 * struct foo_item {struct foo foo; struct a1dlist_item list}
 *
 * Also `struct foo` can be part of several independent lists.
 * For example,
 *
 * struct foo {int bar; struct a1dlist_item list1; struct a1dlist_item list2;}
 */
struct a1dlist_item {
    /** the next item in the list */
    struct a1dlist_item *next;
    /** the previous item in the list */
    struct a1dlist_item *prev;
};

/** Doubly linked list handler */
struct a1dlist {
    /** Keeps pointers to the beginning and end of the list */
    struct a1dlist_item sentinel;
};

/** Doubly linked list iterator handler */
struct a1dlist_iterator {
    struct a1dlist *hnd;        /**< list handler */
    struct a1dlist_item *cur;   /**< current item of the list */
    enum a1dlist_direction dir; /**< direction of traverse */
};

/**
 * The list initialization.
 * The handler memory is provided by caller.
 * @param hnd  the list handler
 */
void a1dlist_init(struct a1dlist *hnd);

/**
 * Tell if list is empty.
 * @param hnd     the list handler
 * @retval true   the list is empty
 * @retval false  the list is not empty
 */
bool a1dlist_is_empty(const struct a1dlist *hnd);

/**
 * Check if the given list item is part of ANY list.
 * @param[in] item  the list item to check.
 * @retval true     item IS part of SOME list
 * @retval false    item IS NOT part of ANY list
 */
bool a1dlist_item_is_linked(const struct a1dlist_item *item);

/**
 * Initialize list item.
 * @param item  list item to initialize.
 */
void a1dlist_item_init(struct a1dlist_item *item);

/**
 * Give next item after the given one.
 * Can be used to iterate a list. However if item pop operation is expected
 * during the iteration, then list iterator APIs are to be used instead.
 * @param hnd   the list handler
 * @param item  the item next to this one is returned.
 * @return The next item or NULL if \a item is the last one in the list
 */
struct a1dlist_item *a1dlist_next(
    const struct a1dlist *hnd, const struct a1dlist_item *item
);

/**
 * Give previous item for the given one.
 * Can be used to iterate a list. However if item pop operation is expected
 * during the iteration, then list iterator APIs are to be used instead.
 * @param hnd   the list handler
 * @param item  the item previous to this one is returned
 * @return The previous item or NULL if \a item is the first one in the list
 */
struct a1dlist_item *a1dlist_prev(
    const struct a1dlist *hnd, const struct a1dlist_item *item
);

/**
 * Return the number of items in the list.
 * @param hnd  the list handler
 * @return The number of items in the list.
 */
int a1dlist_size(const struct a1dlist *hnd);

/**
 * Push a new item before the item, which is already in the list.
 * @param item  the new item is pushed before this item
 * @param new_item the new item to be pushed to the list
 */
void a1dlist_push_before(
    struct a1dlist_item *item, struct a1dlist_item *new_item
);

/**
 * Push a new item after the item, which is already in the list.
 * @param item  the new item is pushed after this item
 * @param new_item the new item to be pushed to the list
 */
void a1dlist_push_after(
    struct a1dlist_item *item, struct a1dlist_item *new_item
);

/**
 * Add a new item at the front (head) of the list.
 * @param hnd   the list handler
 * @param item  the item to be added
 */
void a1dlist_push_front(struct a1dlist *hnd, struct a1dlist_item *item);

/**
 * Add a new item at the back (tail) of the list.
 * @param hnd   the list handler
 * @param item  the item to be added
 */
void a1dlist_push_back(struct a1dlist *hnd, struct a1dlist_item *item);

/**
 * Pop the given item from the list.
 * The provided item must be part of the list.
 * Otherwise the behavior is undefined.
 * @param item    the item to pop
 * @retval true   the item was popped
 * @retval false  the item was not popped as it was not linked
 */
bool a1dlist_pop(struct a1dlist_item *item);

/**
 * Pop an item in front (head) of the list.
 * @param hnd  the list handler
 * @return the popped item or NULL if the list was empty
 */
struct a1dlist_item *a1dlist_pop_front(struct a1dlist *hnd);

/**
 * Pop an item from the back (tail) of the list.
 * @param hnd  the list handler
 * @return the popped item or NULL if the list was empty
 */
struct a1dlist_item *a1dlist_pop_back(struct a1dlist *hnd);

/**
 * Return the list item at the front (head) of the list.
 * The item is not removed from the list.
 * @param hnd  the list handler
 * @return The pointer to the front (head) item or NULL if the list is empty
 */
struct a1dlist_item *a1dlist_peek_front(struct a1dlist *hnd);

/**
 * Return the list item at the back (tail) of the list.
 * The item is not removed from the list.
 * @param hnd  the list handler
 * @return The pointer to the back (tail) item or NULL if the list is empty
 */
struct a1dlist_item *a1dlist_peek_back(struct a1dlist *hnd);

/**
 * Predicate callback type that tells if item is found.
 * @param context  the predicate context
 * @param item     item to analyze
 * @retval true    item was found
 * @retval false   item was not found
 */
typedef bool (*a1dlist_item_found_cb_t)(
    void *context, struct a1dlist_item *item
);

/**
 * Find an item in the list using the predicate function.
 * @param hnd          the list handler
 * @param is_found_cb  the predicate function, which tells if the item
 *                     is found. If the predicate returns 1, the item is
 *                     found. If it returns 0, the item is not found.
 * @param context      the context, which is provided verbatim to predicate
 * @return The item, found by the predicate callback function.
 *         NULL, if nothing was found. The found item is not popped
 *         from the list.
 */
struct a1dlist_item *a1dlist_find(
    const struct a1dlist *hnd,
    a1dlist_item_found_cb_t is_found_cb,
    void *context
);

/**
 * Initialize a new iterator.
 * Must be called before calling a1dlist_iterator_next().
 * If the iterator is used to traverse the list once, then
 * it must be re-initialized by calling this function in order to
 * be used with a1dlist_iterator_next() again.
 * The only valid operation with the iterator after this one is
 * a1dlist_iterator_next(). Otherwise the behavior is undefined.
 * @param hnd  the list handler
 * @param it   the iterator to be initialized
 * @param dir  the direction, at which the iteration is going to be done,
 *             when a1dlist_iterator_next() is used.
 */
void a1dlist_iterator_init(
    struct a1dlist *hnd,
    struct a1dlist_iterator *it,
    enum a1dlist_direction direction
);

/**
 * Iterate over the list in the predefined direction.
 * Is supposed to be called in iterative way to traverse part or full list.
 * The iteration can visit each item only once. When all items of
 * the list were visited, the next invocation of the function returns NULL.
 * The current visited item can only be popped with
 * a1dlist_iterator_pop().
 * @param it the iterator initialized by a1dlist_iterator_init()
 * @return The visited item. The item is not popped from the list.
 */
struct a1dlist_item *a1dlist_iterator_next(struct a1dlist_iterator *it);

/**
 * Pop the item pointed by the iterator from the list.
 * The popped item is returned. The iterator is still usable after the removal.
 * At least one a1dlist_iterator_next() is expected for the iterator before
 * this function is called. Otherwise the behavior is undefined. The only valid
 * operation possible after this call is a1dlist_iterator_next(); Otherwise the
 * behavior is undefined.
 * @param it The iterator.
 * @retval The popped item.
 */
struct a1dlist_item *a1dlist_iterator_pop(struct a1dlist_iterator *it);

/**
 * Check if the given item is part of the list.
 * @param hnd     the list handler
 * @param item    the item to be checked
 * @retval true   the given item belongs to the list
 * @retval false  the given item is not part of the list
 */
bool a1dlist_owns(const struct a1dlist *hnd, const struct a1dlist_item *item);

#if defined __cplusplus
}
#endif

#endif /* DLIST_H_INCLUDED */
