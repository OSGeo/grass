/* Produced by texiweb from libavl.w. */

/* libavl - library for manipulation of binary trees.
   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.
 */

/* Nov 2016, Markus Metz
 * from libavl-2.0.3
 * added safety checks and speed optimizations
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "pavl.h"

/* Creates and returns a new table
   with comparison function |compare| using parameter |param|
   and memory allocator |allocator|.
   Returns |NULL| if memory allocation failed. */
struct pavl_table *pavl_create(pavl_comparison_func *compare,
                               struct libavl_allocator *allocator)
{
    struct pavl_table *tree;

    assert(compare != NULL);

    if (allocator == NULL)
        allocator = &pavl_allocator_default;

    tree = allocator->libavl_malloc(sizeof *tree);
    if (tree == NULL)
        return NULL;

    tree->pavl_root = NULL;
    tree->pavl_compare = compare;
    tree->pavl_alloc = allocator;
    tree->pavl_count = 0;

    return tree;
}

/* Search |tree| for an item matching |item|, and return it if found.
   Otherwise return |NULL|. */
void *pavl_find(const struct pavl_table *tree, const void *item)
{
    const struct pavl_node *p;

    assert(tree != NULL && item != NULL);

    p = tree->pavl_root;
    while (p != NULL) {
        int cmp = tree->pavl_compare(item, p->pavl_data);

        if (cmp == 0)
            return p->pavl_data;

        p = p->pavl_link[cmp > 0];
    }

    return NULL;
}

/* Inserts |item| into |tree| and returns a pointer to |item|'s address.
   If a duplicate item is found in the tree,
   returns a pointer to the duplicate without inserting |item|.
   Returns |NULL| in case of memory allocation failure. */
void **pavl_probe(struct pavl_table *tree, void *item)
{
    struct pavl_node *y; /* Top node to update balance factor, and parent. */
    struct pavl_node *p, *q; /* Iterator, and parent. */
    struct pavl_node *n;     /* Newly inserted node. */
    struct pavl_node *w;     /* New root of rebalanced subtree. */
    int dir;                 /* Direction to descend. */

    assert(tree != NULL && item != NULL);

    y = p = tree->pavl_root;
    q = NULL;
    dir = 0;
    while (p != NULL) {
        int cmp = tree->pavl_compare(item, p->pavl_data);

        if (cmp == 0)
            return &p->pavl_data;

        dir = cmp > 0;

        if (p->pavl_balance != 0)
            y = p;

        q = p, p = p->pavl_link[dir];
    }

    n = tree->pavl_alloc->libavl_malloc(sizeof *p);
    if (n == NULL)
        return NULL;

    tree->pavl_count++;
    n->pavl_link[0] = n->pavl_link[1] = NULL;
    n->pavl_parent = q;
    n->pavl_data = item;
    n->pavl_balance = 0;
    if (q == NULL) {
        tree->pavl_root = n;

        return &n->pavl_data;
    }
    q->pavl_link[dir] = n;

    p = n;
    while (p != y) {
        q = p->pavl_parent;
        /*
           dir = q->pavl_link[0] != p;
           if (dir == 0)
           q->pavl_balance--;
           else
           q->pavl_balance++;
         */
        if (q->pavl_link[0] != p)
            q->pavl_balance++;
        else
            q->pavl_balance--;

        p = q;
    }

    if (y->pavl_balance == -2) {
        struct pavl_node *x = y->pavl_link[0];

        if (x->pavl_balance == -1) {
            w = x;
            y->pavl_link[0] = x->pavl_link[1];
            x->pavl_link[1] = y;
            x->pavl_balance = y->pavl_balance = 0;
            x->pavl_parent = y->pavl_parent;
            y->pavl_parent = x;
            if (y->pavl_link[0] != NULL)
                y->pavl_link[0]->pavl_parent = y;
        }
        else {
            assert(x->pavl_balance == +1);
            w = x->pavl_link[1];
            x->pavl_link[1] = w->pavl_link[0];
            w->pavl_link[0] = x;
            y->pavl_link[0] = w->pavl_link[1];
            w->pavl_link[1] = y;
            if (w->pavl_balance == -1)
                x->pavl_balance = 0, y->pavl_balance = +1;
            else if (w->pavl_balance == 0)
                x->pavl_balance = y->pavl_balance = 0;
            else /* |w->pavl_balance == +1| */
                x->pavl_balance = -1, y->pavl_balance = 0;
            w->pavl_balance = 0;
            w->pavl_parent = y->pavl_parent;
            x->pavl_parent = y->pavl_parent = w;
            if (x->pavl_link[1] != NULL)
                x->pavl_link[1]->pavl_parent = x;
            if (y->pavl_link[0] != NULL)
                y->pavl_link[0]->pavl_parent = y;
        }
    }
    else if (y->pavl_balance == +2) {
        struct pavl_node *x = y->pavl_link[1];

        if (x->pavl_balance == +1) {
            w = x;
            y->pavl_link[1] = x->pavl_link[0];
            x->pavl_link[0] = y;
            x->pavl_balance = y->pavl_balance = 0;
            x->pavl_parent = y->pavl_parent;
            y->pavl_parent = x;
            if (y->pavl_link[1] != NULL)
                y->pavl_link[1]->pavl_parent = y;
        }
        else {
            assert(x->pavl_balance == -1);
            w = x->pavl_link[0];
            x->pavl_link[0] = w->pavl_link[1];
            w->pavl_link[1] = x;
            y->pavl_link[1] = w->pavl_link[0];
            w->pavl_link[0] = y;
            if (w->pavl_balance == +1)
                x->pavl_balance = 0, y->pavl_balance = -1;
            else if (w->pavl_balance == 0)
                x->pavl_balance = y->pavl_balance = 0;
            else /* |w->pavl_balance == -1| */
                x->pavl_balance = +1, y->pavl_balance = 0;
            w->pavl_balance = 0;
            w->pavl_parent = y->pavl_parent;
            x->pavl_parent = y->pavl_parent = w;
            if (x->pavl_link[0] != NULL)
                x->pavl_link[0]->pavl_parent = x;
            if (y->pavl_link[1] != NULL)
                y->pavl_link[1]->pavl_parent = y;
        }
    }
    else
        return &n->pavl_data;

    if (w->pavl_parent != NULL)
        w->pavl_parent->pavl_link[y != w->pavl_parent->pavl_link[0]] = w;
    else
        tree->pavl_root = w;

    return &n->pavl_data;
}

/* Inserts |item| into |table|.
   Returns |NULL| if |item| was successfully inserted
   or if a memory allocation error occurred.
   Otherwise, returns the duplicate item. */
void *pavl_insert(struct pavl_table *table, void *item)
{
    void **p = pavl_probe(table, item);

    return p == NULL || *p == item ? NULL : *p;
}

/* Inserts |item| into |table|, replacing any duplicate item.
   Returns |NULL| if |item| was inserted without replacing a duplicate,
   or if a memory allocation error occurred.
   Otherwise, returns the item that was replaced. */
void *pavl_replace(struct pavl_table *table, void *item)
{
    void **p = pavl_probe(table, item);

    if (p == NULL || *p == item)
        return NULL;
    else {
        void *r = *p;

        *p = item;

        return r;
    }
}

/* Deletes from |tree| and returns an item matching |item|.
   Returns a null pointer if no matching item found. */
void *pavl_delete(struct pavl_table *tree, const void *item)
{
    struct pavl_node *p; /* Traverses tree to find node to delete. */
    struct pavl_node *q; /* Parent of |p|. */
    int dir;             /* Side of |q| on which |p| is linked. */
    int cmp;             /* Result of comparison between |item| and |p|. */

    assert(tree != NULL && item != NULL);

    p = tree->pavl_root;
    dir = 0;
    while (p != NULL) {
        cmp = tree->pavl_compare(item, p->pavl_data);

        if (cmp == 0)
            break;

        dir = cmp > 0;
        p = p->pavl_link[dir];
    }
    if (p == NULL)
        return NULL;

    item = p->pavl_data;

    q = p->pavl_parent;
    if (q == NULL) {
        q = (struct pavl_node *)&tree->pavl_root;
        dir = 0;
    }

    if (p->pavl_link[1] == NULL) {
        q->pavl_link[dir] = p->pavl_link[0];
        if (q->pavl_link[dir] != NULL)
            q->pavl_link[dir]->pavl_parent = p->pavl_parent;
    }
    else {
        struct pavl_node *r = p->pavl_link[1];

        if (r->pavl_link[0] == NULL) {
            r->pavl_link[0] = p->pavl_link[0];
            q->pavl_link[dir] = r;
            r->pavl_parent = p->pavl_parent;
            if (r->pavl_link[0] != NULL)
                r->pavl_link[0]->pavl_parent = r;
            r->pavl_balance = p->pavl_balance;
            q = r;
            dir = 1;
        }
        else {
            struct pavl_node *s = r->pavl_link[0];

            while (s->pavl_link[0] != NULL)
                s = s->pavl_link[0];
            r = s->pavl_parent;
            r->pavl_link[0] = s->pavl_link[1];
            s->pavl_link[0] = p->pavl_link[0];
            s->pavl_link[1] = p->pavl_link[1];
            q->pavl_link[dir] = s;
            if (s->pavl_link[0] != NULL)
                s->pavl_link[0]->pavl_parent = s;
            s->pavl_link[1]->pavl_parent = s;
            s->pavl_parent = p->pavl_parent;
            if (r->pavl_link[0] != NULL)
                r->pavl_link[0]->pavl_parent = r;
            s->pavl_balance = p->pavl_balance;
            q = r;
            dir = 0;
        }
    }
    tree->pavl_alloc->libavl_free(p);

    while (q != (struct pavl_node *)&tree->pavl_root) {
        struct pavl_node *y = q;

        if (y->pavl_parent != NULL)
            q = y->pavl_parent;
        else
            q = (struct pavl_node *)&tree->pavl_root;

        if (dir == 0) {
            dir = q->pavl_link[0] != y;
            y->pavl_balance++;
            if (y->pavl_balance == +1)
                break;
            else if (y->pavl_balance == +2) {
                struct pavl_node *x = y->pavl_link[1];

                if (x->pavl_balance == -1) {
                    struct pavl_node *w;

                    assert(x->pavl_balance == -1);
                    w = x->pavl_link[0];
                    x->pavl_link[0] = w->pavl_link[1];
                    w->pavl_link[1] = x;
                    y->pavl_link[1] = w->pavl_link[0];
                    w->pavl_link[0] = y;
                    if (w->pavl_balance == +1)
                        x->pavl_balance = 0, y->pavl_balance = -1;
                    else if (w->pavl_balance == 0)
                        x->pavl_balance = y->pavl_balance = 0;
                    else /* |w->pavl_balance == -1| */
                        x->pavl_balance = +1, y->pavl_balance = 0;
                    w->pavl_balance = 0;
                    w->pavl_parent = y->pavl_parent;
                    x->pavl_parent = y->pavl_parent = w;
                    if (x->pavl_link[0] != NULL)
                        x->pavl_link[0]->pavl_parent = x;
                    if (y->pavl_link[1] != NULL)
                        y->pavl_link[1]->pavl_parent = y;
                    q->pavl_link[dir] = w;
                }
                else {
                    y->pavl_link[1] = x->pavl_link[0];
                    x->pavl_link[0] = y;
                    x->pavl_parent = y->pavl_parent;
                    y->pavl_parent = x;
                    if (y->pavl_link[1] != NULL)
                        y->pavl_link[1]->pavl_parent = y;
                    q->pavl_link[dir] = x;
                    if (x->pavl_balance == 0) {
                        x->pavl_balance = -1;
                        y->pavl_balance = +1;
                        break;
                    }
                    else {
                        x->pavl_balance = y->pavl_balance = 0;
                        y = x;
                    }
                }
            }
        }
        else {
            dir = q->pavl_link[0] != y;
            y->pavl_balance--;
            if (y->pavl_balance == -1)
                break;
            else if (y->pavl_balance == -2) {
                struct pavl_node *x = y->pavl_link[0];

                if (x->pavl_balance == +1) {
                    struct pavl_node *w;

                    assert(x->pavl_balance == +1);
                    w = x->pavl_link[1];
                    x->pavl_link[1] = w->pavl_link[0];
                    w->pavl_link[0] = x;
                    y->pavl_link[0] = w->pavl_link[1];
                    w->pavl_link[1] = y;
                    if (w->pavl_balance == -1)
                        x->pavl_balance = 0, y->pavl_balance = +1;
                    else if (w->pavl_balance == 0)
                        x->pavl_balance = y->pavl_balance = 0;
                    else /* |w->pavl_balance == +1| */
                        x->pavl_balance = -1, y->pavl_balance = 0;
                    w->pavl_balance = 0;
                    w->pavl_parent = y->pavl_parent;
                    x->pavl_parent = y->pavl_parent = w;
                    if (x->pavl_link[1] != NULL)
                        x->pavl_link[1]->pavl_parent = x;
                    if (y->pavl_link[0] != NULL)
                        y->pavl_link[0]->pavl_parent = y;
                    q->pavl_link[dir] = w;
                }
                else {
                    y->pavl_link[0] = x->pavl_link[1];
                    x->pavl_link[1] = y;
                    x->pavl_parent = y->pavl_parent;
                    y->pavl_parent = x;
                    if (y->pavl_link[0] != NULL)
                        y->pavl_link[0]->pavl_parent = y;
                    q->pavl_link[dir] = x;
                    if (x->pavl_balance == 0) {
                        x->pavl_balance = +1;
                        y->pavl_balance = -1;
                        break;
                    }
                    else {
                        x->pavl_balance = y->pavl_balance = 0;
                        y = x;
                    }
                }
            }
        }
    }

    tree->pavl_count--;
    return (void *)item;
}

/* Initializes |trav| for use with |tree|
   and selects the null node. */
void pavl_t_init(struct pavl_traverser *trav, struct pavl_table *tree)
{
    trav->pavl_table = tree;
    trav->pavl_node = NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the least value,
   or |NULL| if |tree| is empty. */
void *pavl_t_first(struct pavl_traverser *trav, struct pavl_table *tree)
{
    assert(tree != NULL && trav != NULL);

    trav->pavl_table = tree;
    trav->pavl_node = tree->pavl_root;
    if (trav->pavl_node != NULL) {
        while (trav->pavl_node->pavl_link[0] != NULL)
            trav->pavl_node = trav->pavl_node->pavl_link[0];

        return trav->pavl_node->pavl_data;
    }
    else
        return NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the greatest value,
   or |NULL| if |tree| is empty. */
void *pavl_t_last(struct pavl_traverser *trav, struct pavl_table *tree)
{
    assert(tree != NULL && trav != NULL);

    trav->pavl_table = tree;
    trav->pavl_node = tree->pavl_root;
    if (trav->pavl_node != NULL) {
        while (trav->pavl_node->pavl_link[1] != NULL)
            trav->pavl_node = trav->pavl_node->pavl_link[1];

        return trav->pavl_node->pavl_data;
    }
    else
        return NULL;
}

/* Searches for |item| in |tree|.
   If found, initializes |trav| to the item found and returns the item
   as well.
   If there is no matching item, initializes |trav| to the null item
   and returns |NULL|. */
void *pavl_t_find(struct pavl_traverser *trav, struct pavl_table *tree,
                  void *item)
{
    struct pavl_node *p;

    assert(trav != NULL && tree != NULL && item != NULL);

    trav->pavl_table = tree;

    p = tree->pavl_root;
    while (p != NULL) {
        int cmp = tree->pavl_compare(item, p->pavl_data);

        if (cmp == 0) {
            trav->pavl_node = p;

            return p->pavl_data;
        }

        p = p->pavl_link[cmp > 0];
    }

    trav->pavl_node = NULL;

    return NULL;
}

/* Attempts to insert |item| into |tree|.
   If |item| is inserted successfully, it is returned and |trav| is
   initialized to its location.
   If a duplicate is found, it is returned and |trav| is initialized to
   its location.  No replacement of the item occurs.
   If a memory allocation failure occurs, |NULL| is returned and |trav|
   is initialized to the null item. */
void *pavl_t_insert(struct pavl_traverser *trav, struct pavl_table *tree,
                    void *item)
{
    void **p;

    assert(trav != NULL && tree != NULL && item != NULL);

    p = pavl_probe(tree, item);
    if (p != NULL) {
        trav->pavl_table = tree;
        trav->pavl_node =
            ((struct pavl_node *)((char *)p -
                                  offsetof(struct pavl_node, pavl_data)));

        return *p;
    }
    else {
        pavl_t_init(trav, tree);

        return NULL;
    }
}

/* Initializes |trav| to have the same current node as |src|. */
void *pavl_t_copy(struct pavl_traverser *trav, const struct pavl_traverser *src)
{
    assert(trav != NULL && src != NULL);

    trav->pavl_table = src->pavl_table;
    trav->pavl_node = src->pavl_node;

    return trav->pavl_node != NULL ? trav->pavl_node->pavl_data : NULL;
}

/* Returns the next data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
void *pavl_t_next(struct pavl_traverser *trav)
{
    assert(trav != NULL);

    if (trav->pavl_node == NULL)
        return pavl_t_first(trav, trav->pavl_table);
    else if (trav->pavl_node->pavl_link[1] == NULL) {
        struct pavl_node *q, *p; /* Current node and its child. */

        for (p = trav->pavl_node, q = p->pavl_parent;;
             p = q, q = q->pavl_parent)
            if (q == NULL || p == q->pavl_link[0]) {
                trav->pavl_node = q;

                return trav->pavl_node != NULL ? trav->pavl_node->pavl_data
                                               : NULL;
            }
    }
    else {
        trav->pavl_node = trav->pavl_node->pavl_link[1];
        while (trav->pavl_node->pavl_link[0] != NULL)
            trav->pavl_node = trav->pavl_node->pavl_link[0];

        return trav->pavl_node->pavl_data;
    }
}

/* Returns the previous data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
void *pavl_t_prev(struct pavl_traverser *trav)
{
    assert(trav != NULL);

    if (trav->pavl_node == NULL)
        return pavl_t_last(trav, trav->pavl_table);
    else if (trav->pavl_node->pavl_link[0] == NULL) {
        struct pavl_node *q, *p; /* Current node and its child. */

        for (p = trav->pavl_node, q = p->pavl_parent;;
             p = q, q = q->pavl_parent)
            if (q == NULL || p == q->pavl_link[1]) {
                trav->pavl_node = q;

                return trav->pavl_node != NULL ? trav->pavl_node->pavl_data
                                               : NULL;
            }
    }
    else {
        trav->pavl_node = trav->pavl_node->pavl_link[0];
        while (trav->pavl_node->pavl_link[1] != NULL)
            trav->pavl_node = trav->pavl_node->pavl_link[1];

        return trav->pavl_node->pavl_data;
    }
}

/* Returns |trav|'s current item. */
void *pavl_t_cur(struct pavl_traverser *trav)
{
    assert(trav != NULL);

    return trav->pavl_node != NULL ? trav->pavl_node->pavl_data : NULL;
}

/* Replaces the current item in |trav| by |new| and returns the item replaced.
   |trav| must not have the null item selected.
   The new item must not upset the ordering of the tree. */
void *pavl_t_replace(struct pavl_traverser *trav, void *new)
{
    void *old;

    assert(trav != NULL && trav->pavl_node != NULL && new != NULL);
    old = trav->pavl_node->pavl_data;
    trav->pavl_node->pavl_data = new;

    return old;
}

/* Destroys |new| with |pavl_destroy (new, destroy)|,
   first initializing right links in |new| that have
   not yet been initialized at time of call. */
static void copy_error_recovery(struct pavl_node *q, struct pavl_table *new,
                                pavl_item_func *destroy)
{
    assert(q != NULL && new != NULL);

    for (;;) {
        struct pavl_node *p = q;

        q = q->pavl_parent;
        if (q == NULL)
            break;

        if (p == q->pavl_link[0])
            q->pavl_link[1] = NULL;
    }

    pavl_destroy(new, destroy);
}

/* Copies |org| to a newly created tree, which is returned.
   If |copy != NULL|, each data item in |org| is first passed to |copy|,
   and the return values are inserted into the tree;
   |NULL| return values are taken as indications of failure.
   On failure, destroys the partially created new tree,
   applying |destroy|, if non-null, to each item in the new tree so far,
   and returns |NULL|.
   If |allocator != NULL|, it is used for allocation in the new tree.
   Otherwise, the same allocator used for |org| is used. */
struct pavl_table *pavl_copy(const struct pavl_table *org, pavl_copy_func *copy,
                             pavl_item_func *destroy,
                             struct libavl_allocator *allocator)
{
    struct pavl_table *new;
    const struct pavl_node *x;
    struct pavl_node *y;

    assert(org != NULL);
    new = pavl_create(org->pavl_compare,
                      allocator != NULL ? allocator : org->pavl_alloc);
    if (new == NULL)
        return NULL;

    new->pavl_count = org->pavl_count;
    if (new->pavl_count == 0)
        return new;

    x = (const struct pavl_node *)&org->pavl_root;
    y = (struct pavl_node *)&new->pavl_root;
    while (x != NULL) {
        while (x->pavl_link[0] != NULL) {
            y->pavl_link[0] =
                new->pavl_alloc->libavl_malloc(sizeof *y->pavl_link[0]);
            if (y->pavl_link[0] == NULL) {
                if (y != (struct pavl_node *)&new->pavl_root) {
                    y->pavl_data = NULL;
                    y->pavl_link[1] = NULL;
                }

                copy_error_recovery(y, new, destroy);

                return NULL;
            }
            y->pavl_link[0]->pavl_parent = y;

            x = x->pavl_link[0];
            y = y->pavl_link[0];
        }
        y->pavl_link[0] = NULL;

        for (;;) {
            y->pavl_balance = x->pavl_balance;
            if (copy == NULL)
                y->pavl_data = x->pavl_data;
            else {
                y->pavl_data = copy(x->pavl_data);
                if (y->pavl_data == NULL) {
                    y->pavl_link[1] = NULL;
                    copy_error_recovery(y, new, destroy);

                    return NULL;
                }
            }

            if (x->pavl_link[1] != NULL) {
                y->pavl_link[1] =
                    new->pavl_alloc->libavl_malloc(sizeof *y->pavl_link[1]);
                if (y->pavl_link[1] == NULL) {
                    copy_error_recovery(y, new, destroy);

                    return NULL;
                }
                y->pavl_link[1]->pavl_parent = y;

                x = x->pavl_link[1];
                y = y->pavl_link[1];
                break;
            }
            else
                y->pavl_link[1] = NULL;

            for (;;) {
                const struct pavl_node *w = x;

                x = x->pavl_parent;
                if (x == NULL) {
                    new->pavl_root->pavl_parent = NULL;
                    return new;
                }
                y = y->pavl_parent;

                if (w == x->pavl_link[0])
                    break;
            }
        }
    }

    return new;
}

/* Frees storage allocated for |tree|.
   If |destroy != NULL|, applies it to each data item in inorder. */
void pavl_destroy(struct pavl_table *tree, pavl_item_func *destroy)
{
    struct pavl_node *p, *q;

    assert(tree != NULL);

    p = tree->pavl_root;
    while (p != NULL) {
        if (p->pavl_link[0] == NULL) {
            q = p->pavl_link[1];
            if (destroy != NULL && p->pavl_data != NULL)
                destroy(p->pavl_data);
            tree->pavl_alloc->libavl_free(p);
        }
        else {
            q = p->pavl_link[0];
            p->pavl_link[0] = q->pavl_link[1];
            q->pavl_link[1] = p;
        }
        p = q;
    }

    tree->pavl_alloc->libavl_free(tree);
}

/* Allocates |size| bytes of space using |malloc()|.
   Returns a null pointer if allocation fails. */
void *pavl_malloc(size_t size)
{
    if (size > 0)
        return malloc(size);

    return NULL;
}

/* Frees |block|. */
void pavl_free(void *block)
{
    if (block)
        free(block);
}

/* Default memory allocator that uses |malloc()| and |free()|. */
struct libavl_allocator pavl_allocator_default = {pavl_malloc, pavl_free};

#undef NDEBUG
#include <assert.h>

/* Asserts that |pavl_insert()| succeeds at inserting |item| into |table|. */
void(pavl_assert_insert)(struct pavl_table *table, void *item)
{
    void **p = pavl_probe(table, item);

    assert(p != NULL && *p == item);
}

/* Asserts that |pavl_delete()| really removes |item| from |table|,
   and returns the removed item. */
void *(pavl_assert_delete)(struct pavl_table *table, void *item)
{
    void *p = pavl_delete(table, item);

    assert(p != NULL);

    return p;
}
