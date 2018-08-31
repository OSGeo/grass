
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
#include "index.h"

struct Rect rects[] = {
    {{0, 0, 0, 2, 2, 0}},	/* xmin, ymin, zmin, xmax, ymax, zmax (for 3 dimensional RTree) */
    {{5, 5, 0, 7, 7, 0}},
    {{8, 5, 0, 9, 6, 0}},
    {{7, 1, 0, 9, 2, 0}}
};


int nrects = sizeof(rects) / sizeof(rects[0]);
struct Rect search_rect = {
    {6, 4, 0, 10, 6, 0}		/* search will find above rects that this one overlaps */
};

int MySearchCallback(int id, void *arg)
{
    /* Note: -1 to make up for the +1 when data was inserted */
    fprintf(stdout, "Hit data rect %d\n", id - 1);
    return 1;			/* keep going */
}

int main()
{
    struct RTree *rtree = RTreeNewIndex(2);
    int i, nhits;

    fprintf(stdout, "nrects = %d\n", nrects);
    /*
     * Insert all the data rects.
     * Notes about the arguments:
     * parameter 1 is the rect being inserted,
     * parameter 2 is its ID. NOTE: *** ID MUST NEVER BE ZERO ***, hence the +1,
     * parameter 3 is the root of the tree. Note: its address is passed
     * because it can change as a result of this call, therefore no other parts
     * of this code should stash its address since it could change undernieth.
     * parameter 4 is always zero which means to add from the root.
     */
    for (i = 0; i < nrects; i++)
	RTreeInsertRect(&rects[i], i + 1, rtree);	/* i+1 is rect ID. */
    nhits = RTreeSearch(rtree, &search_rect, MySearchCallback, 0);
    fprintf(stdout, "Search resulted in %d hits\n", nhits);

    return 0;
}
