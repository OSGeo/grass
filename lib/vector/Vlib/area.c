/*!
   \file lib/vector/Vlib/area.c

   \brief Vector library - area-related functions 

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009, 2011-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"
#endif

#include "local_proto.h"

/*!
   \brief Returns polygon array of points (outer ring) of given area

   \param Map pointer to Map_info structure
   \param area area id
   \param[out] BPoints points array

   \return number of points
   \return -1 on error
 */
int Vect_get_area_points(const struct Map_info *Map,
			 int area, struct line_pnts *BPoints)
{
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_get_area_points(): area = %d", area);
    Vect_reset_line(BPoints);

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL) {		/* dead area */
	G_warning(_("Attempt to read points of nonexistent area"));
	return -1;		/* error, because we should not read dead areas */
    }

    G_debug(3, "  n_lines = %d", Area->n_lines);
    return Vect__get_area_points(Map, Area->lines, Area->n_lines, BPoints);
}

/*!
   \brief Returns polygon array of points for given isle

   \param Map pointer to Map_info structure
   \param isle island id
   \param[out] BPoints points array

   \return number of points
   \return -1 on error
 */
int Vect_get_isle_points(const struct Map_info *Map,
                         int isle, struct line_pnts *BPoints)
{
    const struct Plus_head *Plus;
    struct P_isle *Isle;

    G_debug(3, "Vect_get_isle_points(): isle = %d", isle);
    Vect_reset_line(BPoints);

    Plus = &(Map->plus);
    Isle = Plus->Isle[isle];
    
    if (Isle == NULL) {		/* dead isle */
	G_warning(_("Attempt to read points of nonexistent isle"));
	return -1;		/* error, because we should not read dead isles */
    }

    G_debug(3, "  n_lines = %d", Isle->n_lines);

    if (Map->format == GV_FORMAT_POSTGIS &&
        Map->fInfo.pg.toposchema_name &&
        Map->fInfo.pg.cache.ctype != CACHE_MAP) {
#ifdef HAVE_POSTGRES
        /* PostGIS Topology */
        return Vect__get_area_points_pg(Map, Isle->lines, Isle->n_lines, BPoints);
#else
        G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif
    }
    /* native format */
    return Vect__get_area_points_nat(Map, Isle->lines, Isle->n_lines, BPoints);
}

/*!
   \brief Returns centroid id for given area

   \param Map pointer to Map_info structure
   \param area area id

   \return centroid id of area
   \return 0 if no centroid found
 */
int Vect_get_area_centroid(const struct Map_info *Map, int area)
{
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_get_area_centroid(): area = %d", area);

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL)
	G_fatal_error(_("Attempt to read topo for dead area (%d)"), area);

    return (Area->centroid);
}

/*!
   \brief Creates list of boundaries for given area

   Note that ids in <b>List</b> can be negative. The sign indicates in
   which direction the boundary should be read (negative for
   backward).
   
   \param Map pointer to Map_info structure
   \param area area id
   \param[out] List pointer to list of boundaries

   \return number of boundaries
*/
int Vect_get_area_boundaries(const struct Map_info *Map, int area, struct ilist *List)
{
    int i, line;
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_get_area_boundaries(): area = %d", area);

    Vect_reset_list(List);

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL)
	G_fatal_error(_("Attempt to read topo for dead area (%d)"), area);

    for (i = 0; i < Area->n_lines; i++) {
	line = Area->lines[i];
	Vect_list_append(List, line);
    }

    return (List->n_values);
}

/*!
   \brief Creates list of boundaries for given isle

   Note that ids in <b>List</b> can be negative. The sign indicates in
   which direction the boundary should be read (negative for forward).

   \param Map pointer to Map_info structur
   \param isle island number
   \param[out] List pointer to list where boundaries are stored

   \return number of boundaries
 */
int Vect_get_isle_boundaries(const struct Map_info *Map, int isle, struct ilist *List)
{
    int i, line;
    const struct Plus_head *Plus;
    struct P_isle *Isle;

    G_debug(3, "Vect_get_isle_boundaries(): isle = %d", isle);

    Vect_reset_list(List);

    Plus = &(Map->plus);
    Isle = Plus->Isle[isle];

    if (Isle == NULL)
	G_fatal_error(_("Attempt to read topo for dead isle (%d)"), isle);

    for (i = 0; i < Isle->n_lines; i++) {
	line = Isle->lines[i];
	Vect_list_append(List, line);
    }

    return (List->n_values);
}

/*!
   \brief Returns number of isles for given area

   \param Map pointer to Map_info structure
   \param area area id

   \return number of isles for area
   \return 0 if area not found
 */
int Vect_get_area_num_isles(const struct Map_info *Map, int area)
{
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_get_area_num_isles(): area = %d", area);

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL)
	G_fatal_error(_("Attempt to read topo for dead area (%d)"), area);

    G_debug(3, "  n_isles = %d", Area->n_isles);

    return (Area->n_isles);

}

/*!
   \brief Returns isle id for area

   \param Map pointer to Map_info structure
   \param area area id
   \param isle isle index (0 .. nisles - 1)

   \return isle id
   \return 0 if no isle found
 */
int Vect_get_area_isle(const struct Map_info *Map, int area, int isle)
{
    const struct Plus_head *Plus;
    struct P_area *Area;

    G_debug(3, "Vect_get_area_isle(): area = %d isle = %d", area, isle);

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    if (Area == NULL)
	G_fatal_error(_("Attempt to read topo for dead area (%d)"), area);

    G_debug(3, "  -> isle = %d", Area->isles[isle]);

    return (Area->isles[isle]);
}

/*!
   \brief Returns area id for isle

   \param Map vector
   \param isle isle number (0 .. nisles - 1)

   \return area id
   \return 0 area not found
 */
int Vect_get_isle_area(const struct Map_info *Map, int isle)
{
    const struct Plus_head *Plus;
    struct P_isle *Isle;

    G_debug(3, "Vect_get_isle_area(): isle = %d", isle);

    Plus = &(Map->plus);
    Isle = Plus->Isle[isle];

    if (Isle == NULL)
	G_fatal_error(_("Attempt to read topo for dead isle (%d)"), isle);

    G_debug(3, "  -> area = %d", Isle->area);

    return (Isle->area);
}


/*!
   \brief Calculate area perimeter

   \param Points list of points defining area boundary

   \return area perimeter
 */
double Vect_area_perimeter(const struct line_pnts *Points)
{
    return Vect_line_length(Points);
}


/*!
   \brief Check if point is in area

   \param x,y point coordinates
   \param Map pointer to Map_info structure
   \param area area id
   \param box area bounding box

   \return 1 if point is in area
   \return 0 if not 
 */
int Vect_point_in_area(double x, double y, const struct Map_info *Map,
                       int area, struct bound_box *box)
{
    int i, isle;
    const struct Plus_head *Plus;
    struct P_area *Area;
    struct bound_box ibox;
    int poly;

    Plus = &(Map->plus);
    Area = Plus->Area[area];
    if (Area == NULL)
	return 0;

    poly = Vect_point_in_area_outer_ring(x, y, Map, area, box);
    if (poly == 0)
	return 0;		/* includes area boundary (poly == 2), OK? */

    /* check if in islands */
    for (i = 0; i < Area->n_isles; i++) {
	isle = Area->isles[i];
	Vect_get_isle_box(Map, isle, &ibox);
	poly = Vect_point_in_island(x, y, Map, isle, &ibox);
	if (poly >= 1)
	    return 0;		/* excludes island boundary (poly == 2), OK? */
    }

    return 1;
}

/*!
   \brief Returns area of area without areas of isles

   \param Map pointer to Map_info structure
   \param area area id

   \return area of area without areas of isles
 */
double Vect_get_area_area(const struct Map_info *Map, int area)
{
    const struct Plus_head *Plus;
    struct P_area *Area;
    struct line_pnts *Points;
    double size;
    int i;
    static int first_time = 1;

    G_debug(3, "Vect_get_area_area(): area = %d", area);

    if (first_time == 1) {
	G_begin_polygon_area_calculations();
	first_time = 0;
    }

    Points = Vect_new_line_struct();
    Plus = &(Map->plus);
    Area = Plus->Area[area];

    Vect_get_area_points(Map, area, Points);
    size = G_area_of_polygon(Points->x, Points->y, Points->n_points);

    /* substructing island areas */
    for (i = 0; i < Area->n_isles; i++) {
	Vect_get_isle_points(Map, Area->isles[i], Points);
	size -= G_area_of_polygon(Points->x, Points->y, Points->n_points);
    }

    Vect_destroy_line_struct(Points);

    G_debug(3, "    area = %f", size);

    return (size);
}

/*!
   \brief Get area categories

   \param Map pointer to Map_info structure
   \param area area id
   \param[out] Cats list of categories

   \return 0 centroid found (but may be without categories)
   \return 1 no centroid found
 */
int Vect_get_area_cats(const struct Map_info *Map, int area, struct line_cats *Cats)
{
    int centroid;

    Vect_reset_cats(Cats);

    centroid = Vect_get_area_centroid(Map, area);
    if (centroid > 0) {
	Vect_read_line(Map, NULL, Cats, centroid);
    }
    else {
	return 1;		/* no centroid */
    }


    return 0;
}

/*!
   \brief Find FIRST category of given field and area

   \param Map pointer to Map_info structure
   \param area area id
   \param field layer number

   \return first found category of given field
   \return -1 no centroid or no category found
 */
int Vect_get_area_cat(const struct Map_info *Map, int area, int field)
{
    int i;
    static struct line_cats *Cats = NULL;

    if (!Cats)
	Cats = Vect_new_cats_struct();
    else
	Vect_reset_cats(Cats);

    if (Vect_get_area_cats(Map, area, Cats) == 1 || Cats->n_cats == 0) {
	return -1;
    }

    for (i = 0; i < Cats->n_cats; i++) {
	if (Cats->field[i] == field) {
	    return Cats->cat[i];
	}
    }

    return -1;
}

/*!
  \brief Get area boundary points (internal use only)
  
  For PostGIS Topology calls Vect__get_area_points_pg() otherwise
  Vect__get_area_points_nat(),
  
  \param Map pointer to Map_info struct
  \param lines array of boundary lines
  \param n_lines number of lines in array
  \param[out] APoints pointer to output line_pnts struct

  \return number of points
  \return -1 on error
*/
int Vect__get_area_points(const struct Map_info *Map, const plus_t *lines, int n_lines,
                          struct line_pnts *BPoints)
{
    if (Map->format == GV_FORMAT_POSTGIS &&
        Map->fInfo.pg.toposchema_name &&
        Map->fInfo.pg.cache.ctype != CACHE_MAP) {
#ifdef HAVE_POSTGRES
        /* PostGIS Topology */
        return Vect__get_area_points_pg(Map, lines, n_lines, BPoints);
#else
        G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif
    }
    /* native format */
    return Vect__get_area_points_nat(Map, lines, n_lines, BPoints);
}


/*!
  \brief Get area boundary points (native format)
  
  Used by Vect_build_line_area() and Vect_get_area_points().
  
  \param Map pointer to Map_info struct
  \param lines array of boundary lines
  \param n_lines number of lines in array
  \param[out] APoints pointer to output line_pnts struct

  \return number of points
  \return -1 on error
*/
int Vect__get_area_points_nat(const struct Map_info *Map, const plus_t *lines, int n_lines,
                              struct line_pnts *BPoints)
{
    int i, line, aline, dir;
    static struct line_pnts *Points;
    
    if (!Points)
        Points = Vect_new_line_struct();
    
    Vect_reset_line(BPoints);
    for (i = 0; i < n_lines; i++) {
        line = lines[i];
        aline = abs(line);
        G_debug(5, "  append line(%d) = %d", i, line);
        
        if (0 > Vect_read_line(Map, Points, NULL, aline))
            return -1;
        
        dir = line > 0 ? GV_FORWARD : GV_BACKWARD;
        Vect_append_points(BPoints, Points, dir);
        BPoints->n_points--;    /* skip last point, avoids duplicates */
    }
    BPoints->n_points++;        /* close polygon */

    return BPoints->n_points;
}
