/*!
   \file lib/vector/Vlib/box.c

   \brief Vector library - bounding box

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Tests for point in box

   \param x,y,z coordinates
   \param Box boundary box

   \return 1 point is in box
   \return 0 point is not in box
 */
int Vect_point_in_box(double x, double y, double z, const struct bound_box *Box)
{

    return (x >= Box->W && x <= Box->E &&
	    y >= Box->S && y <= Box->N && 
	    z >= Box->B && z <= Box->T);
}

/*!
   \brief Tests for overlap of two boxes

   \param A boundary box A
   \param B boundary box B

   \return 1 boxes overlap
   \return 0 boxes do not overlap
 */
int Vect_box_overlap(const struct bound_box *A, const struct bound_box *B)
{

    if (A->E < B->W || A->W > B->E ||
	A->N < B->S || A->S > B->N || A->T < B->B || A->B > B->T) {
	return 0;
    }

    return 1;
}

/*!
   \brief Copy box B to box A

   \param A boundary A
   \param B boundary B

   \return 1
 */
int Vect_box_copy(struct bound_box *A, const struct bound_box *B)
{

    A->N = B->N;
    A->S = B->S;
    A->E = B->E;
    A->W = B->W;
    A->T = B->T;
    A->B = B->B;

    return 1;
}

/*!
   \brief Extend box A by box B

   \param A boundary A
   \param B boundary B

   \return 1
 */
int Vect_box_extend(struct bound_box *A, const struct bound_box *B)
{

    if (B->N > A->N)
	A->N = B->N;
    if (B->S < A->S)
	A->S = B->S;
    if (B->E > A->E)
	A->E = B->E;
    if (B->W < A->W)
	A->W = B->W;
    if (B->T > A->T)
	A->T = B->T;
    if (B->B < A->B)
	A->B = B->B;

    return 1;
}


/*!
 * \brief Clip coordinates to box, if necessary, lines extending outside of a box.
 *
 * A line represented by the coordinates <em>x, y</em> and <em>c_x, c_y</em> is clipped to
 * the window defined by <em>s</em> (south), <em>n</em> (north), <em>w</em>
 * (west), and <em>e</em> (east). Note that the following constraints must be
 * true:
 * w <e
 * s <n
 * The <em>x</em> and <em>c_x</em> are values to be compared to <em>w</em> and
 * <em>e.</em> The <em>y</em> and <em>c_y</em> are values to be compared to
 * <em>s</em> and <em>n.</em>
 * The <em>x</em> and <em>c_x</em> values returned lie between <em>w</em> and 
 * <em>e.</em> The <em>y</em> and <em>c_y</em> values returned lie between 
 * <em>s</em> and <em>n.</em>
 *
 *  \param x, y coordinates (w, e)
 *  \param c_x,c_y coordinates (s, n)
 *  \param Box boundary box
 *
 *  \return 1 if any clipping occured
 *  \return 0 otherwise
 */
int Vect_box_clip(double *x, double *y, double *c_x, double *c_y, const struct bound_box *Box)
{
    int mod;

    mod = 0;

    if (*x < Box->W) {
	if (*c_x != *x)
	    *y = *y + (Box->W - *x) / (*c_x - *x) * (*c_y - *y);
	*x = Box->W;
	mod = 1;
    }
    if (*x > Box->E) {
	if (*c_x != *x)
	    *y = *y + (Box->E - *x) / (*c_x - *x) * (*c_y - *y);
	*x = Box->E;
	mod = 1;
    }
    if (*c_x < Box->W) {
	if (*c_x != *x)
	    *c_y = *c_y + (Box->W - *c_x) / (*x - *c_x) * (*y - *c_y);
	*c_x = Box->W;
	mod = 1;
    }
    if (*c_x > Box->E) {
	if (*c_x != *x)
	    *c_y = *c_y + (Box->E - *c_x) / (*x - *c_x) * (*y - *c_y);
	*c_x = Box->E;
	mod = 1;
    }
    if (*y < Box->S) {
	if (*c_y != *y)
	    *x = *x + (Box->S - *y) / (*c_y - *y) * (*c_x - *x);
	*y = Box->S;
	mod = 1;
    }
    if (*y > Box->N) {
	if (*c_y != *y)
	    *x = *x + (Box->N - *y) / (*c_y - *y) * (*c_x - *x);
	*y = Box->N;
	mod = 1;
    }
    if (*c_y < Box->S) {
	if (*c_y != *y)
	    *c_x = *c_x + (Box->S - *c_y) / (*y - *c_y) * (*x - *c_x);
	*c_y = Box->S;
	mod = 1;
    }
    if (*c_y > Box->N) {
	if (*c_y != *y)
	    *c_x = *c_x + (Box->N - *c_y) / (*y - *c_y) * (*x - *c_x);
	*c_y = Box->N;
	mod = 1;
    }

    return (mod);
}


/*!
   \brief Get bounding box of given feature

   Vector map must be open at topological level and built with level
   >= GV_BUILD_BASE.

   \param Map vector map
   \param line feature id
   \param[out] Box bounding box

   \return 1 on success
   \return 0 line is dead
   \return -1 on error
 */
int Vect_get_line_box(const struct Map_info *Map, int line, struct bound_box *Box)
{
    struct Plus_head *Plus;
    struct P_line *Line;
    int type;
    static struct line_pnts *Points = NULL;

    Plus = (struct Plus_head *) &(Map->plus);
    if (line < 1 || line > Plus->n_lines) {
      G_warning(_("Attempt to access feature with invalid id (%d)"), line);
      return -1;
    }
    
    Line = Plus->Line[line];
    if (Line == NULL) {		/* dead */
	Box->N = Box->S = Box->E = Box->W = Box->T = Box->B = 0. / 0.;
	return 0;
    }
	
    type = Line->type;

    /* GV_LINES: retrieve box from spatial index */
    if (type & GV_LINES) {
	if (dig_find_line_box(Plus, line, Box) == 0) {
	    G_warning(_("Unable to determine bbox for feature %d"), line);
            return -1;
        }
        
	if (!Vect_is_3d(Map)) {
	    Box->T =  PORT_DOUBLE_MAX;
	    Box->B = -PORT_DOUBLE_MAX;
	}

	return 1;
    }

    /* all other: read line */
    if (Points == NULL)
	Points = Vect_new_line_struct();

    Vect_read_line(Map, Points, NULL, line);
    dig_line_box(Points, Box);

    if (!Vect_is_3d(Map)) {
	Box->T =  PORT_DOUBLE_MAX;
	Box->B = -PORT_DOUBLE_MAX;
    }

    return 1;
}


/*!
   \brief Get bounding box of area

   Vector map must be open at topological level and built with level
   >= GV_BUILD_AREAS.

   \param Map vector map
   \param area area id
   \param[out] Box bounding box

   \return 1 on success
   \return 0 area is dead
   \return -1 on error
 */
int Vect_get_area_box(const struct Map_info *Map, int area, struct bound_box *Box)
{
    struct Plus_head *Plus;
    struct P_area *Area;

    Plus = (struct Plus_head *) &(Map->plus);
    if (area < 1 || area > Plus->n_areas) {
        G_warning(_("Attempt to access area with invalid id (%d)"), area);
        return -1;
    }

    Area = Plus->Area[area];

    if (Area == NULL) {		/* dead */
	Box->N = Box->S = Box->E = Box->W = Box->T = Box->B = 0. / 0.;
	return 0;
    }

    if (dig_find_area_box(Plus, area, Box) == 0) {
        G_warning(_("Unable to determine bbox for area %d"), area);
        return -1;
    }

    if (!Vect_is_3d(Map)) {
	Box->T =  PORT_DOUBLE_MAX;
	Box->B = -PORT_DOUBLE_MAX;
    }

    return 1;
}

/*!
   \brief Get bounding box of isle

   Vector map must be open at topological level and built with level
   >= GV_BUILD_AREAS.

   \param Map vector map
   \param isle isle id
   \param[out] Box bounding box

   \return 1 on success
   \return 0 isle is dead / bounding box not found
   \return -1 on error
 */
int Vect_get_isle_box(const struct Map_info *Map, int isle, struct bound_box *Box)
{
    struct Plus_head *Plus;
    struct P_isle *Isle;

    Plus = (struct Plus_head *) &(Map->plus);

    if (isle < 1 || isle > Plus->n_isles) {
        G_warning(_("Attempt to access area with invalid id (%d)"), isle);
        return -1;
    }

    Isle = Plus->Isle[isle];

    if (Isle == NULL) {		/* dead */
	Box->N = Box->S = Box->E = Box->W = Box->T = Box->B = 0. / 0.;
	return 0;
    }

    if (dig_find_isle_box(Plus, isle, Box) == 0) {
	G_warning(_("Unable to determine bbox for isle %d"), isle);
        return -1;
    }

    if (!Vect_is_3d(Map)) {
	Box->T =  PORT_DOUBLE_MAX;
	Box->B = -PORT_DOUBLE_MAX;
    }

    return 1;
}

/*!
   \brief Get bounding box of map (all features in the map)

   \param Map vector map
   \param[out] Box bouding box

   \return 1 on success
   \return 0 on error
 */
int Vect_get_map_box(const struct Map_info *Map, struct bound_box *Box)
{
    const struct Plus_head *Plus;

    Plus = &(Map->plus);

    Box->N = Plus->box.N;
    Box->S = Plus->box.S;
    Box->E = Plus->box.E;
    Box->W = Plus->box.W;
    Box->T = Plus->box.T;
    Box->B = Plus->box.B;

    return 1;
}


/*!
   \brief Copy region window to bounding box

   \param Window region structure (raster-based)
   \param[out] Box boundary box (vector-based)

   \return 1 on success
   \return 0 on error
 */
int Vect_region_box(const struct Cell_head *Window, struct bound_box *Box)
{

    Box->N = Window->north;
    Box->S = Window->south;
    Box->E = Window->east;
    Box->W = Window->west;
    Box->T = PORT_DOUBLE_MAX;
    Box->B = -PORT_DOUBLE_MAX;

    return 1;
}
