/*!
   \file lib/vector/Vlib/overlay.c

   \brief Vector library - overlays

   Higher level functions for reading/writing/manipulating vectors.

   This is file is just example and starting point for writing overlay
   functions!!!

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
*/

#include <string.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int Vect_overlay_and(struct Map_info *, int, struct ilist *, struct ilist *,
		     struct Map_info *, int, struct ilist *, struct ilist *,
		     struct Map_info *);

/*!
   \brief Get operator code from string

   \param str operator code string

   \return operator code
   \return -1 on error
 */
int Vect_overlay_str_to_operator(const char *str)
{

    if (strcmp(str, GV_ON_AND) == 0)
	return GV_O_AND;
    else if (strcmp(str, GV_ON_OVERLAP) == 0)
	return GV_O_OVERLAP;

    return -1;
}

/*!
   \brief Overlay 2 vector maps and create new one

   \param AMap vector map A
   \param atype feature type for A
   \param AList unused ?
   \param AAList unused ?
   \param BMap vector map B
   \param btype feature type for B
   \param BList unused ?
   \param BAList unused ?
   \param operator operator code
   \param[out] OMap output vector map

   \return 0 on success
 */
int Vect_overlay(struct Map_info *AMap, int atype, struct ilist *AList, struct ilist *AAList,	/* map A */
		 struct Map_info *BMap, int btype, struct ilist *BList, struct ilist *BAList,	/* map B */
		 int operator, struct Map_info *OMap)
{				/* output map */
    switch (operator) {
    case GV_O_AND:
	Vect_overlay_and(AMap, atype, AList, AAList, BMap, btype, BList,
			 BAList, OMap);
	break;
    default:
	G_fatal_error("Vect_overlay(): %s", _("unknown operator"));
    }

    return 0;
}

/*!
   \brief Overlay 2 vector maps with AND.

   AND supports:point line area
   point  +     -    +
   line   -     -    -
   area   +     -    -

   \param AMap vector map A
   \param atype feature type for A
   \param AList unused ?
   \param AAList unused ?
   \param BMap vector map B
   \param btype feature type for B
   \param BList unused ?
   \param BAList unused ?
   \param OMap output vector map

   \return 1 on success
   \return 0 on error
 */
int
Vect_overlay_and(struct Map_info *AMap, int atype, struct ilist *AList,
		 struct ilist *AAList, struct Map_info *BMap, int btype,
		 struct ilist *BList, struct ilist *BAList,
		 struct Map_info *OMap)
{
    int i, j, k, node, line, altype, bltype, oltype, area, centr;
    struct line_pnts *Points;
    struct line_cats *ACats, *BCats, *OCats;
    struct ilist *AOList, *BOList;
    struct boxlist *boxlist;
    struct bound_box box;

    /* TODO: support Lists */

    Points = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    OCats = Vect_new_cats_struct();
    AOList = Vect_new_list();
    BOList = Vect_new_list();
    boxlist = Vect_new_boxlist(0);

    /* TODO: support all types; at present only point x point, area x point and point x area supported  */
    if ((atype & GV_LINES) || (btype & GV_LINES))
	G_warning(_("Overlay: line/boundary types not supported by AND operator"));

    if ((atype & GV_AREA) && (btype & GV_AREA))
	G_warning(_("Overlay: area x area types not supported by AND operator"));

    /* TODO: more points in one node in one map */

    /* point x point: select all points with identical coordinates in both maps */
    if ((atype & GV_POINTS) && (btype & GV_POINTS)) {	/* both points and centroids */
	G_debug(3, "overlay: AND: point x point");
	for (i = 1; i <= Vect_get_num_lines(AMap); i++) {
	    altype = Vect_read_line(AMap, Points, ACats, i);
	    if (!(altype & GV_POINTS))
		continue;

	    box.E = box.W = Points->x[0];
	    box.N = box.S = Points->y[0];
	    box.T = box.B = Points->z[0];
	    Vect_select_lines_by_box(BMap, &box, GV_POINTS, boxlist);

	    Vect_reset_cats(OCats);

	    for (j = 0; j < boxlist->n_values; j++) {
		line = boxlist->id[j];
		bltype = Vect_read_line(BMap, NULL, BCats, line);
		if (!(bltype & GV_POINTS))
		    continue;

		/* Identical points found -> write out */
		/* TODO: do something if fields in ACats and BCats are identical */
		for (k = 0; k < ACats->n_cats; k++)
		    Vect_cat_set(OCats, ACats->field[k], ACats->cat[k]);

		for (k = 0; k < BCats->n_cats; k++)
		    Vect_cat_set(OCats, BCats->field[k], BCats->cat[k]);

		/* TODO: what to do if one type is GV_POINT and second GV_CENTROID */
		oltype = altype;
		Vect_write_line(OMap, oltype, Points, OCats);
		Vect_list_append(AOList, i);	/* add to list of written lines */
		Vect_list_append(BOList, line);
		break;
	    }
	}
    }

    /* TODO: check only labeled areas */
    /* point x area: select points from A in areas in B */
    if ((atype & GV_POINTS) && (btype & GV_AREA)) {	/* both points and centroids */
	G_debug(3, "overlay: AND: point x area");

	for (i = 1; i <= Vect_get_num_lines(AMap); i++) {
	    altype = Vect_read_line(AMap, Points, ACats, i);
	    if (!(altype & GV_POINTS))
		continue;

	    area = Vect_find_area(BMap, Points->x[0], Points->y[0]);
	    if (area == 0)
		continue;

	    Vect_reset_cats(OCats);

	    /* TODO: do something if fields in ACats and BCats are identical */
	    for (k = 0; k < ACats->n_cats; k++)
		Vect_cat_set(OCats, ACats->field[k], ACats->cat[k]);

	    centr = Vect_get_area_centroid(BMap, area);
	    if (centr > 0) {
		bltype = Vect_read_line(BMap, NULL, BCats, centr);
		for (k = 0; k < BCats->n_cats; k++)
		    Vect_cat_set(OCats, BCats->field[k], BCats->cat[k]);
	    }

	    /* Check if not yet written */
	    if (!(Vect_val_in_list(AOList, i))) {
		Vect_write_line(OMap, altype, Points, OCats);
		Vect_list_append(AOList, i);
	    }

	}
    }
    /* area x point: select points from B in areas in A */
    if ((btype & GV_POINTS) && (atype & GV_AREA)) {	/* both points and centroids */
	G_debug(3, "overlay: AND: area x point");

	for (i = 1; i <= Vect_get_num_lines(BMap); i++) {
	    bltype = Vect_read_line(BMap, Points, BCats, i);
	    if (!(bltype & GV_POINTS))
		continue;

	    area = Vect_find_area(AMap, Points->x[0], Points->y[0]);
	    if (area == 0)
		continue;

	    Vect_reset_cats(OCats);

	    /* TODO: do something if fields in ACats and BCats are identical */
	    for (k = 0; k < BCats->n_cats; k++)
		Vect_cat_set(OCats, BCats->field[k], BCats->cat[k]);

	    centr = Vect_get_area_centroid(AMap, area);
	    if (centr > 0) {
		altype = Vect_read_line(AMap, NULL, ACats, centr);
		for (k = 0; k < ACats->n_cats; k++)
		    Vect_cat_set(OCats, ACats->field[k], ACats->cat[k]);
	    }

	    /* Check if not yet written */
	    if (!(Vect_val_in_list(BOList, i))) {
		Vect_write_line(OMap, bltype, Points, OCats);
		Vect_list_append(BOList, i);
	    }

	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(ACats);
    Vect_destroy_cats_struct(BCats);
    Vect_destroy_cats_struct(OCats);
    Vect_destroy_list(AOList);
    Vect_destroy_list(BOList);
    Vect_destroy_boxlist(boxlist);

    return 0;
}
