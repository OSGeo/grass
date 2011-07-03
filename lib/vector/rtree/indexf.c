
/****************************************************************************
* MODULE:       R-Tree library 
*              
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*               Markus Metz - file-based and memory-based R*-tree
*               
* PURPOSE:      Multidimensional index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <grass/config.h>
#include <grass/gis.h>
#include "index.h"
#include "card.h"

/* stack used for non-recursive insertion/deletion */
struct fstack
{
    struct RTree_Node sn;	/* stack node */
    int branch_id;	/* branch no to follow down */
    off_t pos;		/* file position of stack node */
};

int RTreeValidChildF(union RTree_Child *child)
{
    return (child->pos > -1);
}

/*
 * Search in an index tree for all data retangles that
 * overlap the argument rectangle.
 * Return the number of qualifying data rects.
 */
int RTreeSearchF(struct RTree *t, struct RTree_Rect *r,
		 SearchHitCallback *shcb, void *cbarg)
{
    struct RTree_Node *n;
    int hitCount = 0, found, currlevel;
    int i;
    struct fstack s[MAXLEVEL];
    int top = 0;

    /* stack size of t->rootlevel + 1 is enough because of depth first search */
    /* only one node per level on stack at any given time */

    /* add root node position to stack */
    currlevel = t->rootlevel;
    s[top].pos = t->rootpos;
    RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);
    s[top].branch_id = i = 0;
    
    while (top >= 0) {
	if (s[top].sn.level > 0) {		/* this is an internal node in the tree */
	    n = &(s[top].sn);
	    found = 1;
	    currlevel = s[top].sn.level - 1;
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		if (s[top].sn.branch[i].child.pos > -1 &&
		    RTreeOverlap(r, &(s[top].sn.branch[i].rect), t)) {
		    s[top++].branch_id = i + 1;
		    /* add next node to stack */
		    s[top].pos = n->branch[i].child.pos;
		    RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);
		    s[top].branch_id = 0;
		    found = 0;
		    break;
		}
	    }
	    if (found) {
		/* nothing else found, go back up */
		s[top].branch_id = t->nodecard;
		top--;
	    }
	}
	else {			/* this is a leaf node */
	    for (i = 0; i < t->leafcard; i++) {
		if (s[top].sn.branch[i].child.id &&
		    RTreeOverlap(r, &(s[top].sn.branch[i].rect), t)) {
		    hitCount++;
		    if (shcb) {	/* call the user-provided callback */
			if (!shcb(s[top].sn.branch[i].child.id,
			          s[top].sn.branch[i].rect, cbarg)) {
			    /* callback wants to terminate search early */
			    return hitCount;
			}
		    }
		}
	    }
	    top--;
	}
    }

    return hitCount;
}

/*
 * Inserts a new data rectangle into the index structure.
 * Non-recursively descends tree, propagates splits back up.
 * Returns 0 if node was not split.  Old node updated.
 * If node was split, returns 1 and sets the pointer pointed to by
 * new_node to point to the new node.  Old node updated to become one of two.
 * The level argument specifies the number of steps up from the leaf
 * level to insert; e.g. a data rectangle goes in at level = 0.
 */
static int RTreeInsertRect2F(struct RTree_Rect *r, union RTree_Child child, int level, 
			     struct RTree_Node *newnode, off_t *newnode_pos,
			     struct RTree *t,
			     struct RTree_ListBranch **ee, int *overflow)
{
    int i, currlevel;
    struct RTree_Branch b;
    struct RTree_Rect nr, *cover;
    struct RTree_Node *n, *n2, nn;
    struct fstack s[MAXLEVEL];
    int top = 0, down = 0;
    int result;

    n2 = &nn;

    /* add root node position to stack */
    currlevel = t->rootlevel;
    s[top].pos = t->rootpos;
    RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);

    /* go down to level of insertion */
    while (s[top].sn.level > level) {
	n = &(s[top].sn);
	i = RTreePickBranch(r, n, t);
	s[top++].branch_id = i;
	/* add next node to stack */
	s[top].pos = n->branch[i].child.pos;
	currlevel--;
	RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);
	assert(s[top].sn.level == currlevel);
    }
    
    /* Have reached level for insertion. Add rect, split if necessary */
    if (s[top].sn.level == level) {
	b.rect = *r;
	/* child field of leaves contains tid of data record */
	b.child = child;
	/* add branch, may split node or remove branches */
	if (top)
	    cover = &(s[top - 1].sn.branch[s[top - 1].branch_id].rect);
	else 
	    cover = NULL;
	result = RTreeAddBranch(&b, &(s[top].sn), &n2, ee, cover, overflow, t);
	/* write out new node if node was split */
	if (result == 1) {
	    *newnode_pos = RTreeGetNodePos(t);
	    RTreeWriteNode(n2, t);
	    t->n_nodes++;
	}
	/* update node */
	RTreePutNode(&(s[top].sn), s[top].pos, t);
    }
    else {
	/* Not supposed to happen */
	assert(s[top].sn.level == level);
	return 0;
    }
    
    /* go back up */
    while (top) {
	down = top--;
	i = s[top].branch_id;
	if (result == 0) {        /* branch was added */
	    nr = RTreeCombineRect(r, &(s[top].sn.branch[i].rect), t);
	    /* rewrite rect */
	    if (!RTreeCompareRect(&nr, &(s[top].sn.branch[i].rect), t)) {
		RTreeUpdateRect(&nr, &(s[top].sn), s[top].pos, i, t);
	    }
	}
	else if (result == 2) {	/* branches were removed */
	    /* get node cover of previous node */
	    nr = RTreeNodeCover(&(s[down].sn), t);
	    /* rewrite rect */
	    if (!RTreeCompareRect(&nr, &(s[top].sn.branch[i].rect), t)) {
		RTreeUpdateRect(&nr, &(s[top].sn), s[top].pos, i, t);
	    }
	}
	else if (result == 1) {                /* node was split */
	    /* get node cover of previous node */
	    s[top].sn.branch[i].rect = RTreeNodeCover(&(s[down].sn), t);
	    /* add new branch for new node previously added by RTreeAddBranch() */
	    b.child.pos = *newnode_pos;
	    b.rect = RTreeNodeCover(n2, t);
	    
	    /* add branch, may split node or remove branches */
	    if (top)
		cover = &(s[top - 1].sn.branch[s[top - 1].branch_id].rect);
	    else
		cover = NULL;
	    result = RTreeAddBranch(&b, &(s[top].sn), &n2, ee, cover, overflow, t);
	    
	    /* write out new node if node was split */
	    if (result == 1) {
		*newnode_pos = RTreeGetNodePos(t);
		RTreeWriteNode(n2, t);
	 	t->n_nodes++;
	    }
	    /* update node */
	    RTreePutNode(&(s[top].sn), s[top].pos, t);
	}
    }

    *newnode = *n2;

    return result;
}

/* 
 * Insert a data rectangle into an index structure.
 * RTreeInsertRect1 provides for splitting the root;
 * returns 1 if root was split, 0 if it was not.
 * The level argument specifies the number of steps up from the leaf
 * level to insert; e.g. a data rectangle goes in at level = 0.
 * RTreeInsertRect2 does the actual insertion.
 */
int RTreeInsertRectF(struct RTree_Rect *r, union RTree_Child child, int level,
                     struct RTree *t)
{
    struct RTree_Node oldroot, newroot, newnode;
    struct RTree_Branch b;
    struct RTree_ListBranch *e, *reInsertList = NULL;
    int result;
    int i, overflow[MAXLEVEL];
    off_t newnode_pos = -1;

    /* R*-tree forced reinsertion: for each level only once */
    for (i = 0; i < MAXLEVEL; i++)
	overflow[i] = 1;

    result =
	RTreeInsertRect2F(r, child, level, &newnode, &newnode_pos, t,
	                  &reInsertList, overflow);
    
    if (result == 1) {	/* root split */
	RTreeGetNode(&oldroot, t->rootpos, t->rootlevel, t);
	/* grow a new root, & tree taller */
	t->rootlevel++;
	RTreeInitNode(&newroot, NODETYPE(t->rootlevel, t->fd));
	newroot.level = t->rootlevel;
	/* branch for old root */
	b.rect = RTreeNodeCover(&oldroot, t);
	b.child.pos = t->rootpos;
	RTreeAddBranch(&b, &newroot, NULL, NULL, NULL, NULL, t);
	/* branch for new node created by RTreeInsertRect2F() */
	b.rect = RTreeNodeCover(&newnode, t);
	b.child.pos = newnode_pos;  /* offset to new node as returned by RTreeInsertRect2F() */
	RTreeAddBranch(&b, &newroot, NULL, NULL, NULL, NULL, t);
	/* write new root node */
	t->rootpos = RTreeGetNodePos(t);
	RTreeWriteNode(&newroot, t);
	t->n_nodes++;
    }    
    else if (result == 2) {	/* branches were removed */
	while (reInsertList) {
	    /* get next branch in list */
	    b = reInsertList->b;
	    level = reInsertList->level;
	    e = reInsertList;
	    reInsertList = reInsertList->next;
	    RTreeFreeListBranch(e);
	    /* reinsert branches */
	    result =
		RTreeInsertRect2F(&(b.rect), b.child, level, &newnode, &newnode_pos, t,
				 &reInsertList, overflow);

	    if (result == 1) {	/* root split */
		RTreeGetNode(&oldroot, t->rootpos, t->rootlevel, t);
		/* grow a new root, & tree taller */
		t->rootlevel++;
		RTreeInitNode(&newroot, NODETYPE(t->rootlevel, t->fd));
		newroot.level = t->rootlevel;
		/* branch for old root */
		b.rect = RTreeNodeCover(&oldroot, t);
		b.child.pos = t->rootpos;
		RTreeAddBranch(&b, &newroot, NULL, NULL, NULL, NULL, t);
		/* branch for new node created by RTreeInsertRect2F() */
		b.rect = RTreeNodeCover(&newnode, t);
		b.child.pos = newnode_pos; 
		RTreeAddBranch(&b, &newroot, NULL, NULL, NULL, NULL, t);
		/* write new root node */
		t->rootpos = RTreeGetNodePos(t);
		RTreeWriteNode(&newroot, t);
		t->n_nodes++;
	    }
	}
    }

    return result;
}

/*
 * Delete a rectangle from non-root part of an index structure.
 * Called by RTreeDeleteRect.  Descends tree non-recursively,
 * merges branches on the way back up.
 * Returns 1 if record not found, 0 if success.
 */
static int
RTreeDeleteRect2F(struct RTree_Rect *r, union RTree_Child child, struct RTree *t,
		 struct RTree_ListNode **ee)
{
    int i, notfound = 1, currlevel;
    struct RTree_Node *n;
    struct RTree_Rect nr;
    struct fstack s[MAXLEVEL];
    int top = 0, down = 0;
    int minfill;

    assert(ee);
    
    /* add root node position to stack */
    currlevel = t->rootlevel;
    s[top].pos = t->rootpos;
    RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);
    s[top].branch_id = 0;

    while (notfound) {
	/* go down to level 0, remember path */
	if (s[top].sn.level > 0) {
	    n = &(s[top].sn);
	    currlevel = s[top].sn.level - 1;
	    for (i = s[top].branch_id; i < t->nodecard; i++) {
		if (n->branch[i].child.pos > -1 && RTreeOverlap(r, &(n->branch[i].rect), t)) {
		    s[top++].branch_id = i + 1;
		    /* add next node to stack */
		    s[top].pos = n->branch[i].child.pos;
		    RTreeGetNode(&(s[top].sn), s[top].pos, currlevel, t);
		    s[top].branch_id = 0;
		    
		    notfound = 0;
		    break;
		}
	    }
	    if (notfound) {
		/* nothing else found, go back up */
		s[top].branch_id = t->nodecard;
		top--;
	    }
	    else       /* found a way down but not yet the item */
		notfound = 1;
	}
	else {
	    for (i = 0; i < t->leafcard; i++) {
		if (s[top].sn.branch[i].child.id &&
		    s[top].sn.branch[i].child.id == child.id) { /* found item */
		    RTreeDisconnectBranch(&(s[top].sn), i, t);
		    RTreePutNode(&(s[top].sn), s[top].pos, t);
		    t->n_leafs--;
		    notfound = 0;
		    break;
		}
	    }
	    if (notfound)    /* continue searching */
		top--;
	}
    }
    
    if (notfound) {
	return notfound;
    }
    
    /* go back up */
    while (top) {
	down = top--;
	i = s[top].branch_id - 1;
	assert(s[down].sn.level == s[top].sn.level - 1);
	
	minfill = (s[down].sn.level ? t->min_node_fill : t->min_leaf_fill);
	if (s[down].sn.count >= minfill) {
	    /* just update node cover */
	    nr = RTreeNodeCover(&(s[down].sn), t);
	    /* rewrite rect */
	    if (!RTreeCompareRect(&nr, &(s[top].sn.branch[i].rect), t)) {
		RTreeUpdateRect(&nr, &(s[top].sn), s[top].pos, i, t);
	    }
	}
	else {
	    /* not enough entries in child, eliminate child node */
	    assert(s[top].sn.branch[i].child.pos == s[down].pos);
	    n = RTreeNewNode(t, s[down].sn.level);
	    memcpy(n, &(s[down].sn), t->nodesize);
	    RTreeAddNodePos(s[top].sn.branch[i].child.pos, s[down].sn.level, t);
	    RTreeReInsertNode(n, ee);
	    RTreeDisconnectBranch(&(s[top].sn), i, t);

	    RTreePutNode(&(s[top].sn), s[top].pos, t);
	}
    }

    return notfound;
}

/*
 * should be called by RTreeDeleteRect() only
 * 
 * Delete a data rectangle from an index structure.
 * Pass in a pointer to a Rect, the tid of the record, ptr RTree.
 * Returns 1 if record not found, 0 if success.
 * RTreeDeleteRect1 provides for eliminating the root.
 */
int RTreeDeleteRectF(struct RTree_Rect *r, union RTree_Child child, struct RTree *t)
{
    int i;
    struct RTree_Node *n, rn;
    struct RTree_ListNode *e, *reInsertList = NULL;

    if (!RTreeDeleteRect2F(r, child, t, &reInsertList)) {
	/* found and deleted a data item */

	/* reinsert any branches from eliminated nodes */
	while (reInsertList) {
	    t->n_nodes--;
	    n = reInsertList->node;
	    if (n->level > 0) {  /* reinsert node branches */
		for (i = 0; i < t->nodecard; i++) {
		    if (n->branch[i].child.pos > -1) {
			RTreeInsertRectF(&(n->branch[i].rect),
					 n->branch[i].child, n->level, t);
		    }
		}
	    }
	    else {  /* reinsert leaf branches */
		for (i = 0; i < t->leafcard; i++) {
		    if (n->branch[i].child.id) {
			RTreeInsertRectF(&(n->branch[i].rect),
					 n->branch[i].child, n->level, t);
		    }
		}
	    }
	    e = reInsertList;
	    reInsertList = reInsertList->next;
	    RTreeFreeNode(e->node);
	    RTreeFreeListNode(e);
	}

	/* check for redundant root (not leaf, 1 child) and eliminate */
	RTreeGetNode(&rn, t->rootpos, t->rootlevel, t);
	
	if (rn.count == 1 && rn.level > 0) {
	    for (i = 0; i < t->nodecard; i++) {
		if (rn.branch[i].child.pos > -1)
		    break;
	    }
	    RTreeAddNodePos(t->rootpos, t->rootlevel, t);
	    t->rootpos = rn.branch[i].child.pos;
	    t->rootlevel--;
	}
	return 0;
    }
    else {
	return 1;
    }
}

