
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
#include "assert.h"
#include "index.h"
#include "card.h"
#include "split_q.h"

struct Branch BranchBuf[MAXCARD + 1];
int BranchCount;
struct Rect CoverSplit;
RectReal CoverSplitArea;

/* variables for finding a partition */
struct PartitionVars Partitions[METHODS];

/*-----------------------------------------------------------------------------
| Load branch buffer with branches from full node plus the extra branch.
-----------------------------------------------------------------------------*/
static void RTreeGetBranches(struct Node *n, struct Branch *b)
{
    int i;

    assert(n);
    assert(b);

    /* load the branch buffer */
    for (i = 0; i < MAXKIDS(n); i++) {
	assert(n->branch[i].child);	/* n should have every entry full */
	BranchBuf[i] = n->branch[i];
    }
    BranchBuf[MAXKIDS(n)] = *b;
    BranchCount = MAXKIDS(n) + 1;

    /* calculate rect containing all in the set */
    CoverSplit = BranchBuf[0].rect;
    for (i = 1; i < MAXKIDS(n) + 1; i++) {
	CoverSplit = RTreeCombineRect(&CoverSplit, &BranchBuf[i].rect);
    }
    CoverSplitArea = RTreeRectSphericalVolume(&CoverSplit);

    RTreeInitNode(n);
}




/*-----------------------------------------------------------------------------
| Put a branch in one of the groups.
-----------------------------------------------------------------------------*/
static void RTreeClassify(int i, int group, struct PartitionVars *p)
{
    assert(p);
    assert(!p->taken[i]);

    p->partition[i] = group;
    p->taken[i] = TRUE;

    if (p->count[group] == 0)
	p->cover[group] = BranchBuf[i].rect;
    else
	p->cover[group] =
	    RTreeCombineRect(&BranchBuf[i].rect, &p->cover[group]);
    p->area[group] = RTreeRectSphericalVolume(&p->cover[group]);
    p->count[group]++;
}




/*-----------------------------------------------------------------------------
| Pick two rects from set to be the first elements of the two groups.
| Pick the two that waste the most area if covered by a single rectangle.
-----------------------------------------------------------------------------*/
static void RTreePickSeeds(struct PartitionVars *p)
{
    int i, j, seed0 = 0, seed1 = 0;
    RectReal worst, waste, area[MAXCARD + 1];

    for (i = 0; i < p->total; i++)
	area[i] = RTreeRectSphericalVolume(&BranchBuf[i].rect);

    worst = -CoverSplitArea - 1;
    for (i = 0; i < p->total - 1; i++) {
	for (j = i + 1; j < p->total; j++) {
	    struct Rect one_rect;

	    one_rect = RTreeCombineRect(&BranchBuf[i].rect,
					&BranchBuf[j].rect);
	    waste = RTreeRectSphericalVolume(&one_rect) - area[i] - area[j];
	    if (waste > worst) {
		worst = waste;
		seed0 = i;
		seed1 = j;
	    }
	}
    }
    RTreeClassify(seed0, 0, p);
    RTreeClassify(seed1, 1, p);
}




/*-----------------------------------------------------------------------------
| Copy branches from the buffer into two nodes according to the partition.
-----------------------------------------------------------------------------*/
static void RTreeLoadNodes(struct Node *n, struct Node *q,
			   struct PartitionVars *p)
{
    int i;

    assert(n);
    assert(q);
    assert(p);

    for (i = 0; i < p->total; i++) {
	assert(p->partition[i] == 0 || p->partition[i] == 1);
	if (p->partition[i] == 0)
	    RTreeAddBranch(&BranchBuf[i], n, NULL);
	else if (p->partition[i] == 1)
	    RTreeAddBranch(&BranchBuf[i], q, NULL);
    }
}




/*-----------------------------------------------------------------------------
| Initialize a PartitionVars structure.
-----------------------------------------------------------------------------*/
static void RTreeInitPVars(struct PartitionVars *p, int maxrects, int minfill)
{
    int i;

    assert(p);

    p->count[0] = p->count[1] = 0;
    p->cover[0] = p->cover[1] = RTreeNullRect();
    p->area[0] = p->area[1] = (RectReal) 0;
    p->total = maxrects;
    p->minfill = minfill;
    for (i = 0; i < maxrects; i++) {
	p->taken[i] = FALSE;
	p->partition[i] = -1;
    }
}




/*-----------------------------------------------------------------------------
| Print out data for a partition from PartitionVars struct.
-----------------------------------------------------------------------------*/
static void RTreePrintPVars(struct PartitionVars *p)
{
    int i;

    assert(p);

    fprintf(stdout, "\npartition:\n");
    for (i = 0; i < p->total; i++) {
	fprintf(stdout, "%3d\t", i);
    }
    fprintf(stdout, "\n");
    for (i = 0; i < p->total; i++) {
	if (p->taken[i])
	    fprintf(stdout, "  t\t");
	else
	    fprintf(stdout, "\t");
    }
    fprintf(stdout, "\n");
    for (i = 0; i < p->total; i++) {
	fprintf(stdout, "%3d\t", p->partition[i]);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "count[0] = %d  area = %f\n", p->count[0], p->area[0]);
    fprintf(stdout, "count[1] = %d  area = %f\n", p->count[1], p->area[1]);
    if (p->area[0] + p->area[1] > 0) {
	fprintf(stdout, "total area = %f  effectiveness = %3.2f\n",
		p->area[0] + p->area[1],
		(float)CoverSplitArea / (p->area[0] + p->area[1]));
    }
    fprintf(stdout, "cover[0]:\n");
    RTreePrintRect(&p->cover[0], 0);

    fprintf(stdout, "cover[1]:\n");
    RTreePrintRect(&p->cover[1], 0);
}


/*-----------------------------------------------------------------------------
| Method #0 for choosing a partition:
| As the seeds for the two groups, pick the two rects that would waste the
| most area if covered by a single rectangle, i.e. evidently the worst pair
| to have in the same group.
| Of the remaining, one at a time is chosen to be put in one of the two groups.
| The one chosen is the one with the greatest difference in area expansion
| depending on which group - the rect most strongly attracted to one group
| and repelled from the other.
| If one group gets too full (more would force other group to violate min
| fill requirement) then other group gets the rest.
| These last are the ones that can go in either group most easily.
-----------------------------------------------------------------------------*/
static void RTreeMethodZero(struct PartitionVars *p, int minfill)
{
    int i;
    RectReal biggestDiff;
    int group, chosen = 0, betterGroup = 0;

    assert(p);

    RTreeInitPVars(p, BranchCount, minfill);
    RTreePickSeeds(p);

    while (p->count[0] + p->count[1] < p->total
	   && p->count[0] < p->total - p->minfill
	   && p->count[1] < p->total - p->minfill) {
	biggestDiff = (RectReal) - 1.;
	for (i = 0; i < p->total; i++) {
	    if (!p->taken[i]) {
		struct Rect *r, rect_0, rect_1;
		RectReal growth0, growth1, diff;

		r = &BranchBuf[i].rect;
		rect_0 = RTreeCombineRect(r, &p->cover[0]);
		rect_1 = RTreeCombineRect(r, &p->cover[1]);
		growth0 = RTreeRectSphericalVolume(&rect_0) - p->area[0];
		growth1 = RTreeRectSphericalVolume(&rect_1) - p->area[1];
		diff = growth1 - growth0;
		if (diff >= 0)
		    group = 0;
		else {
		    group = 1;
		    diff = -diff;
		}

		if (diff > biggestDiff) {
		    biggestDiff = diff;
		    chosen = i;
		    betterGroup = group;
		}
		else if (diff == biggestDiff &&
			 p->count[group] < p->count[betterGroup]) {
		    chosen = i;
		    betterGroup = group;
		}
	    }
	}
	RTreeClassify(chosen, betterGroup, p);
    }

    /* if one group too full, put remaining rects in the other */
    if (p->count[0] + p->count[1] < p->total) {
	if (p->count[0] >= p->total - p->minfill)
	    group = 1;
	else
	    group = 0;
	for (i = 0; i < p->total; i++) {
	    if (!p->taken[i])
		RTreeClassify(i, group, p);
	}
    }

    assert(p->count[0] + p->count[1] == p->total);
    assert(p->count[0] >= p->minfill && p->count[1] >= p->minfill);
}


/*-----------------------------------------------------------------------------
| Split a node.
| Divides the nodes branches and the extra one between two nodes.
| Old node is one of the new ones, and one really new one is created.
| Tries more than one method for choosing a partition, uses best result.
-----------------------------------------------------------------------------*/
void RTreeSplitNode(struct Node *n, struct Branch *b, struct Node **nn)
{
    struct PartitionVars *p;
    int level;

    assert(n);
    assert(b);

    /* load all the branches into a buffer, initialize old node */
    level = n->level;
    RTreeGetBranches(n, b);

    /* find partition */
    p = &Partitions[0];
    /* Note: can't use MINFILL(n) below since n was cleared by GetBranches() */
    RTreeMethodZero(p, level > 0 ? MinNodeFill : MinLeafFill);

    /*
     * put branches from buffer into 2 nodes
     * according to chosen partition
     */
    *nn = RTreeNewNode();
    (*nn)->level = n->level = level;
    RTreeLoadNodes(n, *nn, p);
    assert(n->count + (*nn)->count == p->total);
}
