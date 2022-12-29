/*!
 * \file rbtree.c
 *
 * \brief binary search tree 
 *
 * Generic balanced binary search tree (Red Black Tree) implementation
 *
 * (C) 2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author Julienne Walker 2003, 2008
 *         GRASS implementation Markus Metz, 2009
 */

/* balanced binary search tree implementation
 * 
 * this one is a Red Black Tree, no parent pointers, no threads
 * The core code comes from Julienne Walker's tutorials on binary search trees
 * original license: public domain
 * http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
 * some ideas come from libavl (GPL >= 2)
 *
 * Red Black Trees are used to maintain a data structure with
 * search, insertion and deletion in O(log N) time
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/rbtree.h>

/* internal functions */
static struct RB_NODE *rbtree_single(struct RB_NODE *, int);
static struct RB_NODE *rbtree_double(struct RB_NODE *, int);
static void *rbtree_first(struct RB_TRAV *);
static void *rbtree_last(struct RB_TRAV *trav);
static void *rbtree_next(struct RB_TRAV *);
static void *rbtree_previous(struct RB_TRAV *);
static struct RB_NODE *rbtree_make_node(size_t, void *);
static int is_red(struct RB_NODE *);


/* create new tree and initialize
 * returns pointer to new tree, NULL for memory allocation error
 */
struct RB_TREE *rbtree_create(rb_compare_fn *compare, size_t rb_datasize)
{
    struct RB_TREE *tree = (struct RB_TREE *)malloc(sizeof(struct RB_TREE));

    if (tree == NULL) {
	G_warning("RB tree: Out of memory!");
	return NULL;
    }

    assert(compare);

    tree->datasize = rb_datasize;
    tree->rb_compare = compare;
    tree->count = 0;
    tree->root = NULL;

    return tree;
}

/* add an item to a tree
 * non-recursive top-down insertion
 * the algorithm does not allow duplicates and also does not warn about a duplicate
 * returns 1 on success, 0 on failure
 */
int rbtree_insert(struct RB_TREE *tree, void *data)
{
    assert(tree && data);

    if (tree->root == NULL) {
	/* create a new root node for tree */
	tree->root = rbtree_make_node(tree->datasize, data);
	if (tree->root == NULL)
	    return 0;
    }
    else {
	struct RB_NODE head = { 0, 0, {0, 0} };	/* False tree root */
	struct RB_NODE *g, *t;	/* Grandparent & parent */
	struct RB_NODE *p, *q;	/* Iterator & parent */
	int dir = 0, last = 0;

	/* Set up helpers */
	t = &head;
	g = p = NULL;
	q = t->link[1] = tree->root;

	/* Search down the tree */
	for (;;) {
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

	    /* Stop if found. This check also disallows duplicates in the tree */
	    if (dir == 0)
		break;

	    dir = dir < 0;

	    /* Move the helpers down */
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

/* remove an item from a tree that matches given data
 * non-recursive top-down removal
 * returns 1 on successful removal
 * returns 0 if data item was not found
 */
int rbtree_remove(struct RB_TREE *tree, const void *data)
{
    struct RB_NODE head = { 0, 0, {0, 0} };	/* False tree root */
    struct RB_NODE *q, *p, *g;	/* Helpers */
    struct RB_NODE *f = NULL;	/* Found item */
    int dir = 1, removed = 0;

    assert(tree && data);

    if (tree->root == NULL) {
	return 0;		/* empty tree, nothing to remove */
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
	if (dir == 0)
	    f = q;

	dir = dir < 0;

	/* Push the red node down */
	if (!is_red(q) && !is_red(q->link[dir])) {
	    if (is_red(q->link[!dir]))
		p = p->link[last] = rbtree_single(q, dir);
	    else if (!is_red(q->link[!dir])) {
		struct RB_NODE *s = p->link[!last];

		if (s != NULL) {
		    if (!is_red(s->link[!last]) && !is_red(s->link[last])) {
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
	free(f->data);
	f->data = q->data;
	p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
	free(q);
	q = NULL;
	tree->count--;
	removed = 1;
    }
    else
	G_debug(2, "RB tree: data not found in search tree");

    /* Update root and make it black */
    tree->root = head.link[1];
    if (tree->root != NULL)
	tree->root->red = 0;

    return removed;
}

/* find data item in tree
 * returns pointer to data item if found else NULL
 */
void *rbtree_find(struct RB_TREE *tree, const void *data)
{
    struct RB_NODE *curr_node = tree->root;
    int cmp;

    assert(tree && data);

    while (curr_node != NULL) {
	cmp = tree->rb_compare(curr_node->data, data);
	if (cmp == 0)
	    return curr_node->data;	/* found */

	curr_node = curr_node->link[cmp < 0];
    }
    return NULL;
}

/* initialize tree traversal
 * (re-)sets trav structure
 * returns 0
 */
int rbtree_init_trav(struct RB_TRAV *trav, struct RB_TREE *tree)
{
    assert(trav && tree);

    trav->tree = tree;
    trav->curr_node = tree->root;
    trav->first = 1;
    trav->top = 0;

    return 0;
}

/* traverse the tree in ascending order
 * useful to get all items in the tree non-recursively
 * struct RB_TRAV *trav needs to be initialized first
 * returns pointer to data, NULL when finished
 */
void *rbtree_traverse(struct RB_TRAV *trav)
{
    assert(trav);

    if (trav->curr_node == NULL) {
	if (trav->first)
	    G_debug(1, "RB tree: empty tree");
	else
	    G_debug(1, "RB tree: finished traversing");

	return NULL;
    }

    if (!trav->first)
	return rbtree_next(trav);
    else {
	trav->first = 0;
	return rbtree_first(trav);
    }
}

/* traverse the tree in descending order
 * useful to get all items in the tree non-recursively
 * struct RB_TRAV *trav needs to be initialized first
 * returns pointer to data, NULL when finished
 */
void *rbtree_traverse_backwd(struct RB_TRAV *trav)
{
    assert(trav);

    if (trav->curr_node == NULL) {
	if (trav->first)
	    G_debug(1, "RB tree: empty tree");
	else
	    G_debug(1, "RB tree: finished traversing");

	return NULL;
    }

    if (!trav->first)
	return rbtree_previous(trav);
    else {
	trav->first = 0;
	return rbtree_last(trav);
    }
}

/* find start point to traverse the tree in ascending order
 * useful to get a selection of items in the tree
 * magnitudes faster than traversing the whole tree
 * may return first item that's smaller or first item that's larger
 * struct RB_TRAV *trav needs to be initialized first
 * returns pointer to data, NULL when finished
 */
void *rbtree_traverse_start(struct RB_TRAV *trav, const void *data)
{
    int dir = 0;

    assert(trav && data);

    if (trav->curr_node == NULL) {
	if (trav->first)
	    G_warning("RB tree: empty tree");
	else
	    G_warning("RB tree: finished traversing");

	return NULL;
    }

    if (!trav->first)
	return rbtree_next(trav);

    /* else first time, get start node */

    trav->first = 0;
    trav->top = 0;

    while (trav->curr_node != NULL) {
	dir = trav->tree->rb_compare(trav->curr_node->data, data);
	/* exact match, great! */
	if (dir == 0)
	    return trav->curr_node->data;
	else {
	    dir = dir < 0;
	    /* end of branch, also reached if
	     * smallest item is larger than search template or
	     * largest item is smaller than search template */
	    if (trav->curr_node->link[dir] == NULL)
		return trav->curr_node->data;

	    trav->up[trav->top++] = trav->curr_node;
	    trav->curr_node = trav->curr_node->link[dir];
	}
    }

    return NULL;		/* should not happen */
}

/* two functions needed to fully traverse the tree: initialize and continue
 * useful to get all items in the tree non-recursively
 * this one here uses a stack
 * parent pointers or threads would also be possible
 * but these would need to be added to RB_NODE
 * -> more memory needed for standard operations
 */

/* start traversing the tree
 * returns pointer to smallest data item
 */
static void *rbtree_first(struct RB_TRAV *trav)
{
    /* get smallest item */
    while (trav->curr_node->link[0] != NULL) {
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->link[0];
    }

    return trav->curr_node->data;	/* return smallest item */
}

/* start traversing the tree
 * returns pointer to largest data item
 */
static void *rbtree_last(struct RB_TRAV *trav)
{
    /* get smallest item */
    while (trav->curr_node->link[1] != NULL) {
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->link[1];
    }

    return trav->curr_node->data;	/* return smallest item */
}

/* continue traversing the tree in ascending order
 * returns pointer to data item, NULL when finished
 */
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
	return NULL;		/* finished traversing */
}

/* continue traversing the tree in descending order
 * returns pointer to data item, NULL when finished
 */
void *rbtree_previous(struct RB_TRAV *trav)
{
    if (trav->curr_node->link[0] != NULL) {
	/* something on the left side: smaller item */
	trav->up[trav->top++] = trav->curr_node;
	trav->curr_node = trav->curr_node->link[0];

	/* go down, find largest item in this branch */
	while (trav->curr_node->link[1] != NULL) {
	    trav->up[trav->top++] = trav->curr_node;
	    trav->curr_node = trav->curr_node->link[1];
	}
    }
    else {
	/* at largest item in this branch, go back up */
	struct RB_NODE *last;

	do {
	    if (trav->top == 0) {
		trav->curr_node = NULL;
		break;
	    }
	    last = trav->curr_node;
	    trav->curr_node = trav->up[--trav->top];
	} while (last == trav->curr_node->link[0]);
    }

    if (trav->curr_node != NULL) {
	return trav->curr_node->data;
    }
    else
	return NULL;		/* finished traversing */
}

/* clear the tree, removing all entries */
void rbtree_clear(struct RB_TREE *tree)
{
    struct RB_NODE *it;
    struct RB_NODE *save = tree->root;

    /*
    Rotate away the left links so that
    we can treat this like the destruction
    of a linked list
    */
    while((it = save) != NULL) {
	if (it->link[0] == NULL) {
	    /* No left links, just kill the node and move on */
	    save = it->link[1];
	    free(it->data);
	    it->data = NULL;
	    free(it);
	    it = NULL;
	}
	else {
	    /* Rotate away the left link and check again */
	    save = it->link[0];
	    it->link[0] = save->link[1];
	    save->link[1] = it;
	}
    }
    tree->root = NULL;
}

/* destroy the tree */
void rbtree_destroy(struct RB_TREE *tree)
{
    /* remove all entries */
    rbtree_clear(tree);

    free(tree);
    tree = NULL;
}

/* used for debugging: check for errors in tree structure */
int rbtree_debug(struct RB_TREE *tree, struct RB_NODE *root)
{
    int lh, rh;

    if (root == NULL)
	return 1;
    else {
	struct RB_NODE *ln = root->link[0];
	struct RB_NODE *rn = root->link[1];
	int lcmp = 0, rcmp = 0;

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

	if (rn) {
	    rcmp = tree->rb_compare(rn->data, root->data);
	}

	/* Invalid binary search tree:
	 * left node >= parent or right node <= parent */
	if ((ln != NULL && lcmp > -1)
	    || (rn != NULL && rcmp < 1)) {
	    G_warning("Red Black Tree debugging: Binary tree violation");
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

/*******************************************************
 *                                                     *
 *  internal functions for Red Black Tree maintenance  *
 *                                                     *
 *******************************************************/

/* add a new node to the tree */
static struct RB_NODE *rbtree_make_node(size_t datasize, void *data)
{
    struct RB_NODE *new_node = (struct RB_NODE *)malloc(sizeof(*new_node));

    if (new_node == NULL)
	G_fatal_error("RB Search Tree: Out of memory!");

    new_node->data = malloc(datasize);
    if (new_node->data == NULL)
	G_fatal_error("RB Search Tree: Out of memory!");

    memcpy(new_node->data, data, datasize);
    new_node->red = 1;		/* 1 is red, 0 is black */
    new_node->link[0] = NULL;
    new_node->link[1] = NULL;

    return new_node;
}

/* check for red violation */
static int is_red(struct RB_NODE *root)
{
    if (root)
	return root->red == 1;

    return 0;
}

/* single rotation */
static struct RB_NODE *rbtree_single(struct RB_NODE *root, int dir)
{
    struct RB_NODE *newroot = root->link[!dir];

    root->link[!dir] = newroot->link[dir];
    newroot->link[dir] = root;

    root->red = 1;
    newroot->red = 0;

    return newroot;
}

/* double rotation */
static struct RB_NODE *rbtree_double(struct RB_NODE *root, int dir)
{
    root->link[!dir] = rbtree_single(root->link[!dir], !dir);
    return rbtree_single(root, dir);
}
