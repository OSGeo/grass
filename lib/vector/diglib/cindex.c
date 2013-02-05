
/*****************************************************************************
*
* MODULE:       Vector library 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/vector.h>

/*! 
 * \brief Initialize Plus_head structure (cidx)
 *
 * \param Plus pointer to Plus_head structure
 *
 * \return 1 OK
 * \return 0 on error      
 */
int dig_cidx_init(struct Plus_head *Plus)
{
    G_debug(3, "dig_cidx_init()");

    Plus->n_cidx = 0;
    Plus->a_cidx = 5;
    Plus->cidx =
	(struct Cat_index *)G_malloc(Plus->a_cidx * sizeof(struct Cat_index));
    if (!Plus->cidx)
	return 0;
    Plus->cidx_up_to_date = 0;
    return 1;
}

/* Free category index */
void dig_cidx_free(struct Plus_head *Plus)
{
    int i;
    struct Cat_index *ci;

    G_debug(2, "dig_cidx_free()");
    for (i = 0; i < Plus->n_cidx; i++) {
	ci = &(Plus->cidx[0]);
	G_free(ci->cat);
	ci->cat = NULL;
	ci->field = ci->n_cats = ci->a_cats = ci->n_types = 0;
    }
    if (Plus->cidx) {
	G_free(Plus->cidx);
	Plus->cidx = NULL;
    }
    Plus->a_cidx = 0;
    Plus->n_cidx = 0;
    Plus->cidx_up_to_date = 0;
}

/* 
 *  dig_cidx_add_cat ()
 *  add new field - cat - line record, space is allocated if necessary 
 *
 *  returns 1 OK
 *          0 on error      
 */
int
dig_cidx_add_cat(struct Plus_head *Plus, int field, int cat, int line,
		 int type)
{
    int i, si, found;
    struct Cat_index *ci;

    G_debug(3, "dig_cidx_add_cat(): field = %d cat = %d line = %d type = %d",
	    field, cat, line, type);

    /* Find field or add new */
    si = -1;
    for (i = 0; i < Plus->n_cidx; i++) {
	if (Plus->cidx[i].field == field) {
	    si = i;
	}
    }
    if (si == -1) {		/* not found add new */
	if (Plus->n_cidx == Plus->a_cidx) {
	    Plus->a_cidx += 10;
	    Plus->cidx =
		(struct Cat_index *)G_realloc(Plus->cidx,
					      Plus->a_cidx *
					      sizeof(struct Cat_index));
	    if (!Plus->cidx)
		return 0;
	}
	si = Plus->n_cidx;
	ci = &(Plus->cidx[si]);
	ci->field = field;
	ci->n_cats = ci->a_cats = 0;
	ci->cat = NULL;
	ci->n_types = 0;
	ci->offset = 0;
	Plus->n_cidx++;
    }

    /* Add new cat - line record */
    ci = &(Plus->cidx[si]);
    if (ci->n_cats == ci->a_cats) {
	ci->a_cats += 5000;
	ci->cat = G_realloc(ci->cat, ci->a_cats * 3 * sizeof(int));
    }

    ci->cat[ci->n_cats][0] = cat;
    ci->cat[ci->n_cats][1] = type;
    ci->cat[ci->n_cats][2] = line;
    ci->n_cats++;

    /* Add type */
    found = 0;
    for (i = 0; i < ci->n_types; i++) {
	if (ci->type[i][0] == type) {
	    ci->type[i][1]++;
	    found = 1;
	}
    }
    if (!found) {
	ci->type[ci->n_types][0] = type;
	ci->type[ci->n_types][1] = 1;
	ci->n_types++;
    }

    return 1;
}

/* Compare by cat */
static int cmp_cat(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (p1[0] < p2[0])
	return -1;
    if (p1[0] > p2[0])
	return 1;
    return 0;
}

/* Compare by field */
static int cmp_field(const void *pa, const void *pb)
{
    struct Cat_index *p1 = (struct Cat_index *)pa;
    struct Cat_index *p2 = (struct Cat_index *)pb;

    if (p1->field < p2->field)
	return -1;
    if (p1->field > p2->field)
	return 1;
    return 0;
}

/* 
 *  dig_cidx_add_cat_sorted ()
 *  add new field - cat - line record to sorted category index, space is allocated if necessary 
 *  
 *  returns 1 OK
 *          0 on error      
 */
int
dig_cidx_add_cat_sorted(struct Plus_head *Plus, int field, int cat, int line,
			int type)
{
    int i, si, found, position;
    struct Cat_index *ci;

    G_debug(3,
	    "dig_cidx_add_cat_sorted(): field = %d cat = %d line = %d type = %d",
	    field, cat, line, type);

    /* Find field or add new */
    si = -1;
    for (i = 0; i < Plus->n_cidx; i++) {
	if (Plus->cidx[i].field == field) {
	    si = i;
	}
    }
    if (si == -1) {		/* not found add new */
	if (Plus->n_cidx == Plus->a_cidx) {
	    Plus->a_cidx += 10;
	    Plus->cidx =
		(struct Cat_index *)G_realloc(Plus->cidx,
					      Plus->a_cidx *
					      sizeof(struct Cat_index));
	    if (!Plus->cidx)
		return 0;
	}
	si = Plus->n_cidx;
	ci = &(Plus->cidx[si]);
	ci->field = field;
	ci->n_cats = ci->a_cats = 0;
	ci->cat = NULL;
	ci->n_types = 0;
	ci->offset = 0;
	Plus->n_cidx++;
    }

    /* Add new cat - line record */
    ci = &(Plus->cidx[si]);
    if (ci->n_cats == ci->a_cats) {
	ci->a_cats += 5000;
	ci->cat = G_realloc(ci->cat, ci->a_cats * 3 * sizeof(int));
    }

    /* Find position and move on the way */
    for (position = ci->n_cats; position > 0; position--) {
	if (ci->cat[position - 1][0] < cat ||
	   (ci->cat[position - 1][0] == cat && ci->cat[position - 1][1] <= type)) {
	       break;
	}
	ci->cat[position][0] = ci->cat[position - 1][0];
	ci->cat[position][1] = ci->cat[position - 1][1];
	ci->cat[position][2] = ci->cat[position - 1][2];
    }

    G_debug(4, "position = %d", position);

    ci->cat[position][0] = cat;
    ci->cat[position][1] = type;
    ci->cat[position][2] = line;
    ci->n_cats++;

    /* Add type */
    found = 0;
    for (i = 0; i < ci->n_types; i++) {
	if (ci->type[i][0] == type) {
	    ci->type[i][1]++;
	    found = 1;
	}
    }
    if (!found) {
	ci->type[ci->n_types][0] = type;
	ci->type[ci->n_types][1] = 1;
	ci->n_types++;
    }

    /* Sort by field */
    qsort(Plus->cidx, Plus->n_cidx, sizeof(struct Cat_index), cmp_field);

    G_debug(3, "Added new category to index");

    return 1;
}

/* 
 *  dig_cidx_del_cat ()
 *  delete old field - cat - line record from _sorted_ category index
 *
 *  returns 1 OK
 *          0 on error      
 */
int
dig_cidx_del_cat(struct Plus_head *Plus, int field, int cat, int line,
		 int type)
{
    int i, position;
    struct Cat_index *ci;

    G_debug(3, "dig_cidx_del_cat(): field = %d cat = %d line = %d", field,
	    cat, line);

    /* Find field or add new */
    ci = NULL;
    for (i = 0; i < Plus->n_cidx; i++) {
	if (Plus->cidx[i].field == field) {
	    ci = &(Plus->cidx[i]);
	}
    }
    if (ci == NULL) {		/* should not happen */
	G_warning("BUG: Category index not found for field %d.", field);
	return 0;
    }

    /* Find position */
    G_debug(3, "n_cats = %d", ci->n_cats);
    for (position = 0; position < ci->n_cats; position++) {
	if (ci->cat[position][0] == cat && ci->cat[position][1] == type &&
	    ci->cat[position][2] == line) {
	    break;
	}
    }

    G_debug(4, "position = %d", position);

    if (position == ci->n_cats) {
	G_warning("BUG: Category not found in category index.");
	return 0;
    }

    /* Delete */
    for (i = position; i < ci->n_cats - 1; i++) {
	ci->cat[i][0] = ci->cat[i + 1][0];
	ci->cat[i][1] = ci->cat[i + 1][1];
	ci->cat[i][2] = ci->cat[i + 1][2];
    }

    ci->n_cats--;

    for (i = 0; i < ci->n_types; i++) {
	if (ci->type[i][0] == type) {
	    ci->type[i][1]--;
	}
    }

    G_debug(3, "Deleted from category index");
    return 1;
}

/* 
 *  dig_cidx_sort ()
 *  sort all records in cat index  
 *
 */
void dig_cidx_sort(struct Plus_head *Plus)
{
    int f;
    struct Cat_index *ci;

    G_debug(2, "dig_cidx_sort()");

    for (f = 0; f < Plus->n_cidx; f++) {
	int c, nucats = 0;

	ci = &(Plus->cidx[f]);

	/* Sort by category */
	qsort(ci->cat, ci->n_cats, 3 * sizeof(int), cmp_cat);

	/* Calculate number of unique cats */
	if (ci->n_cats > 0)
	    nucats++;
	for (c = 1; c < ci->n_cats; c++) {
	    if (ci->cat[c][0] != ci->cat[c - 1][0])
		nucats++;
	}
	ci->n_ucats = nucats;
    }

    /* Sort by field */
    qsort(Plus->cidx, Plus->n_cidx, sizeof(struct Cat_index), cmp_field);
}
