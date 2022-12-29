/*!
   \file lib/vector/Vlib/sindex.c

   \brief Vector library - select vector features

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek, Markus Metz
 */

#include <stdlib.h>
#include <grass/vector.h>


/*!
   \brief Select lines with bounding boxes by box.

   Select lines whose boxes overlap specified box!!!  It means that
   selected line may or may not overlap the box.

   \param Map vector map
   \param Box bounding box
   \param type line type
   \param[out] list output list, must be initialized

   \return number of lines
 */
int
Vect_select_lines_by_box(struct Map_info *Map, const struct bound_box *Box,
			 int type, struct boxlist *list)
{
    int i, line, nlines, ntypes, mtype;
    struct Plus_head *plus;
    struct P_line *Line;
    static struct boxlist *LocList = NULL;

    G_debug(3, "Vect_select_lines_by_box()");
    G_debug(3, "  Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
	    Box->E, Box->W, Box->T, Box->B);
    plus = &(Map->plus);

    Vect_reset_boxlist(list);

    ntypes = mtype = 0;
    /* count the number of different primitives in Map */
    if (plus->n_plines != 0) {
	ntypes++;
	mtype |= GV_POINT;
    }
    if (plus->n_llines != 0) {
	ntypes++;
	mtype |= GV_LINE;
    }
    if (plus->n_blines != 0) {
	ntypes++;
	mtype |= GV_BOUNDARY;
    }
    if (plus->n_clines != 0) {
	ntypes++;
	mtype |= GV_CENTROID;
    }
    if (plus->n_flines != 0) {
	ntypes++;
	mtype |= GV_FACE;
    }
    if (plus->n_klines != 0) {
	ntypes++;
	mtype |= GV_KERNEL;
    }

    if (ntypes == 1) {
	/* there is only one type in Map */
	if (mtype & type)
	    return dig_select_lines(plus, Box, list);
	return 0;
    }

    if (ntypes == 0)
	/* empty vector */
	return 0;

    if (!LocList) {
	LocList = (struct boxlist *)G_malloc(sizeof(struct boxlist));
	dig_init_boxlist(LocList, 1);
    }

    nlines = dig_select_lines(plus, Box, LocList);
    G_debug(3, "  %d lines selected (all types)", nlines);

    /* Remove lines of not requested types */
    for (i = 0; i < nlines; i++) {
	line = LocList->id[i];
	if (plus->Line[line] == NULL)
	    continue;		/* Should not happen */
	Line = plus->Line[line];
	if (!(Line->type & type))
	    continue;
	dig_boxlist_add(list, line, &LocList->box[i]);
    }

    G_debug(3, "  %d lines of requested type", list->n_values);

    return list->n_values;
}


/*!
   \brief Select areas with bounding boxes by box.

   Select areas whose boxes overlap specified box!!!
   It means that selected area may or may not overlap the box.

   \param Map vector map
   \param Box bounding box
   \param[out] output list, must be initialized

   \return number of areas
 */
int
Vect_select_areas_by_box(struct Map_info *Map, const struct bound_box * Box,
			 struct boxlist *list)
{
    int i;
    static int debug_level = -1;

    if (debug_level == -1) {
	const char *dstr = G_getenv_nofatal("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    G_debug(3, "Vect_select_areas_by_box()");
    G_debug(3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
	    Box->E, Box->W, Box->T, Box->B);

    dig_select_areas(&(Map->plus), Box, list);
    G_debug(3, "  %d areas selected", list->n_values);
    /* avoid loop when not debugging */
    if (debug_level > 2) {
	for (i = 0; i < list->n_values; i++) {
	    G_debug(3, "  area = %d pointer to area structure = %p",
		    list->id[i], (void *)Map->plus.Area[list->id[i]]);
	}
    }

    return list->n_values;
}


/*!
   \brief Select isles with bounding boxes by box.

   Select isles whose boxes overlap specified box!!!
   It means that selected isle may or may not overlap the box.

   \param Map vector map
   \param Box bounding box
   \param[out] list output list, must be initialized

   \return number of isles
 */
int
Vect_select_isles_by_box(struct Map_info *Map, const struct bound_box * Box,
			 struct boxlist *list)
{
    G_debug(3, "Vect_select_isles_by_box()");
    G_debug(3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
	    Box->E, Box->W, Box->T, Box->B);

    dig_select_isles(&(Map->plus), Box, list);
    G_debug(3, "  %d isles selected", list->n_values);

    return list->n_values;
}

/*!
   \brief Select nodes by box.

   \param Map vector map
   \param Box bounding box
   \param[out] list output list, must be initialized

   \return number of nodes
 */
int
Vect_select_nodes_by_box(struct Map_info *Map, const struct bound_box * Box,
			 struct ilist *list)
{
    struct Plus_head *plus;

    G_debug(3, "Vect_select_nodes_by_box()");
    G_debug(3, "Box(N,S,E,W,T,B): %e, %e, %e, %e, %e, %e", Box->N, Box->S,
	    Box->E, Box->W, Box->T, Box->B);

    plus = &(Map->plus);

    Vect_reset_list(list);

    dig_select_nodes(plus, Box, list);
    G_debug(3, "  %d nodes selected", list->n_values);

    return list->n_values;
}

/*!
   \brief Select lines by Polygon with optional isles. 

   Polygons should be closed, i.e. first and last points must be identical.

   \param Map vector map
   \param Polygon outer ring
   \param nisles number of islands or 0
   \param Isles array of islands or NULL
   \param type line type
   \param[out] list output list, must be initialised

   \return number of lines
 */
int
Vect_select_lines_by_polygon(struct Map_info *Map, struct line_pnts *Polygon,
			     int nisles, struct line_pnts **Isles, int type,
			     struct ilist *List)
{
    int i;
    struct bound_box box;
    static struct line_pnts *LPoints = NULL;
    static struct boxlist *LocList = NULL;

    /* TODO: this function was not tested with isles */
    G_debug(3, "Vect_select_lines_by_polygon() nisles = %d", nisles);

    Vect_reset_list(List);
    if (!LPoints)
	LPoints = Vect_new_line_struct();
    if (!LocList) {
	LocList = Vect_new_boxlist(0);
    }

    /* Select first all lines by box */
    dig_line_box(Polygon, &box);
    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;
    Vect_select_lines_by_box(Map, &box, type, LocList);
    G_debug(3, "  %d lines selected by box", LocList->n_values);

    /* Check all lines if intersect the polygon */
    for (i = 0; i < LocList->n_values; i++) {
	int j, line, intersect = 0;

	line = LocList->id[i];
	/* Read line points */
	Vect_read_line(Map, LPoints, NULL, line);

	/* Check if any of line vertices is within polygon */
	for (j = 0; j < LPoints->n_points; j++) {
	    if (Vect_point_in_poly(LPoints->x[j], LPoints->y[j], Polygon) >= 1) {	/* inside polygon */
		int k, inisle = 0;

		for (k = 0; k < nisles; k++) {
		    if (Vect_point_in_poly(LPoints->x[j], LPoints->y[j], Isles[k]) >= 1) {	/* in isle */
			inisle = 1;
			break;
		    }
		}

		if (!inisle) {	/* inside polygon, outside isles -> select */
		    intersect = 1;
		    break;
		}
	    }
	}
	if (intersect) {
	    Vect_list_append(List, line);
	    continue;
	}

	/* Check intersections of the line with area/isles boundary */
	/* Outer boundary */
	if (Vect_line_check_intersection(LPoints, Polygon, 0)) {
	    Vect_list_append(List, line);
	    continue;
	}

	/* Islands */
	for (j = 0; j < nisles; j++) {
	    if (Vect_line_check_intersection(LPoints, Isles[j], 0)) {
		intersect = 1;
		break;
	    }
	}
	if (intersect) {
	    Vect_list_append(List, line);
	}
    }

    G_debug(4, "  %d lines selected by polygon", List->n_values);

    return List->n_values;
}


/*!
   \brief Select areas by Polygon with optional isles. 

   Polygons should be closed, i.e. first and last points must be identical.

   \param Map vector map
   \param Polygon outer ring
   \param nisles number of islands or 0
   \param Isles array of islands or NULL
   \param[out] list output list, must be initialised

   \return number of areas
 */
int
Vect_select_areas_by_polygon(struct Map_info *Map, struct line_pnts *Polygon,
			     int nisles, struct line_pnts **Isles,
			     struct ilist *List)
{
    int i, area;
    static struct ilist *BoundList = NULL;

    /* TODO: this function was not tested with isles */
    G_debug(3, "Vect_select_areas_by_polygon() nisles = %d", nisles);

    Vect_reset_list(List);
    if (!BoundList)
	BoundList = Vect_new_list();

    /* Select boundaries by polygon */
    Vect_select_lines_by_polygon(Map, Polygon, nisles, Isles, GV_BOUNDARY,
				 BoundList);

    /* Add areas on left/right side of selected boundaries */
    for (i = 0; i < BoundList->n_values; i++) {
	int line, left, right;

	line = BoundList->value[i];

	Vect_get_line_areas(Map, line, &left, &right);
	G_debug(4, "boundary = %d left = %d right = %d", line, left, right);

	if (left > 0) {
	    Vect_list_append(List, left);
	}
	else if (left < 0) {	/* island */
	    area = Vect_get_isle_area(Map, abs(left));
	    G_debug(4, "  left island -> area = %d", area);
	    if (area > 0)
		Vect_list_append(List, area);
	}

	if (right > 0) {
	    Vect_list_append(List, right);
	}
	else if (right < 0) {	/* island */
	    area = Vect_get_isle_area(Map, abs(right));
	    G_debug(4, "  right island -> area = %d", area);
	    if (area > 0)
		Vect_list_append(List, area);
	}
    }

    /* But the Polygon may be completely inside the area (only one), in that case 
     * we find the area by one polygon point and add it to the list */
    area = Vect_find_area(Map, Polygon->x[0], Polygon->y[0]);
    if (area > 0)
	Vect_list_append(List, area);

    G_debug(3, "  %d areas selected by polygon", List->n_values);

    return List->n_values;
}
