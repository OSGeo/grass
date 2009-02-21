
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "assert.h"
#include "index.h"
#include "card.h"

/* Make a new index, empty.  Consists of a single node. */
struct Node *RTreeNewIndex(void)
{
    struct Node *x;

    x = RTreeNewNode();
    x->level = 0;		/* leaf */
    return x;
}

/*
 * Search in an index tree or subtree for all data retangles that
 * overlap the argument rectangle.
 * Return the number of qualifying data rects.
 */
int RTreeSearch(struct Node *N, struct Rect *R, SearchHitCallback shcb,
		void *cbarg)
{
    register struct Node *n = N;
    register struct Rect *r = R;	/* NOTE: Suspected bug was R sent in as Node* and cast to Rect* here. */

    /* Fix not yet tested. */
    register int hitCount = 0;
    register int i;

    assert(n);
    assert(n->level >= 0);
    assert(r);

    if (n->level > 0) {		/* this is an internal node in the tree */
	for (i = 0; i < NODECARD; i++)
	    if (n->branch[i].child && RTreeOverlap(r, &n->branch[i].rect)) {
		hitCount += RTreeSearch(n->branch[i].child, r, shcb, cbarg);
	    }
    }
    else {			/* this is a leaf node */

	for (i = 0; i < LEAFCARD; i++)
	    if (n->branch[i].child && RTreeOverlap(r, &n->branch[i].rect)) {
		hitCount++;
		if (shcb)	/* call the user-provided callback */
		    if (!shcb((int)n->branch[i].child, cbarg))
			return hitCount;	/* callback wants to terminate search early */
	    }
    }
    return hitCount;
}

/*
 * Inserts a new data rectangle into the index structure.
 * Recursively descends tree, propagates splits back up.
 * Returns 0 if node was not split.  Old node updated.
 * If node was split, returns 1 and sets the pointer pointed to by
 * new_node to point to the new node.  Old node updated to become one of two.
 * The level argument specifies the number of steps up from the leaf
 * level to insert; e.g. a data rectangle goes in at level = 0.
 */
static int RTreeInsertRect2(struct Rect *r,
			    struct Node *child, struct Node *n, struct Node **new_node,
			    int level)
{
    /*
       register struct Rect *r = R;
       register int tid = Tid;
       register struct Node *n = N, **new_node = New_node;
       register int level = Level;
     */

    register int i;
    struct Branch b;
    struct Node *n2;

    assert(r && n && new_node);
    assert(level >= 0 && level <= n->level);

    /* Still above level for insertion, go down tree recursively */
    if (n->level > level) {
	i = RTreePickBranch(r, n);
	if (!RTreeInsertRect2(r, child, n->branch[i].child, &n2, level)) {
	    /* child was not split */
	    n->branch[i].rect = RTreeCombineRect(r, &(n->branch[i].rect));
	    return 0;
	}
	else {			/* child was split */

	    n->branch[i].rect = RTreeNodeCover(n->branch[i].child);
	    b.child = n2;
	    b.rect = RTreeNodeCover(n2);
	    return RTreeAddBranch(&b, n, new_node);
	}
    }

    /* Have reached level for insertion. Add rect, split if necessary */
    else if (n->level == level) {
	b.rect = *r;
	b.child = child;
	/* child field of leaves contains tid of data record */
	return RTreeAddBranch(&b, n, new_node);
    }
    else {
	/* Not supposed to happen */
	assert(FALSE);
	return 0;
    }
}

/* 
 * Insert a data rectangle into an index structure.
 * RTreeInsertRect provides for splitting the root;
 * returns 1 if root was split, 0 if it was not.
 * The level argument specifies the number of steps up from the leaf
 * level to insert; e.g. a data rectangle goes in at level = 0.
 * RTreeInsertRect2 does the recursion.
 */
int RTreeInsertRect(struct Rect *R, int Tid, struct Node **Root, int Level)
{
    assert(Level == 0);
    return RTreeInsertRect1(R, (struct Node *) Tid, Root, Level);
}

int RTreeInsertRect1(struct Rect *R, struct Node *Child, struct Node **Root, int Level)
{
    register struct Rect *r = R;
    register struct Node *child = Child;
    register struct Node **root = Root;
    register int level = Level;
    register int i;
    register struct Node *newroot;
    struct Node *newnode;
    struct Branch b;
    int result;

    assert(r && root);
    assert(level >= 0 && level <= (*root)->level);
    for (i = 0; i < NUMDIMS; i++) {
	assert(r->boundary[i] <= r->boundary[NUMDIMS + i]);
    }

    if (RTreeInsertRect2(r, child, *root, &newnode, level)) {	/* root split */
	newroot = RTreeNewNode();	/* grow a new root, & tree taller */
	newroot->level = (*root)->level + 1;
	b.rect = RTreeNodeCover(*root);
	b.child = *root;
	RTreeAddBranch(&b, newroot, NULL);
	b.rect = RTreeNodeCover(newnode);
	b.child = newnode;
	RTreeAddBranch(&b, newroot, NULL);
	*root = newroot;
	result = 1;
    }
    else
	result = 0;

    return result;
}

/*
 * Allocate space for a node in the list used in DeletRect to
 * store Nodes that are too empty.
 */
static struct ListNode *RTreeNewListNode(void)
{
    return (struct ListNode *)malloc(sizeof(struct ListNode));
    /* return new ListNode; */
}

static void RTreeFreeListNode(struct ListNode *p)
{
    free(p);
    /* delete(p); */
}

/* 
 * Add a node to the reinsertion list.  All its branches will later
 * be reinserted into the index structure.
 */
static void RTreeReInsert(struct Node *n, struct ListNode **ee)
{
    register struct ListNode *l;

    l = RTreeNewListNode();
    l->node = n;
    l->next = *ee;
    *ee = l;
}

/*
 * Delete a rectangle from non-root part of an index structure.
 * Called by RTreeDeleteRect.  Descends tree recursively,
 * merges branches on the way back up.
 * Returns 1 if record not found, 0 if success.
 */
static int
RTreeDeleteRect2(struct Rect *R, struct Node *Child, struct Node *N,
		 struct ListNode **Ee)
{
    register struct Rect *r = R;
    register struct Node *child = Child;
    register struct Node *n = N;
    register struct ListNode **ee = Ee;
    register int i;

    assert(r && n && ee);
    assert(child);
    assert(n->level >= 0);

    if (n->level > 0) {		/* not a leaf node */
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child && RTreeOverlap(r, &(n->branch[i].rect))) {
		if (!RTreeDeleteRect2(r, child, n->branch[i].child, ee)) {
		    if (n->branch[i].child->count >= MinNodeFill) {
			n->branch[i].rect =
			    RTreeNodeCover(n->branch[i].child);
		    }
		    else {
			/* not enough entries in child, eliminate child node */
			RTreeReInsert(n->branch[i].child, ee);
			RTreeDisconnectBranch(n, i);
		    }
		    return 0;
		}
	    }
	}
	return 1;
    }
    else {			/* a leaf node */

	for (i = 0; i < LEAFCARD; i++) {
	    if (n->branch[i].child &&
		n->branch[i].child == child) {
		RTreeDisconnectBranch(n, i);
		return 0;
	    }
	}
	return 1;
    }
}

/*
 * Delete a data rectangle from an index structure.
 * Pass in a pointer to a Rect, the tid of the record, ptr to ptr to root node.
 * Returns 1 if record not found, 0 if success.
 * RTreeDeleteRect provides for eliminating the root.
 */
int RTreeDeleteRect(struct Rect *R, int Tid, struct Node **Nn)
{
    /* wrapper not really needed, but restricts compile warnings to rtree lib */
    /* this way it's easier to fix if necessary? */
    return RTreeDeleteRect1(R, (struct Node *) Tid, Nn);
}

int RTreeDeleteRect1(struct Rect *R, struct Node *Child, struct Node **Nn)
{
    register struct Rect *r = R;
    register struct Node *child = Child;
    register struct Node **nn = Nn;
    register int i;
    struct Node *tmp_nptr = NULL;
    struct ListNode *reInsertList = NULL;
    register struct ListNode *e;

    assert(r && nn);
    assert(*nn);
    assert(child);

    if (!RTreeDeleteRect2(r, child, *nn, &reInsertList)) {
	/* found and deleted a data item */

	/* reinsert any branches from eliminated nodes */
	while (reInsertList) {
	    tmp_nptr = reInsertList->node;
	    for (i = 0; i < MAXKIDS(tmp_nptr); i++) {
		if (tmp_nptr->branch[i].child) {
		    RTreeInsertRect1(&(tmp_nptr->branch[i].rect),
				    tmp_nptr->branch[i].child,
				    nn, tmp_nptr->level);
		}
	    }
	    e = reInsertList;
	    reInsertList = reInsertList->next;
	    RTreeFreeNode(e->node);
	    RTreeFreeListNode(e);
	}

	/* check for redundant root (not leaf, 1 child) and eliminate */
	if ((*nn)->count == 1 && (*nn)->level > 0) {
	    for (i = 0; i < NODECARD; i++) {
		tmp_nptr = (*nn)->branch[i].child;
		if (tmp_nptr)
		    break;
	    }
	    assert(tmp_nptr);
	    RTreeFreeNode(*nn);
	    *nn = tmp_nptr;
	}
	return 0;
    }
    else {
	return 1;
    }
}
