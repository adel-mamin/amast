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
 * dlist_iterator_init() to specify the direction
 * in which doubly linked list iterator traverses the list.
 */
enum dlist_direction {
    dlist_forward, /**< Forward list traverse */
    dlist_backward /**< Backward list traverse */
};

/**
 * Doubly linked item header.
 * There are at least two ways to make an arbitrary structure `struct Foo`
 * a doubly linked list item:
 *
 * [1]
 * struct Foo {int bar; struct dlist_item li;}
 *
 * [2]
 * struct Foo {int bar; }
 * struct FooItem {Foo_t foo; struct dlist_item li}
 *
 * Also `struct Foo` can be part of several independent lists.
 * For example,
 *
 * struct Foo {int bar; struct dlist_item li1; struct dlist_item li2;}
 */
struct dlist_item {
    /** Pointer to the next item in the list */
    struct dlist_item *next;
    /** Pointer to the previous item in the list */
    struct dlist_item *prev;
};

/** Doubly linked list handler */
struct dlist {
    /** Keeps pointers to the beginning and end of the list */
    struct dlist_item sentinel;
};

/** Doubly linked list iterator handler */
struct dlist_iterator {
    struct dlist *p;          /**< Doubly linked list handler pointer */
    struct dlist_item *cur;   /**< Current element of the list */
    enum dlist_direction dir; /**< Direction of traverse */
};

/**
 * Tells if list is empty.
 * @param p the list handler pointer.
 * @retval true the list is empty
 * @retval false the list is not empty
 */
bool dlist_is_empty(const struct dlist *p);

/**
 * Checks if the given list item is part of ANY list.
 * @param[in] item the list item to check.
 * @retval true item IS part of SOME list
 * @retval false item IS NOT part of ANY list
 */
bool dlist_item_is_linked(const struct dlist_item *item);

/**
 * The list initialization.
 * The handler memory is provided by caller.
 * @param p the list handler pointer
 */
void dlist_init(struct dlist *p);

/**
 * Initialize list item.
 * @param p list item to initialize.
 */
void dlist_item_init(struct dlist_item *p);

/**
 * Gives next item after the given one.
 * Can be used to iterate a list. However if item pop operation is expected
 * during the iteration, then list iterator APIs are to be used instead.
 * @param p the list handler pointer
 * @param item the item next to this one is returned.
 * @return The next item or NULL if \a item is the last one in the list
 */
struct dlist_item *dlist_next(
    const struct dlist *p, const struct dlist_item *item
);

/**
 * Gives previous item for the given one.
 * Can be used to iterate a list. However if item pop operation is expected
 * during the iteration, then list iterator APIs are to be used instead.
 * @param p the list handler pointer
 * @param item the item previous to this one is returned.
 * @return The previous item or NULL if \a item is the first one in the list
 */
struct dlist_item *dlist_prev(
    const struct dlist *p, const struct dlist_item *item
);

/**
 * Returns the number of elements in the list.
 * @param p the list handler pointer
 * @return The number of elements in the list.
 */
int dlist_size(const struct dlist *p);

/**
 * Pushes a new item before the item, which is already in the list.
 * @param item the new item is pushed before this item
 * @param new_item the new item to be pushed to the list
 */
void dlist_push_before(struct dlist_item *item, struct dlist_item *new_item);

/**
 * Pushes a new item after the item, which is already in the list.
 * @param item the new item is pushed after this item
 * @param new_item the new item to be pushed to the list
 */
void dlist_push_after(struct dlist_item *item, struct dlist_item *new_item);

/**
 * Adds a new item at the front (head) of the list.
 * @param p the list handler pointer
 * @param item the item to be added
 */
void dlist_push_front(struct dlist *p, struct dlist_item *item);

/**
 * Adds a new element at the back (tail) of the list.
 * @param p the list handler pointer
 * @param item the item to be added
 */
void dlist_push_back(struct dlist *p, struct dlist_item *item);

/**
 * Pops the given element from the list.
 * The provided element must be part of the list.
 * Otherwise the behavior is undefined.
 * @param item the element to pop
 */
void dlist_pop(struct dlist_item *item);

/**
 * Pops an element in front (head) of the list.
 * @param p the list handler pointer
 * @return the popped element or NULL if the list was empty
 */
struct dlist_item *dlist_pop_front(struct dlist *p);

/**
 * Pops an item from the back (tail) of the list.
 * @param p the list handler pointer
 * @return the popped item or NULL if the list was empty
 */
struct dlist_item *dlist_pop_back(struct dlist *p);

/**
 * Returns the list element at the front (head) of the list.
 * The element is not removed from the list.
 * @param p the list handler pointer
 * @return The pointer to the front (head) element or NULL if the list is empty
 */
struct dlist_item *dlist_peek_front(struct dlist *p);

/**
 * Returns the list element at the back (tail) of the list.
 * The element is not removed from the list.
 * @param p the list handler pointer
 * @return The pointer to the back (tail) element or NULL if the list is empty
 */
struct dlist_item *dlist_peek_back(struct dlist *p);

/**
 * Finds an element in the list using the predicate function.
 * @param p the list handler pointer
 * @param is_found the predicate function, which tells if the element
 *                  is found. If the predicate returns 1, the element is
 *                  found. If it returns 0, the element is not found.
 * @param context the context, which is provided verbatim to predicate
 * @return The element, found by the predicate callback function.
 *         NULL, if nothing was found. The found element is not popped
 *         from the list.
 */
struct dlist_item *dlist_find(
    const struct dlist *p,
    int (*is_found)(void *context, struct dlist_item *item),
    void *context
);

/**
 * Initializes a new iterator.
 * Must be called before calling dlist_iterator_next().
 * If the iterator is used to traverse the list once, then
 * it must be re-initialized by calling this function in order to
 * be used with dlist_iterator_next() again.
 * The only valid operation with the iterator after this one is
 * dlist_iterator_next(). Otherwise the behavior is undefined.
 * @param h the list handler pointer
 * @param it the iterator to be initialized
 * @param dir the direction, at which the iteration is going to be done,
 *            when dlist_iterator_next() is used.
 */
void dlist_iterator_init(
    struct dlist *h, struct dlist_iterator *it, enum dlist_direction direction
);

/**
 * Iterates over the list in the predefined direction.
 * Is supposed to be called in iterative way to traverse part or full list.
 * The iteration can visit each element only once. When all elements of
 * the list were visited, the next invocation of the function returns NULL.
 * The current visited element can only be popped with
 * dlist_iterator_pop().
 * @param it the iterator initialized by dlist_iterator_init()
 * @return The visited element. The element is not popped from the list.
 */
struct dlist_item *dlist_iterator_next(struct dlist_iterator *it);

/**
 * Pops the element pointed by the iterator from the list.
 * The popped item is returned. The iterator is still usable after the removal.
 * At least one dlist_iterator_next() is expected for the iterator before
 * this function is called. Otherwise the behavior is undefined. The only valid
 * operation possible after this call is dlist_iterator_next(); Otherwise the
 * behavior is undefined.
 * @param it The iterator.
 * @retval The popped item.
 */
struct dlist_item *dlist_iterator_pop(struct dlist_iterator *it);

/**
 * Checks if the given element is part of the list.
 * @param p the list handler pointer
 * @param item the element to be checked.
 * @retval true the given element belongs to the list
 * @retval false the given element is not part of the list
 */
bool dlist_owns(const struct dlist *p, const struct dlist_item *item);

#if defined __cplusplus
}
#endif

#endif /* DLIST_H_INCLUDED */
