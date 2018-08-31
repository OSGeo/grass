/*!
 * \file lib/vector/Vlib/find.c
 *
 * \brief Vector library - Find nearest vector feature.
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.

 * \author Original author CERL, probably Dave Gerdes or Mike
 * Higgins.
 * \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */
#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>

#ifndef HUGE_VAL
#define HUGE_VAL 9999999999999.0
#endif


/* for qsort */

typedef struct {
    int i;
    double size;
    struct bound_box box;
} BOX_SIZE;

static int sort_by_size(const void *a, const void *b)
{
    BOX_SIZE *as = (BOX_SIZE *)a;
    BOX_SIZE *bs = (BOX_SIZE *)b;
    
    if (as->size < bs->size)
	return -1;

    return (as->size > bs->size);
}


/*!
 * \brief Find the nearest node.
 *
 * \param Map vector map
 * \param ux,uy,uz point coordinates 
 * \param maxdist max distance from the line
 * \param with_z 3D (WITH_Z, WITHOUT_Z)
 *
 * \return number of nearest node
 * \return 0 if not found
 */
int
Vect_find_node(struct Map_info *Map,
	       double ux, double uy, double uz, double maxdist, int with_z)
{
    int i, nnodes, node;
    struct bound_box box;
    struct ilist *NList;
    double x, y, z;
    double cur_dist, dist;

    G_debug(3, "Vect_find_node() for %f %f %f maxdist = %f", ux, uy, uz,
	    maxdist);
    NList = Vect_new_list();

    /* Select all nodes in box */
    box.N = uy + maxdist;
    box.S = uy - maxdist;
    box.E = ux + maxdist;
    box.W = ux - maxdist;
    if (with_z) {
	box.T = uz + maxdist;
	box.B = uz - maxdist;
    }
    else {
	box.T = HUGE_VAL;
	box.B = -HUGE_VAL;
    }

    nnodes = Vect_select_nodes_by_box(Map, &box, NList);
    G_debug(3, " %d nodes in box", nnodes);

    if (nnodes == 0)
	return 0;

    /* find nearest */
    cur_dist = PORT_DOUBLE_MAX;
    node = 0;
    for (i = 0; i < nnodes; i++) {
	Vect_get_node_coor(Map, NList->value[i], &x, &y, &z);
	dist = Vect_points_distance(ux, uy, uz, x, y, z, with_z);
	if (dist < cur_dist) {
	    cur_dist = dist;
	    node = i;
	}
    }
    G_debug(3, "  nearest node %d in distance %f", NList->value[node],
	    cur_dist);

    /* Check if in max distance */
    if (cur_dist <= maxdist)
	return (NList->value[node]);
    else
	return 0;
}

/*!
 * \brief Find the nearest line.
 *
 * \param map vector map
 * \param ux,uy,uz points coordinates
 * \param type feature type (GV_LINE, GV_POINT, GV_BOUNDARY or GV_CENTROID)
 * if only want to search certain types of lines or -1 if search all lines
 * \param maxdist max distance from the line
 * \param with_z 3D (WITH_Z, WITHOUT_Z)
 * \param exclude if > 0 number of line which should be excluded from selection.
 * May be useful if we need line nearest to other one. 
 *
 * \return number of nearest line
 * \return 0 if not found
 *
 */
int
Vect_find_line(struct Map_info *map,
	       double ux, double uy, double uz,
	       int type, double maxdist, int with_z, int exclude)
{
    int line;
    struct ilist *exclude_list;

    exclude_list = Vect_new_list();

    Vect_list_append(exclude_list, exclude);

    line = Vect_find_line_list(map, ux, uy, uz,
			       type, maxdist, with_z, exclude_list, NULL);

    Vect_destroy_list(exclude_list);

    return line;
}

/*!
 * \brief Find the nearest line(s).
 *
 * \param map vector map
 * \param ux,uy,uz points coordinates
 * \param type feature type (GV_LINE, GV_POINT, GV_BOUNDARY or GV_CENTROID)
 * if only want to search certain types of lines or -1 if search all lines
 * \param maxdist max distance from the line
 * \param with_z 3D (WITH_Z, WITHOUT_Z)
 * \param exclude list of lines which should be excluded from selection
 * \param found list of found lines (or NULL)
 *
 * \return number of nearest line
 * \return 0 if not found
 */
int
Vect_find_line_list(struct Map_info *map,
		    double ux, double uy, double uz,
		    int type, double maxdist, int with_z,
		    const struct ilist *exclude, struct ilist *found)
{
    int choice;
    double new_dist;
    double cur_dist;
    int gotone;
    int i, line;
    static struct line_pnts *Points;
    static int first_time = 1;
    struct bound_box box;
    struct boxlist *List;

    G_debug(3, "Vect_find_line_list() for %f %f %f type = %d maxdist = %f",
	    ux, uy, uz, type, maxdist);

    if (first_time) {
	Points = Vect_new_line_struct();
	first_time = 0;
    }

    gotone = 0;
    choice = 0;
    cur_dist = HUGE_VAL;

    box.N = uy + maxdist;
    box.S = uy - maxdist;
    box.E = ux + maxdist;
    box.W = ux - maxdist;
    if (with_z) {
	box.T = uz + maxdist;
	box.B = uz - maxdist;
    }
    else {
	box.T = PORT_DOUBLE_MAX;
	box.B = -PORT_DOUBLE_MAX;
    }

    List = Vect_new_boxlist(0);

    if (found)
	Vect_reset_list(found);

    Vect_select_lines_by_box(map, &box, type, List);
    for (i = 0; i < List->n_values; i++) {
	line = List->id[i];
	if (Vect_val_in_list(exclude, line)) {
	    G_debug(3, " line = %d exclude", line);
	    continue;
	}

	/* No more needed */
	/*
	   Line = Plus->Line[line];       
	   if ( Line == NULL ) continue;
	   if ( !(type & Line->type) ) continue; 
	 */

	Vect_read_line(map, Points, NULL, line);

	Vect_line_distance(Points, ux, uy, uz, with_z, NULL, NULL, NULL,
			   &new_dist, NULL, NULL);
	G_debug(3, " line = %d distance = %f", line, new_dist);

	if (found && new_dist <= maxdist) {
	    Vect_list_append(found, line);
	}

	if ((++gotone == 1) || (new_dist <= cur_dist)) {
	    if (new_dist == cur_dist) {
		/* TODO */
		/* choice = dig_center_check (map->Line, choice, a, ux, uy); */
		continue;
	    }

	    choice = line;
	    cur_dist = new_dist;
	}
    }

    G_debug(3, "min distance found = %f", cur_dist);
    if (cur_dist > maxdist)
	choice = 0;

    Vect_destroy_boxlist(List);

    return (choice);
}

/*!
 * \brief Find the nearest area
 *
 * \param Map vector map
 * \param x,y point coordinates
 *
 * \return area number
 * \return 0 if not found
 */
int Vect_find_area(struct Map_info *Map, double x, double y)
{
    int i, j, ret, area, isle;
    struct bound_box box;
    static struct boxlist *List = NULL;
    static BOX_SIZE *size_list;
    static int alloc_size_list = 0;
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_find_area() x = %f y = %f", x, y);

    if (!List) {
	List = Vect_new_boxlist(1);
	alloc_size_list = 10;
	size_list = G_malloc(alloc_size_list * sizeof(BOX_SIZE));
    }

    Plus = &(Map->plus);

    /* select areas by box */
    box.E = x;
    box.W = x;
    box.N = y;
    box.S = y;
    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;
    Vect_select_areas_by_box(Map, &box, List);
    G_debug(3, "  %d areas selected by box", List->n_values);

    /* sort areas by bbox size
     * get the smallest area that contains the point
     * using the bbox size is working because if 2 areas both contain
     * the point, one of these areas must be inside the other area
     * which means that the bbox of the outer area must be larger than
     * the bbox of the inner area, and equal bbox sizes are not possible */

    if (alloc_size_list < List->n_values) {
	alloc_size_list = List->n_values;
	size_list = G_realloc(size_list, alloc_size_list * sizeof(BOX_SIZE));
    }

    for (i = 0; i < List->n_values; i++) {
	size_list[i].i = List->id[i];
	box = List->box[i];
	size_list[i].box = List->box[i];
	size_list[i].size = (box.N - box.S) * (box.E - box.W);
    }
    
    if (List->n_values == 2) {
	/* simple swap */
	if (size_list[1].size < size_list[0].size) {
	    size_list[0].i = List->id[1];
	    size_list[1].i = List->id[0];
	    size_list[0].box = List->box[1];
	    size_list[1].box = List->box[0];
	}
    }
    else if (List->n_values > 2)
	qsort(size_list, List->n_values, sizeof(BOX_SIZE), sort_by_size);

    for (i = 0; i < List->n_values; i++) {
	area = size_list[i].i;
	/* outer ring */
	ret = Vect_point_in_area_outer_ring(x, y, Map, area, &size_list[i].box);

	G_debug(3, "    area = %d Vect_point_in_area_outer_ring() = %d", area, ret);

	if (ret >= 1) {
	    /* check if in islands */
	    Area = Plus->Area[area];
	    for (j = 0; j < Area->n_isles; j++) {
		isle = Area->isles[j];
		Vect_get_isle_box(Map, isle, &box);
		ret = Vect_point_in_island(x, y, Map, isle, &box);

		G_debug(3, "    area = %d Vect_point_in_island() = %d", area, ret);

		if (ret >= 1) {
		    /* point is not in area
		     * point is also not in any inner area, those have 
		     * been tested before (sorted list) 
		     * -> area inside island could not be built */
		    return 0;
		}
	    }
	    return (area);
	}
    }

    return 0;
}

/*!
 * \brief Find the nearest island
 * 
 * \param Map vector map
 * \param x,y points coordinates
 *
 * \return island number,
 * \return 0 if not found
 */
int Vect_find_island(struct Map_info *Map, double x, double y)
{
    int i, ret, island, current, current_size, size;
    static int first = 1;
    struct bound_box box;
    static struct boxlist *List;
    static struct line_pnts *Points;

    /* TODO: sync to Vect_find_area() */

    G_debug(3, "Vect_find_island() x = %f y = %f", x, y);

    if (first) {
	List = Vect_new_boxlist(1);
	Points = Vect_new_line_struct();
	first = 0;
    }

    /* select islands by box */
    box.E = x;
    box.W = x;
    box.N = y;
    box.S = y;
    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;
    Vect_select_isles_by_box(Map, &box, List);
    G_debug(3, "  %d islands selected by box", List->n_values);

    current_size = -1;
    current = 0;
    for (i = 0; i < List->n_values; i++) {
	island = List->id[i];
	ret = Vect_point_in_island(x, y, Map, island, &List->box[i]);

	if (ret >= 1) {		/* inside */
	    if (current > 0) {	/* not first */
		if (current_size == -1) {	/* second */
		    G_begin_polygon_area_calculations();
		    Vect_get_isle_points(Map, current, Points);
		    current_size =
			G_area_of_polygon(Points->x, Points->y,
					  Points->n_points);
		}

		Vect_get_isle_points(Map, island, Points);
		size =
		    G_area_of_polygon(Points->x, Points->y, Points->n_points);

		if (size < current_size) {
		    current = island;
		    current_size = size;
		}
	    }
	    else {		/* first */
		current = island;
	    }
	}
    }

    return current;
}
