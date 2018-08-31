
/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <grass/gis.h>
#include <grass/raster.h>

#define INCR 10
#define NCATS 100

typedef struct
{
    int idx;
    char cat[NCATS];
    int left;
    int right;
} NODE;

static NODE *tree = 0;		/* tree of values */
static int tlen;		/* allocated tree size */
static int N;			/* number of actual nodes in tree */


int plant_tree(void)
{
    N = 0;
    if (!tree) {
	tlen = INCR;
	tree = (NODE *) G_malloc(tlen * sizeof(NODE));
    }

    return 0;
}

int add_node_to_tree(register CELL cat)
{
    register int p, q;
    int idx, offset;

    if (cat < 0) {
	idx = -(-cat / NCATS) - 1;
	offset = cat - idx * NCATS - 1;
    }
    else {
	idx = cat / NCATS;
	offset = cat - idx * NCATS;
    }
    if (offset < 0 || offset >= NCATS)
	G_warning("cat %ld got offset %d - shouldn't happen", (long)cat,
		  offset);
    /* first node is special case */
    if (N == 0) {
	N = 1;
	G_zero(tree[N].cat, sizeof(tree[N].cat));
	tree[N].idx = idx;
	tree[N].cat[offset] = 1;
	tree[N].left = 0;
	tree[N].right = 0;

	return 0;
    }

    q = 1;
    while (q > 0) {
	p = q;
	if (tree[q].idx == idx) {
	    tree[q].cat[offset] = 1;

	    return 0;		/* found */
	}
	if (tree[q].idx > idx)
	    q = tree[q].left;	/* go left */
	else
	    q = tree[q].right;	/* go right */
    }

    /* new node */
    N++;

    /* grow the tree? */
    if (N >= tlen)
	tree = (NODE *) G_realloc(tree, sizeof(NODE) * (tlen += INCR));

    /* add node to tree */
    G_zero(tree[N].cat, sizeof(tree[N].cat));
    tree[N].idx = idx;
    tree[N].cat[offset] = 1;
    tree[N].left = 0;

    if (tree[p].idx > idx) {
	tree[N].right = -p;	/* create thread */
	tree[p].left = N;	/* insert left */
    }
    else {
	tree[N].right = tree[p].right;	/* copy right link/thread */
	tree[p].right = N;	/* add right */
    }

    return 0;
}

static int curp;

#ifdef COMMENT_OUT
static int curoffset;
#endif

int first_node(void)
{
    int q;

    /* start at root and go all the way to the left */
    curp = 1;
    while ((q = tree[curp].left))
	curp = q;

    return 0;
}

int next_node(void)
{
    int q;

    /* go to the right */
    curp = tree[curp].right;

    if (curp == 0)		/* no more */
	return 0;

    if (curp < 0) {		/* thread. stop here */
	curp = -curp;
	return 1;
    }

    while ((q = tree[curp].left))	/* now go all the way left */
	curp = q;

    return 1;
}

#ifdef COMMENT_OUT
int first_cat(CELL * cat)
{
    first_node();
    curoffset = -1;

    return next_cat(cat);
}

int next_cat(CELL * cat)
{
    int idx;

    for (;;) {
	curoffset++;
	if (curoffset >= NCATS) {
	    if (!next_node())
		return 0;
	    curoffset = -1;
	    continue;
	}
	if (tree[curp].cat[curoffset]) {
	    idx = tree[curp].idx;

	    if (idx < 0)
		*cat = idx * NCATS + curoffset + 1;
	    else
		*cat = idx * NCATS + curoffset;

	    return 1;
	}
    }

    return 0;
}
#endif
