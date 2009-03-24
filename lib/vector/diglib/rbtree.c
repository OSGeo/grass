/****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Markus Metz
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* balanced binary search tree implementation
 * this one is a Red Black Tree, the bare version, no parent pointers, no threads
 * The core code comes from Julienne Walker's tutorials on
 * binary search trees: insert, remove, balance
 * support for any kind of data structures comes from libavl (GPL >= 2)
 *
 * I could have used some off-the-shelf solution, but that's boring
 *
 * Red Black Trees are used to maintain a data structure that allows
 * search, insertion and deletion in O(log N) time
 * This is needed for large vectors with many features
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vect/rbtree.h>

/* internal functions */
void rbtree_destroy2(struct RB_NODE *);
struct RB_NODE *rbtree_single(struct RB_NODE *, int);
struct RB_NODE *rbtree_double(struct RB_NODE *, int);
void *rbtree_first(struct RB_TRAV *);
void *rbtree_next(struct RB_TRAV *);
struct RB_NODE *rbtree_make_node(size_t, void *);
int is_red(struct RB_NODE *);


/* create new tree and initialize
 * return pointer to new tree or NULL for memory allocation error
 */

struct RB_TREE *rbtree_create(rb_compare_fn *compare, size_t rb_datasize)
{
    struct RB_TREE *tree = malloc(sizeof(*tree));

    if (tree == NULL) {
	G_warning("RB Search Tree: Out of memory!");
	return NULL;
    }

    tree->datasize = rb_datasize;
    tree->rb_compare = compare;
    tree->count = 0;
    tree->root = NULL;

    return tree;
} 

/* add an item to a tree
 * returns 1 on success, 0 on failure
 * non-recursive top-down insertion
 * the algorithm does not allow duplicates and also does not warn about a duplicate
 */
int rbtree_insert(struct RB_TREE *tree, void *data)
{
    if (tree->root == NULL) {
	/* create a new root node for tree */
	tree->root = rbtree_make_node(tree->datasize, data);
	if (tree->root == NULL)
	    return 0;
    }
    else {
	struct RB_NODE head = {0}; /* False tree root */

	struct RB_NODE *g, *t;     /* Grandparent & parent */
	struct RB_NODE *p, *q;     /* Iterator & parent */
	int dir = 0, last = 0;

	/* Set up helpers */
	t = &head;
	g = p = NULL;
	q = t->link[1] = tree->root;

	/* Search down the tree */
	for ( ; ; ) {
	    if (q == NULL) {
		/* Insert new node at the bottom */
		p->link[dir] = q = rbtree_make_node(tree->datasize, data);
		if (q == NULL)
		    return 0;
	    }
	    else if (is_red(q->link[0]) && is_red(q->link[1])) {
		/* Color flip */
		q->red = 1;
		q->link[0]->red = 0;
		q->link[1]->red = 0;
	    }

	    /* Fix red violation */
	    if (is_red(q) && is_red(p)) {
		int dir2 = t->link[1] == g;

		if (q == p->link[last])
		    t->link[dir2] = rbtree_single(g, !last);
		else
		    t->link[dir2] = rbtree_double(g, !last);
	    }

	    last = dir;
	    dir = tree->rb_compare(q->data, data);

	    /* Stop if found */
	    if (dir == 2)
		break;

	    /* Update helpers */
	    if (g != NULL)
		t = g;

	    g = p, p = q;
	    q = q->link[dir];
	}

	/* Update root */
	tree->root = head.link[1];
    }

    /* Make root black */
    tree->root->red = 0;

    tree->count++;

    return 1;
}

/* delete an item from a tree
 * returns 1 on successful deletion
 * returns 0 if data item was not found
 * non-recursive top-down deletion
 */
int rbtree_remove(struct RB_TREE *tree, const void *data)
{
    struct RB_NODE head = {0}; /* False tree root */
    struct RB_NODE *q, *p, *g; /* Helpers */
    struct RB_NODE *f = NULL;  /* Found item */
    int dir = 1, found = 0;

    if (tree->root == NULL) {
	return 0; /* empty tree, nothing to remove */
    }

    /* Set up helpers */
    q = &head;
    g = p = NULL;
    q->link[1] = tree->root;

    /* Search and push a red down */
    while (q->link[dir] != NULL) {
	int last = dir;

	/* Update helpers */
	g = p, p = q;
	q = q->link[dir];
	dir = tree->rb_compare(q->data, data);

	/* Save found node */
	if (dir == 2) {
	    f = q;
	    dir = 0;
	}

	/* Push the red node down */
	if (!is_red(q) && !is_red(q->link[dir])) {
	    if (is_red(q->link[!dir]))
		p = p->link[last] = rbtree_single(q, dir);
	    else if (!is_red(q->link[!dir])) {
		struct RB_NODE *s = p->link[!last];

		if (s != NULL) {
		    if (!is_red(s->link[!last]) &&
		        !is_red(s->link[last])) {
			/* Color flip */
			p->red = 0;
			s->red = 1;
			q->red = 1;
		    }
		    else {
			int dir2 = g->link[1] == p;

			if (is_red(s->link[last]))
			    g->link[dir2] = rbtree_double(p, last);
			else if (is_red(s->link[!last]))
			    g->link[dir2] = rbtree_single(p, last);

			/* Ensure correct coloring */
			q->red = g->link[dir2]->red = 1;
			g->link[dir2]->link[0]->red = 0;
			g->link[dir2]->link[1]->red = 0;
		    }
		}
	    }
	}
    }

    /* Replace and remove if found */
    if (f != NULL) {
	p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
	free(q->data);
	free(q);
	tree->count--;
	found = 1;
    }
    else
	G_debug(2, "data not found in search tree");

    /* Update root and make it black */
    tree->root = head.link[1];
    if ( tree->root != NULL)
	tree->root->red = 0;

    return found;
}

/* find data item in tree
 * return pointer to data item if found else NULL
 */
void *rbtree_find(struct RB_TREE *tree, const void *data)
{
    struct RB_NODE *curr_node = tree->root;
    int dir = 0;

    assert(tree && data);

    while (curr_node != NULL) {
	dir = tree->rb_compare(curr_node->data, data);
	if (dir == 2)
	    return curr_node->data;
	else {
	    curr_node = curr_node->link[dir];
	}
    }
    return NULL;
}

/* initialize tree traversal
 * (re-)sets trav structure
 * return pointer to trav struct or NULL on memory allocation error
 */
void rbtree_init_trav(struct RB_TRAV *trav, struct RB_TREE *tree)
{
    int i;

    assert(trav && tree);

    trav->tree = tree;
    trav->curr_node = tree->root;
    trav->first = 1;

    for (i = 0; i < RBTREE_MAX_HEIGHT; i++)
	trav->up[i] = NULL;
}

/* traverse the tree in ascending order
 * useful to get all items in the tree non-recursively
 * return pointer to data
 * struct RB_TRAV *trav needs to be initialized first
 */
void *rbtree_traverse(struct RB_TRAV *trav)
{
    assert(trav);
    if (trav->curr_node == NULL) {
	if (trav->first)
	    G_warning("empty tree");
	else
	    G_warning("finished traversing");

	return NULL;
    }
	
    if (trav->first) {
	trav->first = 0;
	return rbtree_first(trav);
    }
    else
	return rbtree_next(trav);
}

/* two functions needed to fully traverse the tree: initialize and continue
 * useful to get all items in the tree non-recursively
 * this one here uses a stack
 * parent pointers or threads would also be possible
 * but these would need to be added to RB_NODE
 * -> more memory needed for standard operations
 */

/* start traversing the tree */
void *rbtree_first(struct RB_TRAV *trav)
{
    trav->top = 0;

    /* get smallest item */
    if (trav->curr_node != NULL) {
	while (trav->curr_node->link[0] != NULL) {
	    trav->up[trav->top++] = trav->curr_node;
	    trav->curr_node = trav->curr_node->link[0];
	}
    }

    if (trav->curr_node != NULL) {
	return trav->curr_node->data; /* return smallest item */
    }
    else
	return NULL; /* empty tree */
}

/* continue traversing the tree */
void *rbtree_next(struct RB_TRAV *trav)
{
    if (trav->curr_node->link[1] != NULL) {
	/* something on the right side: larger item */
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->link[1];

	/* go down, find smallest item in this branch */
	while (trav->curr_node->link[0] != NULL) {
	    trav->up[trav->top++] = trav->curr_node;
	    trav->curr_node = trav->curr_node->link[0];
	}
    }
    else {
	/* at smallest item in this branch, go back up */
	struct RB_NODE *last;
	do {
	    if (trav->top == 0) {
		trav->curr_node = NULL;
		break;
	    }
	    last = trav->curr_node;
	    trav->curr_node = trav->up[--trav->top];
	} while (last == trav->curr_node->link[1]);
    }

    if (trav->curr_node != NULL) {
	return trav->curr_node->data;
    }
    else	
	return NULL; /* finished traversing */
}

/* destroy the tree */
void rbtree_destroy(struct RB_TREE *tree) {
    rbtree_destroy2(tree->root);
}

void rbtree_destroy2(struct RB_NODE *root)
{
    if (root != NULL) {
	rbtree_destroy2(root->link[0]);
	rbtree_destroy2(root->link[1]);
	free(root->data);
	free(root);
    }
}

/*!
 * internal funtions used for Red Black Tree maintenance
 */

/* add a new node to the tree */
struct RB_NODE *rbtree_make_node(size_t datasize, void *data)
{
    struct RB_NODE *new_node = malloc(sizeof(*new_node));

    if (new_node != NULL) {
	new_node->data = malloc(datasize);
	if (new_node->data == NULL)
	    G_fatal_error("RB Search Tree: Out of memory!");
	    
	memcpy(new_node->data, data, datasize);
	new_node->red = 1; /* 1 is red, 0 is black */
	new_node->link[0] = NULL;
	new_node->link[1] = NULL;
    }
    else
	G_fatal_error("RB Search Tree: Out of memory!");

    return new_node;
}

/* check for red violation */
int is_red(struct RB_NODE *root)
{
    if (root)
	return root->red == 1;

    return 0;
}

/* single rotation */
struct RB_NODE *rbtree_single(struct RB_NODE *root, int dir)
{
    struct RB_NODE *newroot = root->link[!dir];

    root->link[!dir] = newroot->link[dir];
    newroot->link[dir] = root;

    root->red = 1;
    newroot->red = 0;

    return newroot;
}
 
/* double rotation */
struct RB_NODE *rbtree_double(struct RB_NODE *root, int dir)
{
    root->link[!dir] = rbtree_single(root->link[!dir], !dir);
    return rbtree_single(root, dir);
}

/* only used for debugging */
/* check for errors */
int rbtree_debug(struct RB_TREE *tree, struct RB_NODE *root)
{
    int lh, rh;
 
    if (root == NULL)
	return 1;
    else {
	struct RB_NODE *ln = root->link[0];
	struct RB_NODE *rn = root->link[1];
	int lcmp, rcmp;

	/* Consecutive red links */
	if (is_red(root)) {
	    if (is_red(ln) || is_red(rn)) {
		G_warning("Red Black Tree debugging: Red violation");
		return 0;
	    }
	}

	lh = rbtree_debug(tree, ln);
	rh = rbtree_debug(tree, rn);

	if (ln) {
	    lcmp = tree->rb_compare(ln->data, root->data);
	}
	else {
	    lcmp = 1;
	}
	
	if (rn) {
	    rcmp = tree->rb_compare(rn->data, root->data);
	}
	else {
	    rcmp = 1;
	}
	

	/* Invalid binary search tree */
	if ((ln != NULL && (lcmp == 0 || lcmp == 2))
	 || (rn != NULL && (rcmp == 1 || rcmp == 2))) {
	    G_warning("Red Black Tree debugging: Binary tree violation" );
	    return 0;
	}

	/* Black height mismatch */
	if (lh != 0 && rh != 0 && lh != rh) {
	    G_warning("Red Black Tree debugging: Black violation");
	    return 0;
	}

	/* Only count black links */
	if (lh != 0 && rh != 0)
	    return is_red(root) ? lh : lh + 1;
	else
	    return 0;
    }
}
