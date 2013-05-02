/*!
 * \file raster/cell_stats.c
 *
 * \brief Raster Library - Raster cell statistics
 *
 * (C) 2001-2009 GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>

#define INCR 10
#define SHIFT 6

static const int NCATS = 1 << SHIFT;

#define NODE struct Cell_stats_node

static int next_node(struct Cell_stats *);
static void init_node(NODE *, int, int);

/*!
 * \brief Initialize cell stats
 *
 * This routine, which must be called first, initializes the
 * Cell_stats structure.
 *
 * Set the count for NULL-values to zero.
 *
 * \param s pointer to Cell_stats structure
 */
void Rast_init_cell_stats(struct Cell_stats *s)
{
    s->N = 0;
    s->tlen = INCR;
    s->node = (NODE *) G_malloc(s->tlen * sizeof(NODE));
    s->null_data_count = 0;
}

/*!
 * \brief Add data to cell stats
 *
 * The <i>n</i> CELL values in the <i>data</i> array are inserted (and
 * counted) in the Cell_stats structure.
 *
 * Look for NULLs and update the NULL-value count.
 *
 * \param cell raster values
 * \param n number of values
 * \param s pointer to Cell_stats structure which holds cell stats info
 *
 * \return 1 on failure
 * \return 0 on success
 */
int Rast_update_cell_stats(const CELL * cell, int n, struct Cell_stats *s)
{
    CELL cat;
    int p, q;
    int idx, offset;
    int N;
    NODE *node, *pnode;
    NODE *new_node;

    if (n <= 0)
	return 1;

    node = s->node;

    /* first non-null node is special case */
    if ((N = s->N) == 0) {
	cat = *cell++;
	while (Rast_is_c_null_value(&cat)) {
	    s->null_data_count++;
	    cat = *cell++;
	    n--;
	}
	if (n > 0) {		/* if there are some non-null cells */
	    N = 1;
	    if (cat < 0) {
		idx = -((-cat) >> SHIFT) - 1;
		offset = cat + ((-idx) << SHIFT) - 1;
	    }
	    else {
		idx = cat >> SHIFT;
		offset = cat - (idx << SHIFT);
	    }
	    fflush(stderr);
	    init_node(&node[1], idx, offset);
	    node[1].right = 0;
	    n--;
	}
    }
    while (n-- > 0) {
	cat = *cell++;
	if (Rast_is_c_null_value(&cat)) {
	    s->null_data_count++;
	    continue;
	}
	if (cat < 0) {
	    idx = -((-cat) >> SHIFT) - 1;
	    offset = cat + ((-idx) << SHIFT) - 1;
	}
	else {
	    idx = cat >> SHIFT;
	    offset = cat - (idx << SHIFT);
	}

	q = 1;
	while (q > 0) {
	    pnode = &node[p = q];
	    if (pnode->idx == idx) {
		pnode->count[offset]++;
		break;
	    }
	    if (pnode->idx > idx)
		q = pnode->left;	/* go left */
	    else
		q = pnode->right;	/* go right */
	}
	if (q > 0)
	    continue;		/* found */

	/* new node */
	N++;

	/* grow the tree? */
	if (N >= s->tlen) {
	    node =
		(NODE *) G_realloc((char *)node,
				   sizeof(NODE) * (s->tlen += INCR));
	    pnode = &node[p];	/* realloc moves node, must reassign pnode */
	}

	/* add node to tree */
	init_node(new_node = &node[N], idx, offset);

	if (pnode->idx > idx) {
	    new_node->right = -p;	/* create thread */
	    pnode->left = N;	/* insert left */
	}
	else {
	    new_node->right = pnode->right;	/* copy right link/thread */
	    pnode->right = N;	/* add right */
	}
    }				/* while n-- > 0 */
    s->N = N;
    s->node = node;

    return 0;
}

static void init_node(NODE * node, int idx, int offset)
{
    long *count;
    int i;

    count = node->count = (long *)G_calloc(i = NCATS, sizeof(long));
    while (i--)
	*count++ = 0;
    node->idx = idx;
    node->count[offset] = 1;
    node->left = 0;
}


/*!
 * \brief Random query of cell stats
 *
 * This routine allows a random query of the Cell_stats structure. The
 * \p count associated with the raster value \p cat is
 * set. The routine returns 1 if \p cat was found in the
 * structure, 0 otherwise.
 *
 * Allow finding the count for the NULL-value.
 *
 * \param cat raster value
 * \param[out] count count
 * \param s pointer to Cell_stats structure which holds cell stats info
 *
 * \return 1 if found
 * \return 0 if not found
 */
int Rast_find_cell_stat(CELL cat, long *count, const struct Cell_stats *s)
{
    int q;
    int idx;
    int offset;

    *count = 0;
    if (Rast_is_c_null_value(&cat)) {
	*count = s->null_data_count;
	return (*count != 0);
    }

    if (s->N <= 0)
	return 0;

    /*
       if (cat < 0)
       {
       idx = -(-cat/NCATS) - 1;
       offset = cat - idx*NCATS - 1;
       }
       else
       {
       idx = cat/NCATS;
       offset = cat - idx*NCATS;
       }
     */
    if (cat < 0) {
	idx = -((-cat) >> SHIFT) - 1;
	offset = cat + ((-idx) << SHIFT) - 1;
    }
    else {
	idx = cat >> SHIFT;
	offset = cat - (idx << SHIFT);
    }

    q = 1;
    while (q > 0) {
	if (s->node[q].idx == idx) {
	    *count = s->node[q].count[offset];
	    return (*count != 0);
	}
	if (s->node[q].idx > idx)
	    q = s->node[q].left;	/* go left */
	else
	    q = s->node[q].right;	/* go right */
    }
    return 0;
}

/*!
 * \brief Reset/rewind cell stats
 *
 * The structure <i>s</i> is rewound (i.e., positioned at the first
 * raster category) so that sorted sequential retrieval can begin.
 *
 * \param s pointer to Cell_stats structure which holds cell stats info
 *
 * \return 0
 */
int Rast_rewind_cell_stats(struct Cell_stats *s)
{
    int q;

    if (s->N <= 0)
	return 1;
    /* start at root and go all the way to the left */
    s->curp = 1;
    while ((q = s->node[s->curp].left))
	s->curp = q;
    s->curoffset = -1;

    return 0;
}

static int next_node(struct Cell_stats *s)
{
    int q;

    /* go to the right */
    s->curp = s->node[s->curp].right;

    if (s->curp == 0)		/* no more */
	return 0;

    if (s->curp < 0) {		/* thread. stop here */
	s->curp = -(s->curp);
	return 1;
    }

    while ((q = s->node[s->curp].left))	/* now go all the way left */
	s->curp = q;

    return 1;
}

/*!
 * \brief Retrieve sorted cell stats
 *
 * Retrieves the next <i>cat, count</i> combination from the
 * structure. Returns 0 if there are no more items, non-zero if there
 * are more. For example:
 * 
 \code
 struct Cell_stats s;
 CELL cat;
 long count;

 // updating <b>s</b> occurs here

 Rast_rewind_cell_stats(&s);
 while (Rast_next_cell_stat(&cat,&count,&s)
 fprintf(stdout, "%ld %ld\n", (long) cat, count);
 \endcode
 *
 * Do not return a record for the NULL-value
 *
 * \param cat raster value
 * \param[out] count
 * \param s pointer to Cell_stats structure which holds cell stats info
 *
 * \return 0 if there are no more items
 * \return non-zero if there are more
 */
int Rast_next_cell_stat(CELL * cat, long *count, struct Cell_stats *s)
{
    int idx;

    /* first time report stats for null */
    /* decided not to return stats for null in this function 
       static int null_reported = 0;
       if(!null_reported && s->null_data_count > 0)
       {
       *count = s->null_data_count;
       Rast_set_c_null_value(&cat,1);
       null_reported = 1;
       return 1;
       }
     */
    if (s->N <= 0)
	return 0;
    for (;;) {
	s->curoffset++;
	if (s->curoffset >= NCATS) {
	    if (!next_node(s))
		return 0;
	    s->curoffset = -1;
	    continue;
	}
	if ((*count = s->node[s->curp].count[s->curoffset])) {
	    idx = s->node[s->curp].idx;

	    /*
	       if (idx < 0)
	       *cat = idx*NCATS + s->curoffset+1;
	       else
	       *cat = idx*NCATS + s->curoffset;
	     */
	    if (idx < 0)
		*cat = -((-idx) << SHIFT) + s->curoffset + 1;
	    else
		*cat = (idx << SHIFT) + s->curoffset;

	    return 1;
	}
    }
}


/*!
 * \brief Get number of null values.
 *
 * Get a number of null values from stats structure.
 *
 * Note: when reporting values which appear in a map using
 * Rast_next_cell_stats(), to get stats for null, call
 * Rast_get_stats_for_null_value() first, since Rast_next_cell_stats() does
 * not report stats for null.
 *
 * \param count count
 * \param s pointer to Cell_stats structure which holds cell stats info
 */
void Rast_get_stats_for_null_value(long *count, const struct Cell_stats *s)
{
    *count = s->null_data_count;
}

/*!
 * \brief Free cell stats structure
 *
 * The memory associated with structure <i>s</i> is freed. This
 * routine may be called any time after calling Rast_init_cell_stats().
 *
 * \param s pointer to Cell_stats structure
 */
void Rast_free_cell_stats(struct Cell_stats *s)
{
    int i;

    for (i = 1; i <= s->N; i++)
	G_free(s->node[i].count);
    G_free(s->node);
}
