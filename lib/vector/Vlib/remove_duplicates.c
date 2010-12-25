/*!
   \file lib/vector/Vlib/remove_duplicates.c

   \brief Vector library - clean geometry (remove duplicates)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <grass/config.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Remove duplicate features from vector map.

   Remove duplicate lines of given types from vector map. Duplicate
   lines may be optionally written to error map. Input map must be
   opened on level 2 for update. Categories are merged.

   \param[in,out] Map vector map where duplicate lines will be deleted
   \param type type of line to be delete
   \param[out] Err vector map where duplicate lines will be written or NULL

   \return void
 */
void
Vect_remove_duplicates(struct Map_info *Map, int type, struct Map_info *Err)
{
    struct line_pnts *APoints, *BPoints;
    struct line_cats *ACats, *BCats, *Cats;
    int i, j, c, atype, btype, bline;
    int nlines, nbcats_orig;
    struct bound_box ABox;
    struct ilist *List;
    int ndupl;


    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_list();

    nlines = Vect_get_num_lines(Map);

    G_debug(1, "nlines =  %d", nlines);
    /* Go through all lines in vector, for each select lines which overlap MBR of
     *  this line and check if some of them is identical. If someone is identical
     *  remove current line. (In each step just one line is deleted)
     */

    ndupl = 0;

    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 1);
	if (!Vect_line_alive(Map, i))
	    continue;

	atype = Vect_read_line(Map, APoints, ACats, i);
	if (!(atype & type))
	    continue;

	Vect_line_box(APoints, &ABox);
	Vect_select_lines_by_box(Map, &ABox, type, List);
	G_debug(3, "  %d lines selected by box", List->n_values);

	for (j = 0; j < List->n_values; j++) {
	    bline = List->value[j];
	    G_debug(3, "  j = %d bline = %d", j, bline);
	    if (i == bline)
		continue;

	    btype = Vect_read_line(Map, BPoints, BCats, bline);

	    /* check for duplicates */
	    if (!Vect_line_check_duplicate(APoints, BPoints, Vect_is_3d(Map)))
		continue;

	    /* Lines area identical -> remove current */
	    if (Err) {
		Vect_write_line(Err, atype, APoints, ACats);
	    }

	    Vect_delete_line(Map, i);

	    /* Merge categories */
	    nbcats_orig = BCats->n_cats;

	    for (c = 0; c < ACats->n_cats; c++)
		Vect_cat_set(BCats, ACats->field[c], ACats->cat[c]);

	    if (BCats->n_cats > nbcats_orig) {
		G_debug(4, "cats merged: n_cats %d -> %d", nbcats_orig,
			BCats->n_cats);
		Vect_rewrite_line(Map, bline, btype, BPoints, BCats);
	    }

	    ndupl++;

	    break;		/* line was deleted -> take the next one */
	}
	nlines = Vect_get_num_lines(Map);	/* For future when lines with cats will be rewritten */
	G_debug(3, "nlines =  %d\n", nlines);
    }
    G_verbose_message("Removed duplicates: %d", ndupl);
}

/*!
   \brief Check for duplicate lines

   \param APoints first line geometry
   \param BPoints second line geometry

   \return 1 duplicate
   \return 0 not duplicate
 */
int Vect_line_check_duplicate(const struct line_pnts *APoints,
			      const struct line_pnts *BPoints, int with_z)
{
    int k;
    int npoints;
    int forw, backw;

    if (APoints->n_points != BPoints->n_points)
	return 0;

    npoints = APoints->n_points;

    /* Forward */
    forw = 1;
    for (k = 0; k < APoints->n_points; k++) {
	if (APoints->x[k] != BPoints->x[k] ||
	    APoints->y[k] != BPoints->y[k] ||
	    (with_z && APoints->z[k] != BPoints->z[k])) {
	    forw = 0;
	    break;
	}
    }

    /* Backward */
    backw = 1;
    for (k = 0; k < APoints->n_points; k++) {
	if (APoints->x[k] != BPoints->x[npoints - k - 1] ||
	    APoints->y[k] != BPoints->y[npoints - k - 1] ||
	    (with_z && APoints->z[k] != BPoints->z[npoints - k - 1])) {
	    backw = 0;
	    break;
	}
    }

    if (!forw && !backw)
	return 0;

    return 1;
}
