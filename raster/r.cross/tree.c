
/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include "glob.h"


int plant_tree(void)
{
    register CELL *cat;
    register CELL *result;
    register int i;

    NODE *node;

    tree = (NODE *) G_calloc(tlen = INCR, sizeof(NODE));
    node = &tree[N = 1];

    cat = node->cat = (CELL *) G_calloc(nfiles, sizeof(CELL));
    result = node->result = (CELL *) G_calloc(NCATS, sizeof(CELL));

    i = nfiles;
    while (i--)
	*cat++ = 0;
    i = NCATS;
    while (i--)
	*result++ = 0;
    node->left = 0;
    node->right = 0;

    return 0;
}

int first_node(CELL ** cat, CELL ** result)
{
    register int p, q;

    /* start at root and go all the way to the left */
    p = 1;
    while ((q = tree[p].left))
	p = q;
    *cat = tree[p].cat;
    *result = tree[p].result;
    return p;
}

int next_node(int p, CELL ** cat, CELL ** result)
{
    register int q;

    /* go to the right */
    p = tree[p].right;

    if (p == 0)			/* no more */
	return 0;

    if (p < 0) {		/* thread. stop here */
	p = -p;
	*cat = tree[p].cat;
	*result = tree[p].result;
	return p;
    }

    while ((q = tree[p].left))	/* now go all the way left */
	p = q;

    *cat = tree[p].cat;
    *result = tree[p].result;
    return p;
}

CELL index_cat(register CELL cat)
{
    register CELL idx;

    /*
       if (cat < 0)
       idx = -(-cat/NCATS) - 1;
       else
       idx = cat/NCATS;

       if ((idx *= NCATS) < 0) idx++;
     */
    if (cat < 0)
	idx = -((-cat) >> SHIFT) - 1;
    else
	idx = cat >> SHIFT;

    if (idx < 0)
	idx = -((-idx) << SHIFT) + 1;
    else
	idx = idx << SHIFT;

    return idx;
}

int uproot_tree(void)
{
    int i;

    for (i = 1; i <= N; i++) {
	G_free(tree[i].cat);
	G_free(tree[i].result);
    }
    G_free(tree);

    return 0;
}
