
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
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <grass/gis.h>
#include "index.h"
#include "card.h"

/* rectangle distances for forced removal */
struct dist
{
    int id;			/* branch id */
    RectReal distance;		/* distance to node center */
};

/* Initialize one branch cell in an internal node. */
static void RTreeInitNodeBranchM(struct Branch *b)
{
    RTreeInitRect(&(b->rect));
    b->child.ptr = NULL;
}

/* Initialize one branch cell in an internal node. */
static void RTreeInitNodeBranchF(struct Branch *b)
{
    RTreeInitRect(&(b->rect));
    b->child.pos = -1;
}

/* Initialize one branch cell in a leaf node. */
static void RTreeInitLeafBranch(struct Branch *b)
{
    RTreeInitRect(&(b->rect));
    b->child.id = 0;
}

static void (*RTreeInitBranch[3]) () = {
    RTreeInitLeafBranch, RTreeInitNodeBranchM, RTreeInitNodeBranchF
};

/* Initialize a Node structure. */
/* type = 1: leaf, type = 2: internal, memory, type = 3: internal, file */
void RTreeInitNode(struct Node *n, int type)
{
    int i;

    n->count = 0;
    n->level = -1;

    for (i = 0; i < MAXCARD; i++)
	RTreeInitBranch[type](&(n->branch[i]));
}

/* Make a new node and initialize to have all branch cells empty. */
struct Node *RTreeNewNode(struct RTree *t, int level)
{
    struct Node *n;

    n = (struct Node *)malloc((size_t) t->nodesize);
    assert(n);
    RTreeInitNode(n, NODETYPE(level, t->fd));
    return n;
}

void RTreeFreeNode(struct Node *n)
{
    assert(n);
    free(n);
}

/*
 * Find the smallest rectangle that includes all rectangles in
 * branches of a node.
 */
struct Rect RTreeNodeCover(struct Node *n, struct RTree *t)
{
    int i, first_time = 1;
    struct Rect r;

    RTreeInitRect(&r);
    if ((n)->level > 0) { /* internal node */
	for (i = 0; i < t->nodecard; i++) {
	    if (t->valid_child(&(n->branch[i].child))) {
		if (first_time) {
		    r = n->branch[i].rect;
		    first_time = 0;
		}
		else
		    r = RTreeCombineRect(&r, &(n->branch[i].rect), t);
	    }
	}
    }
    else {  /* leaf */
	for (i = 0; i < t->leafcard; i++) {
	    if (n->branch[i].child.id) {
		if (first_time) {
		    r = n->branch[i].rect;
		    first_time = 0;
		}
		else
		    r = RTreeCombineRect(&r, &(n->branch[i].rect), t);
	    }
	}
    }
    return r;
}

/*
 * Idea from R*-tree, modified: not overlap size but overlap number
 * 
 * Pick a branch from leaf nodes (current node has level 1).  Pick the 
 * one that will result in the smallest number of overlapping siblings.
 * This will result in the least ambiguous node covering the new 
 * rectangle, improving search speed.
 * In case of a tie, pick the one which needs the smallest increase in
 * area to accomodate the new rectangle, then the smallest area before,
 * to get the best resolution when searching.
 */

static int RTreePickLeafBranch(struct Rect *r, struct Node *n, struct RTree *t)
{
    struct Rect *rr;
    int i, j;
    RectReal increase, bestIncr = -1, area, bestArea = 0;
    int best = 0, bestoverlap;
    struct Rect tmp_rect;
    int overlap;

    bestoverlap = t->nodecard + 1;

    /* get the branch that will overlap with the smallest number of 
     * sibling branches when including the new rectangle */
    for (i = 0; i < t->nodecard; i++) {
	if (t->valid_child(&(n->branch[i].child))) {
	    rr = &n->branch[i].rect;
	    tmp_rect = RTreeCombineRect(r, rr, t);
	    area = RTreeRectSphericalVolume(rr, t);
	    increase = RTreeRectSphericalVolume(&tmp_rect, t) - area;

	    overlap = 0;
	    for (j = 0; j < t->leafcard; j++) {
		if (j != i) {
		    rr = &n->branch[j].rect;
		    overlap += RTreeOverlap(&tmp_rect, rr, t);
		}
	    }

	    if (overlap < bestoverlap) {
		best = i;
		bestoverlap = overlap;
		bestArea = area;
		bestIncr = increase;
	    }
	    else if (overlap == bestoverlap) {
		/* resolve ties */
		if (increase < bestIncr) {
		    best = i;
		    bestArea = area;
		    bestIncr = increase;
		}
		else if (increase == bestIncr && area < bestArea) {
		    best = i;
		    bestArea = area;
		}
	    }
	}
    }

    return best;
}

/*
 * Pick a branch.  Pick the one that will need the smallest increase
 * in area to accomodate the new rectangle.  This will result in the
 * least total area for the covering rectangles in the current node.
 * In case of a tie, pick the one which was smaller before, to get
 * the best resolution when searching.
 */
int RTreePickBranch(struct Rect *r, struct Node *n, struct RTree *t)
{
    struct Rect *rr;
    int i, first_time = 1;
    RectReal increase, bestIncr = (RectReal) -1, area, bestArea = 0;
    int best = 0;
    struct Rect tmp_rect;

    assert((n)->level > 0);	/* must not be called on leaf node */

    if ((n)->level == 1)
	return RTreePickLeafBranch(r, n, t);

    for (i = 0; i < t->nodecard; i++) {
	if (t->valid_child(&(n->branch[i].child))) {
	    rr = &n->branch[i].rect;
	    area = RTreeRectSphericalVolume(rr, t);
	    tmp_rect = RTreeCombineRect(r, rr, t);
	    increase = RTreeRectSphericalVolume(&tmp_rect, t) - area;
	    if (increase < bestIncr || first_time) {
		best = i;
		bestArea = area;
		bestIncr = increase;
		first_time = 0;
	    }
	    else if (increase == bestIncr && area < bestArea) {
		best = i;
		bestArea = area;
	    }
	}
    }
    return best;
}

/* Disconnect a dependent node. */
void RTreeDisconnectBranch(struct Node *n, int i, struct RTree *t)
{
    if ((n)->level > 0) {
	assert(n && i >= 0 && i < t->nodecard);
	assert(t->valid_child(&(n->branch[i].child)));
	if (t->fd < 0)
	    RTreeInitNodeBranchM(&(n->branch[i]));
	else
	    RTreeInitNodeBranchF(&(n->branch[i]));
    }
    else {
	assert(n && i >= 0 && i < t->leafcard);
	assert(n->branch[i].child.id);
	RTreeInitLeafBranch(&(n->branch[i]));
    }

    n->count--;
}

/* Destroy (free) node recursively. */
/* NOTE: only needed for memory based index */
void RTreeDestroyNode(struct Node *n, int nodes)
{
    int i;

    if (n->level > 0) {		/* it is not leaf -> destroy childs */
	for (i = 0; i < nodes; i++) {
	    if (n->branch[i].child.ptr) {
		RTreeDestroyNode(n->branch[i].child.ptr, nodes);
	    }
	}
    }

    /* Free this node */
    RTreeFreeNode(n);
}

/****************************************************************
 *                                                              *
 *   R*-tree: force remove FORCECARD branches for reinsertion   *
 *                                                              *
 ****************************************************************/

/*
 * swap dist structs
 */
static void RTreeSwapDist(struct dist *a, struct dist *b)
{
    struct dist c;

    c = *a;
    *a = *b;
    *b = c;
}

/*
 * check if dist is sorted ascending to distance
 */
static int RTreeDistIsSorted(struct dist *d, int first, int last)
{
    int i;

    for (i = first; i < last; i++) {
	if (d[i].distance > d[i + 1].distance)
	    return 0;
    }

    return 1;
}

/*
 * partition dist for quicksort on distance
 */
static int RTreePartitionDist(struct dist *d, int first, int last)
{
    int pivot, mid = (first + last) / 2;
    int larger, smaller;

    if (last - first == 1) {	/* only two items in list */
	if (d[first].distance > d[last].distance) {
	    RTreeSwapDist(&(d[first]), &(d[last]));
	}
	return last;
    }

    /* Larger of two */
    if (d[first].distance > d[mid].distance) {
	larger = pivot = first;
	smaller = mid;
    }
    else {
	larger = pivot = mid;
	smaller = first;
    }

    if (d[larger].distance > d[last].distance) {
	/* larger is largest, get the larger of smaller and last */
	if (d[smaller].distance > d[last].distance) {
	    pivot = smaller;
	}
	else {
	    pivot = last;
	}
    }

    if (pivot != last) {
	RTreeSwapDist(&(d[pivot]), &(d[last]));
    }

    pivot = first;

    while (first < last) {
	if (d[first].distance <= d[last].distance) {
	    if (pivot != first) {
		RTreeSwapDist(&(d[pivot]), &(d[first]));
	    }
	    pivot++;
	}
	++first;
    }

    if (pivot != last) {
	RTreeSwapDist(&(d[pivot]), &(d[last]));
    }

    return pivot;
}

/*
 * quicksort dist struct ascending by distance
 * n is last valid index
 */
static void RTreeQuicksortDist(struct dist *d, int n)
{
    int pivot, first, last;
    int s_first[MAXCARD + 1], s_last[MAXCARD + 1], stacksize;

    s_first[0] = 0;
    s_last[0] = n;

    stacksize = 1;

    /* use stack */
    while (stacksize) {
	stacksize--;
	first = s_first[stacksize];
	last = s_last[stacksize];
	if (first < last) {
	    if (!RTreeDistIsSorted(d, first, last)) {

		pivot = RTreePartitionDist(d, first, last);

		s_first[stacksize] = first;
		s_last[stacksize] = pivot - 1;
		stacksize++;

		s_first[stacksize] = pivot + 1;
		s_last[stacksize] = last;
		stacksize++;
	    }
	}
    }
}

/*
 * Allocate space for a branch in the list used in InsertRect to
 * store branches of nodes that are too full.
 */
static struct ListBranch *RTreeNewListBranch(void)
{
    return (struct ListBranch *)malloc(sizeof(struct ListBranch));
}

/* 
 * Add a branch to the reinsertion list.  It will later
 * be reinserted into the index structure.
 */
static void RTreeReInsertBranch(struct Branch b, int level,
				struct ListBranch **ee)
{
    register struct ListBranch *l;

    l = RTreeNewListBranch();
    l->b = b;
    l->level = level;
    l->next = *ee;
    *ee = l;
}

/*
 * Remove branches from a node. Select the 2 branches whose rectangle 
 * center is farthest away from node cover center.
 * Old node updated.
 */
static void RTreeRemoveBranches(struct Node *n, struct Branch *b,
				struct ListBranch **ee, struct Rect *cover,
				struct RTree *t)
{
    int i, j, maxkids, type;
    RectReal center_n[NUMDIMS], center_r, delta;
    struct Branch branchbuf[MAXCARD + 1];
    struct dist rdist[MAXCARD + 1];
    struct Rect new_cover;

    assert(cover);

    maxkids = MAXKIDS((n)->level, t);
    type = NODETYPE((n)->level, t->fd);
    
    assert(n->count == maxkids);	/* must be full */

    new_cover = RTreeCombineRect(cover, &(b->rect), t);

    /* center coords of node cover */
    for (j = 0; j < t->ndims; j++) {
	center_n[j] = (new_cover.boundary[j + NUMDIMS] + new_cover.boundary[j]) / 2;
    }

    /* compute distances of child rectangle centers to node cover center */
    for (i = 0; i < maxkids; i++) {
	branchbuf[i] = n->branch[i];
	rdist[i].distance = 0;
	rdist[i].id = i;
	for (j = 0; j < t->ndims; j++) {
	    center_r =
		(branchbuf[i].rect.boundary[j + NUMDIMS] +
		 branchbuf[i].rect.boundary[j]) / 2;
	    delta = center_n[j] - center_r;
	    rdist[i].distance += delta * delta;
	}

	RTreeInitBranch[type](&(n->branch[i]));
    }

    /* new branch */
    branchbuf[maxkids] = *b;
    rdist[maxkids].distance = 0;
    for (j = 0; j < t->ndims; j++) {
	center_r =
	    (b->rect.boundary[j + NUMDIMS] +
	     b->rect.boundary[j]) / 2;
	delta = center_n[j] - center_r;
	rdist[maxkids].distance += delta * delta;
    }
    rdist[maxkids].id = maxkids;

    /* quicksort dist */
    RTreeQuicksortDist(rdist, maxkids);

    /* put largest three in branch list, farthest from center first */
    for (i = 0; i < FORCECARD; i++) {
	RTreeReInsertBranch(branchbuf[rdist[maxkids - i].id], n->level, ee);
    }
    /* put remaining in node, closest to center first */
    for (i = 0; i < maxkids - FORCECARD + 1; i++) {
	n->branch[i] = branchbuf[rdist[i].id];
    }
    n->count = maxkids - FORCECARD + 1;
}

/*
 * Add a branch to a node.  Split the node if necessary.
 * Returns 0 if node not split.  Old node updated.
 * Returns 1 if node split, sets *new_node to address of new node.
 * Old node updated, becomes one of two.
 * Returns 2 if branches were removed for forced reinsertion
 */
int RTreeAddBranch(struct Branch *b, struct Node *n,
		   struct Node **newnode, struct ListBranch **ee,
		   struct Rect *cover, int *overflow, struct RTree *t)
{
    int i, maxkids;

    maxkids = MAXKIDS((n)->level, t);

    if (n->count < maxkids) {	/* split won't be necessary */
	if ((n)->level > 0) {   /* internal node */
	    for (i = 0; i < maxkids; i++) {	/* find empty branch */
		if (!t->valid_child(&(n->branch[i].child))) {
		    n->branch[i] = *b;
		    n->count++;
		    break;
		}
	    }
	    return 0;
	}
	else if ((n)->level == 0) {   /* leaf */
	    for (i = 0; i < maxkids; i++) {	/* find empty branch */
		if (n->branch[i].child.id == 0) {
		    n->branch[i] = *b;
		    n->count++;
		    break;
		}
	    }
	    return 0;
	}
    }
    else {
	if (n->level < t->rootlevel && overflow[n->level]) {
	    /* R*-tree forced reinsert */
	    RTreeRemoveBranches(n, b, ee, cover, t);
	    overflow[n->level] = 0;
	    return 2;
	}
	else {
	    if (t->fd > -1)
		RTreeInitNode(*newnode, NODETYPE(n->level, t->fd));
	    else
		*newnode = RTreeNewNode(t, (n)->level);
	    RTreeSplitNode(n, b, *newnode, t);
	    return 1;
	}
    }

    /* should not be reached */
    assert(0);
    return -1;
}

/*
 * for debugging only: print items to stdout
 */

void RTreeTabIn(int depth)
{
    int i;

    for (i = 0; i < depth; i++)
	putchar('\t');
}

static void RTreePrintBranch(struct Branch *b, int depth, struct RTree *t)
{
    RTreePrintRect(&(b->rect), depth);
    RTreePrintNode(b->child.ptr, depth, t);
}

/* Print out the data in a node. */
void RTreePrintNode(struct Node *n, int depth, struct RTree *t)
{
    int i, maxkids;

    RTreeTabIn(depth);

    maxkids = (n->level > 0 ? t->nodecard : t->leafcard);

    fprintf(stdout, "node");
    if (n->level == 0)
	fprintf(stdout, " LEAF");
    else if (n->level > 0)
	fprintf(stdout, " NONLEAF");
    else
	fprintf(stdout, " TYPE=?");
    fprintf(stdout, "  level=%d  count=%d", n->level, n->count);

    for (i = 0; i < maxkids; i++) {
	if (n->level == 0) {
	    RTreeTabIn(depth);
	    RTreePrintRect(&(n->branch[i].rect), depth);
	    fprintf(stdout, "\t%d: data id = %d\n", i,
		  n->branch[i].child.id);
	}
	else {
	    RTreeTabIn(depth);
	    fprintf(stdout, "branch %d\n", i);
	    RTreePrintBranch(&(n->branch[i]), depth + 1, t);
	}
    }
}

