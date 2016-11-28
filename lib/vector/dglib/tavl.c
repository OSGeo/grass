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
#include "tavl.h"

/* Creates and returns a new table
   with comparison function |compare| using parameter |param|
   and memory allocator |allocator|.
   Returns |NULL| if memory allocation failed. */
struct tavl_table *tavl_create(tavl_comparison_func * compare, void *param,
			       struct libavl_allocator *allocator)
{
    struct tavl_table *tree;

    assert(compare != NULL);

    if (allocator == NULL)
	allocator = &tavl_allocator_default;

    tree = allocator->libavl_malloc(allocator, sizeof *tree);
    if (tree == NULL)
	return NULL;

    tree->tavl_root = NULL;
    tree->tavl_compare = compare;
    tree->tavl_param = param;
    tree->tavl_alloc = allocator;
    tree->tavl_count = 0;

    return tree;
}

/* Search |tree| for an item matching |item|, and return it if found.
   Otherwise return |NULL|. */
void *tavl_find(const struct tavl_table *tree, const void *item)
{
    const struct tavl_node *p;

    assert(tree != NULL && item != NULL);

    p = tree->tavl_root;
    while (p != NULL) {
	int cmp, dir;

	cmp = tree->tavl_compare(item, p->tavl_data, tree->tavl_param);
	if (cmp == 0)
	    return p->tavl_data;

	dir = cmp > 0;
	if (p->tavl_tag[dir] == TAVL_CHILD)
	    p = p->tavl_link[dir];
	else
	    p = NULL;
    }

    return NULL;
}

/* Inserts |item| into |tree| and returns a pointer to |item|'s address.
   If a duplicate item is found in the tree,
   returns a pointer to the duplicate without inserting |item|.
   Returns |NULL| in case of memory allocation failure. */
void **tavl_probe(struct tavl_table *tree, void *item)
{
    struct tavl_node *y, *z;	/* Top node to update balance factor, and parent. */
    struct tavl_node *p, *q;	/* Iterator, and parent. */
    struct tavl_node *n;	/* Newly inserted node. */
    struct tavl_node *w;	/* New root of rebalanced subtree. */
    int dir;			/* Direction to descend. */

    unsigned char da[TAVL_MAX_HEIGHT];	/* Cached comparison results. */
    int k = 0;			/* Number of cached results. */

    assert(tree != NULL && item != NULL);

    z = (struct tavl_node *)&tree->tavl_root;
    y = tree->tavl_root;
    dir = 0;
    q = z, p = y;
    while (p != NULL) {
	int cmp =
	    tree->tavl_compare(item, p->tavl_data, tree->tavl_param);
	if (cmp == 0)
	    return &p->tavl_data;

	if (p->tavl_balance != 0)
	    z = q, y = p, k = 0;
	da[k++] = dir = cmp > 0;

	if (p->tavl_tag[dir] == TAVL_THREAD)
	    break;
	q = p, p = p->tavl_link[dir];
    }

    n = tree->tavl_alloc->libavl_malloc(tree->tavl_alloc, sizeof *n);
    if (n == NULL)
	return NULL;

    tree->tavl_count++;
    n->tavl_data = item;
    n->tavl_tag[0] = n->tavl_tag[1] = TAVL_THREAD;
    n->tavl_balance = 0;
    if (y == NULL) {
	n->tavl_link[0] = n->tavl_link[1] = NULL;
	tree->tavl_root = n;

	return &n->tavl_data;
    }
    n->tavl_link[dir] = p->tavl_link[dir];
    n->tavl_link[!dir] = p;
    p->tavl_tag[dir] = TAVL_CHILD;
    p->tavl_link[dir] = n;

    p = y, k = 0;
    while (p != n) {
	if (da[k] == 0)
	    p->tavl_balance--;
	else
	    p->tavl_balance++;
	p = p->tavl_link[da[k]], k++;
    }

    if (y->tavl_balance == -2) {
	struct tavl_node *x = y->tavl_link[0];

	if (x->tavl_balance == -1) {
	    w = x;
	    if (x->tavl_tag[1] == TAVL_THREAD) {
		x->tavl_tag[1] = TAVL_CHILD;
		y->tavl_tag[0] = TAVL_THREAD;
		y->tavl_link[0] = x;
	    }
	    else
		y->tavl_link[0] = x->tavl_link[1];
	    x->tavl_link[1] = y;
	    x->tavl_balance = y->tavl_balance = 0;
	}
	else {
	    assert(x->tavl_balance == +1);
	    w = x->tavl_link[1];
	    x->tavl_link[1] = w->tavl_link[0];
	    w->tavl_link[0] = x;
	    y->tavl_link[0] = w->tavl_link[1];
	    w->tavl_link[1] = y;
	    if (w->tavl_balance == -1)
		x->tavl_balance = 0, y->tavl_balance = +1;
	    else if (w->tavl_balance == 0)
		x->tavl_balance = y->tavl_balance = 0;
	    else		/* |w->tavl_balance == +1| */
		x->tavl_balance = -1, y->tavl_balance = 0;
	    w->tavl_balance = 0;
	    if (w->tavl_tag[0] == TAVL_THREAD) {
		x->tavl_tag[1] = TAVL_THREAD;
		x->tavl_link[1] = w;
		w->tavl_tag[0] = TAVL_CHILD;
	    }
	    if (w->tavl_tag[1] == TAVL_THREAD) {
		y->tavl_tag[0] = TAVL_THREAD;
		y->tavl_link[0] = w;
		w->tavl_tag[1] = TAVL_CHILD;
	    }
	}
    }
    else if (y->tavl_balance == +2) {
	struct tavl_node *x = y->tavl_link[1];

	if (x->tavl_balance == +1) {
	    w = x;
	    if (x->tavl_tag[0] == TAVL_THREAD) {
		x->tavl_tag[0] = TAVL_CHILD;
		y->tavl_tag[1] = TAVL_THREAD;
		y->tavl_link[1] = x;
	    }
	    else
		y->tavl_link[1] = x->tavl_link[0];
	    x->tavl_link[0] = y;
	    x->tavl_balance = y->tavl_balance = 0;
	}
	else {
	    assert(x->tavl_balance == -1);
	    w = x->tavl_link[0];
	    x->tavl_link[0] = w->tavl_link[1];
	    w->tavl_link[1] = x;
	    y->tavl_link[1] = w->tavl_link[0];
	    w->tavl_link[0] = y;
	    if (w->tavl_balance == +1)
		x->tavl_balance = 0, y->tavl_balance = -1;
	    else if (w->tavl_balance == 0)
		x->tavl_balance = y->tavl_balance = 0;
	    else		/* |w->tavl_balance == -1| */
		x->tavl_balance = +1, y->tavl_balance = 0;
	    w->tavl_balance = 0;
	    if (w->tavl_tag[0] == TAVL_THREAD) {
		y->tavl_tag[1] = TAVL_THREAD;
		y->tavl_link[1] = w;
		w->tavl_tag[0] = TAVL_CHILD;
	    }
	    if (w->tavl_tag[1] == TAVL_THREAD) {
		x->tavl_tag[0] = TAVL_THREAD;
		x->tavl_link[0] = w;
		w->tavl_tag[1] = TAVL_CHILD;
	    }
	}
    }
    else
	return &n->tavl_data;
    z->tavl_link[y != z->tavl_link[0]] = w;

    return &n->tavl_data;
}

/* Inserts |item| into |table|.
   Returns |NULL| if |item| was successfully inserted
   or if a memory allocation error occurred.
   Otherwise, returns the duplicate item. */
void *tavl_insert(struct tavl_table *table, void *item)
{
    void **p = tavl_probe(table, item);

    return p == NULL || *p == item ? NULL : *p;
}

/* Inserts |item| into |table|, replacing any duplicate item.
   Returns |NULL| if |item| was inserted without replacing a duplicate,
   or if a memory allocation error occurred.
   Otherwise, returns the item that was replaced. */
void *tavl_replace(struct tavl_table *table, void *item)
{
    void **p = tavl_probe(table, item);

    if (p == NULL || *p == item)
	return NULL;
    else {
	void *r = *p;

	*p = item;

	return r;
    }
}

/* Returns the parent of |node| within |tree|,
   or a pointer to |tavl_root| if |s| is the root of the tree. */
static struct tavl_node *find_parent(struct tavl_table *tree,
				     struct tavl_node *node)
{
    if (node != tree->tavl_root) {
	struct tavl_node *x, *y;

	for (x = y = node;; x = x->tavl_link[0], y = y->tavl_link[1])
	    if (y->tavl_tag[1] == TAVL_THREAD) {
		struct tavl_node *p = y->tavl_link[1];

		if (p == NULL || p->tavl_link[0] != node) {
		    while (x->tavl_tag[0] == TAVL_CHILD)
			x = x->tavl_link[0];
		    p = x->tavl_link[0];
		}
		return p;
	    }
	    else if (x->tavl_tag[0] == TAVL_THREAD) {
		struct tavl_node *p = x->tavl_link[0];

		if (p == NULL || p->tavl_link[1] != node) {
		    while (y->tavl_tag[1] == TAVL_CHILD)
			y = y->tavl_link[1];
		    p = y->tavl_link[1];
		}
		return p;
	    }
    }
    else
	return (struct tavl_node *)&tree->tavl_root;
}

/* Deletes from |tree| and returns an item matching |item|.
   Returns a null pointer if no matching item found. */
void *tavl_delete(struct tavl_table *tree, const void *item)
{
    struct tavl_node *p;	/* Traverses tree to find node to delete. */
    struct tavl_node *q;	/* Parent of |p|. */
    int dir;			/* Index into |q->tavl_link[]| to get |p|. */
    int cmp;			/* Result of comparison between |item| and |p|. */

    assert(tree != NULL && item != NULL);

    q = (struct tavl_node *)&tree->tavl_root;
    p = tree->tavl_root;
    dir = 0;
    while (p != NULL) {
	cmp = tree->tavl_compare(item, p->tavl_data, tree->tavl_param);

	if (cmp == 0)
	    break;

	dir = cmp > 0;

	q = p;
	if (p->tavl_tag[dir] == TAVL_CHILD)
	    p = p->tavl_link[dir];
	else
	    p = NULL;
    }
    if (p == NULL)
	return NULL;

    item = p->tavl_data;

    if (p->tavl_tag[1] == TAVL_THREAD) {
	if (p->tavl_tag[0] == TAVL_CHILD) {
	    struct tavl_node *t = p->tavl_link[0];

	    while (t->tavl_tag[1] == TAVL_CHILD)
		t = t->tavl_link[1];
	    t->tavl_link[1] = p->tavl_link[1];
	    q->tavl_link[dir] = p->tavl_link[0];
	}
	else {
	    q->tavl_link[dir] = p->tavl_link[dir];
	    if (q != (struct tavl_node *)&tree->tavl_root)
		q->tavl_tag[dir] = TAVL_THREAD;
	}
    }
    else {
	struct tavl_node *r = p->tavl_link[1];

	if (r->tavl_tag[0] == TAVL_THREAD) {
	    r->tavl_link[0] = p->tavl_link[0];
	    r->tavl_tag[0] = p->tavl_tag[0];
	    if (r->tavl_tag[0] == TAVL_CHILD) {
		struct tavl_node *t = r->tavl_link[0];

		while (t->tavl_tag[1] == TAVL_CHILD)
		    t = t->tavl_link[1];
		t->tavl_link[1] = r;
	    }
	    q->tavl_link[dir] = r;
	    r->tavl_balance = p->tavl_balance;
	    q = r;
	    dir = 1;
	}
	else {
	    struct tavl_node *s;

	    while (r != NULL) {
		s = r->tavl_link[0];
		if (s->tavl_tag[0] == TAVL_THREAD)
		    break;

		r = s;
	    }

	    if (s->tavl_tag[1] == TAVL_CHILD)
		r->tavl_link[0] = s->tavl_link[1];
	    else {
		r->tavl_link[0] = s;
		r->tavl_tag[0] = TAVL_THREAD;
	    }

	    s->tavl_link[0] = p->tavl_link[0];
	    if (p->tavl_tag[0] == TAVL_CHILD) {
		struct tavl_node *t = p->tavl_link[0];

		while (t->tavl_tag[1] == TAVL_CHILD)
		    t = t->tavl_link[1];
		t->tavl_link[1] = s;

		s->tavl_tag[0] = TAVL_CHILD;
	    }

	    s->tavl_link[1] = p->tavl_link[1];
	    s->tavl_tag[1] = TAVL_CHILD;

	    q->tavl_link[dir] = s;
	    s->tavl_balance = p->tavl_balance;
	    q = r;
	    dir = 0;
	}
    }

    tree->tavl_alloc->libavl_free(tree->tavl_alloc, p);

    while (q != (struct tavl_node *)&tree->tavl_root) {
	struct tavl_node *y = q;

	q = find_parent(tree, y);

	if (dir == 0) {
	    dir = q->tavl_link[0] != y;
	    y->tavl_balance++;
	    if (y->tavl_balance == +1)
		break;
	    else if (y->tavl_balance == +2) {
		struct tavl_node *x = y->tavl_link[1];

		assert(x != NULL);
		if (x->tavl_balance == -1) {
		    struct tavl_node *w;

		    assert(x->tavl_balance == -1);
		    w = x->tavl_link[0];
		    x->tavl_link[0] = w->tavl_link[1];
		    w->tavl_link[1] = x;
		    y->tavl_link[1] = w->tavl_link[0];
		    w->tavl_link[0] = y;
		    if (w->tavl_balance == +1)
			x->tavl_balance = 0, y->tavl_balance = -1;
		    else if (w->tavl_balance == 0)
			x->tavl_balance = y->tavl_balance = 0;
		    else	/* |w->tavl_balance == -1| */
			x->tavl_balance = +1, y->tavl_balance = 0;
		    w->tavl_balance = 0;
		    if (w->tavl_tag[0] == TAVL_THREAD) {
			y->tavl_tag[1] = TAVL_THREAD;
			y->tavl_link[1] = w;
			w->tavl_tag[0] = TAVL_CHILD;
		    }
		    if (w->tavl_tag[1] == TAVL_THREAD) {
			x->tavl_tag[0] = TAVL_THREAD;
			x->tavl_link[0] = w;
			w->tavl_tag[1] = TAVL_CHILD;
		    }
		    q->tavl_link[dir] = w;
		}
		else {
		    q->tavl_link[dir] = x;

		    if (x->tavl_balance == 0) {
			y->tavl_link[1] = x->tavl_link[0];
			x->tavl_link[0] = y;
			x->tavl_balance = -1;
			y->tavl_balance = +1;
			break;
		    }
		    else {	/* |x->tavl_balance == +1| */

			if (x->tavl_tag[0] == TAVL_CHILD)
			    y->tavl_link[1] = x->tavl_link[0];
			else {
			    y->tavl_tag[1] = TAVL_THREAD;
			    x->tavl_tag[0] = TAVL_CHILD;
			}
			x->tavl_link[0] = y;
			y->tavl_balance = x->tavl_balance = 0;
		    }
		}
	    }
	}
	else {
	    dir = q->tavl_link[0] != y;
	    y->tavl_balance--;
	    if (y->tavl_balance == -1)
		break;
	    else if (y->tavl_balance == -2) {
		struct tavl_node *x = y->tavl_link[0];

		assert(x != NULL);
		if (x->tavl_balance == +1) {
		    struct tavl_node *w;

		    assert(x->tavl_balance == +1);
		    w = x->tavl_link[1];
		    x->tavl_link[1] = w->tavl_link[0];
		    w->tavl_link[0] = x;
		    y->tavl_link[0] = w->tavl_link[1];
		    w->tavl_link[1] = y;
		    if (w->tavl_balance == -1)
			x->tavl_balance = 0, y->tavl_balance = +1;
		    else if (w->tavl_balance == 0)
			x->tavl_balance = y->tavl_balance = 0;
		    else	/* |w->tavl_balance == +1| */
			x->tavl_balance = -1, y->tavl_balance = 0;
		    w->tavl_balance = 0;
		    if (w->tavl_tag[0] == TAVL_THREAD) {
			x->tavl_tag[1] = TAVL_THREAD;
			x->tavl_link[1] = w;
			w->tavl_tag[0] = TAVL_CHILD;
		    }
		    if (w->tavl_tag[1] == TAVL_THREAD) {
			y->tavl_tag[0] = TAVL_THREAD;
			y->tavl_link[0] = w;
			w->tavl_tag[1] = TAVL_CHILD;
		    }
		    q->tavl_link[dir] = w;
		}
		else {
		    q->tavl_link[dir] = x;

		    if (x->tavl_balance == 0) {
			y->tavl_link[0] = x->tavl_link[1];
			x->tavl_link[1] = y;
			x->tavl_balance = +1;
			y->tavl_balance = -1;
			break;
		    }
		    else {	/* |x->tavl_balance == -1| */

			if (x->tavl_tag[1] == TAVL_CHILD)
			    y->tavl_link[0] = x->tavl_link[1];
			else {
			    y->tavl_tag[0] = TAVL_THREAD;
			    x->tavl_tag[1] = TAVL_CHILD;
			}
			x->tavl_link[1] = y;
			y->tavl_balance = x->tavl_balance = 0;
		    }
		}
	    }
	}
    }

    tree->tavl_count--;

    return (void *)item;
}

/* Initializes |trav| for use with |tree|
   and selects the null node. */
void tavl_t_init(struct tavl_traverser *trav, struct tavl_table *tree)
{
    trav->tavl_table = tree;
    trav->tavl_node = NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the least value,
   or |NULL| if |tree| is empty. */
void *tavl_t_first(struct tavl_traverser *trav, struct tavl_table *tree)
{
    assert(tree != NULL && trav != NULL);

    trav->tavl_table = tree;
    trav->tavl_node = tree->tavl_root;
    if (trav->tavl_node != NULL) {
	while (trav->tavl_node->tavl_tag[0] == TAVL_CHILD)
	    trav->tavl_node = trav->tavl_node->tavl_link[0];
	return trav->tavl_node->tavl_data;
    }
    else
	return NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the greatest value,
   or |NULL| if |tree| is empty. */
void *tavl_t_last(struct tavl_traverser *trav, struct tavl_table *tree)
{
    assert(tree != NULL && trav != NULL);

    trav->tavl_table = tree;
    trav->tavl_node = tree->tavl_root;
    if (trav->tavl_node != NULL) {
	while (trav->tavl_node->tavl_tag[1] == TAVL_CHILD)
	    trav->tavl_node = trav->tavl_node->tavl_link[1];
	return trav->tavl_node->tavl_data;
    }
    else
	return NULL;
}

/* Searches for |item| in |tree|.
   If found, initializes |trav| to the item found and returns the item
   as well.
   If there is no matching item, initializes |trav| to the null item
   and returns |NULL|. */
void *tavl_t_find(struct tavl_traverser *trav, struct tavl_table *tree,
		  void *item)
{
    struct tavl_node *p;

    assert(trav != NULL && tree != NULL && item != NULL);

    trav->tavl_table = tree;
    trav->tavl_node = NULL;

    p = tree->tavl_root;
    while (p != NULL) {
	int cmp, dir;

	cmp = tree->tavl_compare(item, p->tavl_data, tree->tavl_param);
	if (cmp == 0) {
	    trav->tavl_node = p;

	    return p->tavl_data;
	}

	dir = cmp > 0;
	if (p->tavl_tag[dir] == TAVL_CHILD)
	    p = p->tavl_link[dir];
	else
	    p = NULL;
    }

    trav->tavl_node = NULL;

    return NULL;
}

/* Attempts to insert |item| into |tree|.
   If |item| is inserted successfully, it is returned and |trav| is
   initialized to its location.
   If a duplicate is found, it is returned and |trav| is initialized to
   its location.  No replacement of the item occurs.
   If a memory allocation failure occurs, |NULL| is returned and |trav|
   is initialized to the null item. */
void *tavl_t_insert(struct tavl_traverser *trav,
		    struct tavl_table *tree, void *item)
{
    void **p;

    assert(trav != NULL && tree != NULL && item != NULL);

    p = tavl_probe(tree, item);
    if (p != NULL) {
	trav->tavl_table = tree;
	trav->tavl_node = ((struct tavl_node *)
			   ((char *)p -
			    offsetof(struct tavl_node, tavl_data)));
	return *p;
    }
    else {
	tavl_t_init(trav, tree);

	return NULL;
    }
}

/* Initializes |trav| to have the same current node as |src|. */
void *tavl_t_copy(struct tavl_traverser *trav,
		  const struct tavl_traverser *src)
{
    assert(trav != NULL && src != NULL);

    trav->tavl_table = src->tavl_table;
    trav->tavl_node = src->tavl_node;

    return trav->tavl_node != NULL ? trav->tavl_node->tavl_data : NULL;
}

/* Returns the next data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
void *tavl_t_next(struct tavl_traverser *trav)
{
    assert(trav != NULL);

    if (trav->tavl_node == NULL)
	return tavl_t_first(trav, trav->tavl_table);
    else if (trav->tavl_node->tavl_tag[1] == TAVL_THREAD) {
	trav->tavl_node = trav->tavl_node->tavl_link[1];
	return trav->tavl_node != NULL ? trav->tavl_node->tavl_data : NULL;
    }
    else {
	trav->tavl_node = trav->tavl_node->tavl_link[1];
	while (trav->tavl_node->tavl_tag[0] == TAVL_CHILD)
	    trav->tavl_node = trav->tavl_node->tavl_link[0];
	return trav->tavl_node->tavl_data;
    }
}

/* Returns the previous data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
void *tavl_t_prev(struct tavl_traverser *trav)
{
    assert(trav != NULL);

    if (trav->tavl_node == NULL)
	return tavl_t_last(trav, trav->tavl_table);
    else if (trav->tavl_node->tavl_tag[0] == TAVL_THREAD) {
	trav->tavl_node = trav->tavl_node->tavl_link[0];
	return trav->tavl_node != NULL ? trav->tavl_node->tavl_data : NULL;
    }
    else {
	trav->tavl_node = trav->tavl_node->tavl_link[0];
	while (trav->tavl_node->tavl_tag[1] == TAVL_CHILD)
	    trav->tavl_node = trav->tavl_node->tavl_link[1];
	return trav->tavl_node->tavl_data;
    }
}

/* Returns |trav|'s current item. */
void *tavl_t_cur(struct tavl_traverser *trav)
{
    assert(trav != NULL);

    return trav->tavl_node != NULL ? trav->tavl_node->tavl_data : NULL;
}

/* Replaces the current item in |trav| by |new| and returns the item replaced.
   |trav| must not have the null item selected.
   The new item must not upset the ordering of the tree. */
void *tavl_t_replace(struct tavl_traverser *trav, void *new)
{
    void *old;

    assert(trav != NULL && trav->tavl_node != NULL && new != NULL);
    old = trav->tavl_node->tavl_data;
    trav->tavl_node->tavl_data = new;

    return old;
}

/* Creates a new node as a child of |dst| on side |dir|.
   Copies data and |tavl_balance| from |src| into the new node,
   applying |copy()|, if non-null.
   Returns nonzero only if fully successful.
   Regardless of success, integrity of the tree structure is assured,
   though failure may leave a null pointer in a |tavl_data| member. */
static int
copy_node(struct tavl_table *tree,
	  struct tavl_node *dst, int dir,
	  const struct tavl_node *src, tavl_copy_func * copy)
{
    struct tavl_node *new =
	tree->tavl_alloc->libavl_malloc(tree->tavl_alloc, sizeof *new);
    if (new == NULL)
	return 0;

    new->tavl_link[dir] = dst->tavl_link[dir];
    new->tavl_tag[dir] = TAVL_THREAD;
    new->tavl_link[!dir] = dst;
    new->tavl_tag[!dir] = TAVL_THREAD;
    dst->tavl_link[dir] = new;
    dst->tavl_tag[dir] = TAVL_CHILD;

    new->tavl_balance = src->tavl_balance;
    if (copy == NULL)
	new->tavl_data = src->tavl_data;
    else {
	new->tavl_data = copy(src->tavl_data, tree->tavl_param);
	if (new->tavl_data == NULL)
	    return 0;
    }

    return 1;
}

/* Destroys |new| with |tavl_destroy (new, destroy)|,
   first initializing the right link in |new| that has
   not yet been initialized. */
static void
copy_error_recovery(struct tavl_node *p,
		    struct tavl_table *new, tavl_item_func * destroy)
{
    new->tavl_root = p;
    if (p != NULL) {
	while (p->tavl_tag[1] == TAVL_CHILD)
	    p = p->tavl_link[1];
	p->tavl_link[1] = NULL;
    }
    tavl_destroy(new, destroy);
}

/* Copies |org| to a newly created tree, which is returned.
   If |copy != NULL|, each data item in |org| is first passed to |copy|,
   and the return values are inserted into the tree,
   with |NULL| return values taken as indications of failure.
   On failure, destroys the partially created new tree,
   applying |destroy|, if non-null, to each item in the new tree so far,
   and returns |NULL|.
   If |allocator != NULL|, it is used for allocation in the new tree.
   Otherwise, the same allocator used for |org| is used. */
struct tavl_table *tavl_copy(const struct tavl_table *org,
			     tavl_copy_func * copy, tavl_item_func * destroy,
			     struct libavl_allocator *allocator)
{
    struct tavl_table *new;

    const struct tavl_node *p;
    struct tavl_node *q;
    struct tavl_node rp, rq;

    assert(org != NULL);
    new = tavl_create(org->tavl_compare, org->tavl_param,
		      allocator != NULL ? allocator : org->tavl_alloc);
    if (new == NULL)
	return NULL;

    new->tavl_count = org->tavl_count;
    if (new->tavl_count == 0)
	return new;

    p = &rp;
    rp.tavl_link[0] = org->tavl_root;
    rp.tavl_tag[0] = TAVL_CHILD;

    q = &rq;
    rq.tavl_link[0] = NULL;
    rq.tavl_tag[0] = TAVL_THREAD;

    while (p != NULL) {
	if (p->tavl_tag[0] == TAVL_CHILD) {
	    if (!copy_node(new, q, 0, p->tavl_link[0], copy)) {
		copy_error_recovery(rq.tavl_link[0], new, destroy);
		return NULL;
	    }

	    p = p->tavl_link[0];
	    q = q->tavl_link[0];
	}
	else {
	    while (p->tavl_tag[1] == TAVL_THREAD) {
		p = p->tavl_link[1];
		if (p == NULL) {
		    q->tavl_link[1] = NULL;
		    new->tavl_root = rq.tavl_link[0];
		    return new;
		}

		q = q->tavl_link[1];
	    }

	    p = p->tavl_link[1];
	    q = q->tavl_link[1];
	}

	if (p->tavl_tag[1] == TAVL_CHILD)
	    if (!copy_node(new, q, 1, p->tavl_link[1], copy)) {
		copy_error_recovery(rq.tavl_link[0], new, destroy);
		return NULL;
	    }
    }

    return new;
}

/* Frees storage allocated for |tree|.
   If |destroy != NULL|, applies it to each data item in inorder. */
void tavl_destroy(struct tavl_table *tree, tavl_item_func * destroy)
{
    struct tavl_node *p;	/* Current node. */
    struct tavl_node *n;	/* Next node. */

    p = tree->tavl_root;
    if (p != NULL) {
	while (p->tavl_tag[0] == TAVL_CHILD)
	    p = p->tavl_link[0];
    }

    while (p != NULL) {
	n = p->tavl_link[1];
	if (p->tavl_tag[1] == TAVL_CHILD)
	    while (n->tavl_tag[0] == TAVL_CHILD)
		n = n->tavl_link[0];

	if (destroy != NULL && p->tavl_data != NULL)
	    destroy(p->tavl_data, tree->tavl_param);
	tree->tavl_alloc->libavl_free(tree->tavl_alloc, p);

	p = n;
    }

    tree->tavl_alloc->libavl_free(tree->tavl_alloc, tree);
}

/* Allocates |size| bytes of space using |malloc()|.
   Returns a null pointer if allocation fails. */
void *tavl_malloc(struct libavl_allocator *allocator, size_t size)
{
    assert(allocator != NULL && size > 0);
    return malloc(size);
}

/* Frees |block|. */
void tavl_free(struct libavl_allocator *allocator, void *block)
{
    assert(allocator != NULL && block != NULL);
    free(block);
}

/* Default memory allocator that uses |malloc()| and |free()|. */
struct libavl_allocator tavl_allocator_default = {
    tavl_malloc,
    tavl_free
};

#undef NDEBUG
#include <assert.h>

/* Asserts that |tavl_insert()| succeeds at inserting |item| into |table|. */
void (tavl_assert_insert) (struct tavl_table * table, void *item)
{
    void **p = tavl_probe(table, item);

    assert(p != NULL && *p == item);
}

/* Asserts that |tavl_delete()| really removes |item| from |table|,
   and returns the removed item. */
void *(tavl_assert_delete) (struct tavl_table * table, void *item)
{
    void *p = tavl_delete(table, item);

    assert(p != NULL);

    return p;
}
