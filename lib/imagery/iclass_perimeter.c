/*!
   \file lib/imagery/iclass_perimeter.c

   \brief Imagery library - functions for wx.iclass

   Computation based on training areas for supervised classification.
   Based on i.class module (GRASS 6).

   Vector map with training areas is used to determine corresponding
   cells by computing cells on area perimeter.

   Copyright (C) 1999-2007, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author David Satnik, Central Washington University (original author)
   \author Markus Neteler <neteler itc.it> (i.class module)
   \author Bernhard Reiter <bernhard intevation.de> (i.class module)
   \author Brad Douglas <rez touchofmadness.com>(i.class module)
   \author Glynn Clements <glynn gclements.plus.com> (i.class module)
   \author Hamish Bowman <hamish_b yahoo.com> (i.class module)
   \author Jan-Oliver Wagner <jan intevation.de> (i.class module)
   \author Anna Kratochvilova <kratochanna gmail.com> (rewriting for wx.iclass)
   \author Vaclav Petras <wenzeslaus gmail.com> (rewriting for wx.iclass)
 */

#include <stdlib.h>

#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "iclass_local_proto.h"


#define extrema(x,y,z) (((x<y)&&(z<y))||((x>y)&&(z>y)))
#define non_extrema(x,y,z) (((x<y)&&(y<z))||((x>y)&&(y>z)))

/*!
   \brief Creates perimeters from vector areas of given category.

   \param Map vector map
   \param layer_name layer name (within vector map)
   \param category vector category (cat column value)
   \param[out] perimeters list of perimeters
   \param band_region region which determines perimeter cells

   \return number of areas of given cat
   \return -1 on error
 */
int vector2perimeters(struct Map_info *Map, const char *layer_name,
		      int category, IClass_perimeter_list * perimeters,
		      struct Cell_head *band_region)
{
    struct line_pnts *points;

    int nareas, nareas_cat, layer;

    int i, cat, ret;

    int j;

    G_debug(3, "iclass_vector2perimeters():layer = %s, category = %d",
	    layer_name, category);

    layer = Vect_get_field_number(Map, layer_name);
    nareas = Vect_get_num_areas(Map);
    if (nareas == 0)
	return 0;

    nareas_cat = 0;
    /* find out, how many areas have given category */
    for (i = 1; i <= nareas; i++) {
	if (!Vect_area_alive(Map, i))
	    continue;
	cat = Vect_get_area_cat(Map, i, layer);
	if (cat < 0) {
	    /* no centroid, no category */
	}
	else if (cat == category) {
	    nareas_cat++;
	}
    }
    if (nareas_cat == 0)
	return 0;

    perimeters->nperimeters = nareas_cat;
    perimeters->perimeters =
	(IClass_perimeter *) G_calloc(nareas_cat, sizeof(IClass_perimeter));

    j = 0;			/* area with cat */
    for (i = 1; i <= nareas; i++) {
	if (!Vect_area_alive(Map, i))
	    continue;
	cat = Vect_get_area_cat(Map, i, layer);
	if (cat < 0) {
	    /* no centroid, no category */
	}
	else if (cat == category) {
	    j++;

	    points = Vect_new_line_struct();	/* Vect_destroy_line_struct */
	    ret = Vect_get_area_points(Map, i, points);

	    if (ret <= 0) {
		Vect_destroy_line_struct(points);
		free_perimeters(perimeters);
		G_warning(_("Get area %d failed"), i);
		return -1;
	    }
	    if (make_perimeter
		(points, &perimeters->perimeters[j - 1], band_region) <= 0) {
		Vect_destroy_line_struct(points);
		free_perimeters(perimeters);
		G_warning(_("Perimeter computation failed"));
		return -1;
	    }
	    Vect_destroy_line_struct(points);
	}

    }

    /* Vect_close(&Map); */

    return nareas_cat;
}

/*!
   \brief Frees all perimeters in list of perimeters.

   It also frees list of perimeters itself.

   \param perimeters list of perimeters
 */
void free_perimeters(IClass_perimeter_list * perimeters)
{
    int i;

    G_debug(5, "free_perimeters()");

    for (i = 0; i < perimeters->nperimeters; i++) {
	G_free(perimeters->perimeters[i].points);
    }
    G_free(perimeters->perimeters);
}

/*!
   \brief Creates one perimeter from vector area.

   \param points list of vertices represting area
   \param[out] perimeter perimeter
   \param band_region region which determines perimeter cells

   \return 1 on success
   \return 0 on error
 */
int make_perimeter(struct line_pnts *points, IClass_perimeter * perimeter,
		   struct Cell_head *band_region)
{
    IClass_point *tmp_points;

    IClass_point *vertex_points;

    int i, first, prev, skip, next;

    int count, vertex_count;

    int np;			/* perimeter estimate */

    G_debug(5, "iclass_make_perimeter()");
    count = points->n_points;

    tmp_points = (IClass_point *) G_calloc(count, sizeof(IClass_point));	/* TODO test */

    for (i = 0; i < count; i++) {
	G_debug(5, "iclass_make_perimeter(): points: x: %f y: %f",
		points->x[i], points->y[i]);
	tmp_points[i].y = Rast_northing_to_row(points->y[i], band_region);
	tmp_points[i].x = Rast_easting_to_col(points->x[i], band_region);
    }

    /* find first edge which is not horizontal */

    first = -1;
    prev = count - 1;
    for (i = 0; i < count; prev = i++) {
	/* non absurd polygon has vertexes with different y coordinate */
	if (tmp_points[i].y != tmp_points[prev].y) {
	    first = i;
	    break;
	}
    }
    if (first < 0) {
	G_free(tmp_points);
	G_warning(_("Absurd polygon."));
	return 0;
    }

    /* copy tmp to vertex list collapsing adjacent horizontal edges */

    /* vertex_count <= count, size of vertex_points is count */
    vertex_points = (IClass_point *) G_calloc(count, sizeof(IClass_point));	/* TODO test */
    skip = 0;
    vertex_count = 0;
    i = first;			/* stmt not necssary */

    do {
	if (!skip) {
	    vertex_points[vertex_count].x = tmp_points[i].x;
	    vertex_points[vertex_count].y = tmp_points[i].y;
	    vertex_count++;
	}

	prev = i++;
	if (i >= count)
	    i = 0;
	if ((next = i + 1) >= count)
	    next = 0;

	skip = ((tmp_points[prev].y == tmp_points[i].y) &&
		(tmp_points[next].y == tmp_points[i].y));
    }
    while (i != first);

    G_free(tmp_points);

    /* count points on the perimeter */

    np = 0;
    prev = vertex_count - 1;
    for (i = 0; i < vertex_count; prev = i++) {
	np += abs(vertex_points[prev].y - vertex_points[i].y);
    }

    /* allocate perimeter list */

    perimeter->points = (IClass_point *) G_calloc(np, sizeof(IClass_point));
    if (!perimeter->points) {
	G_free(vertex_points);
	G_warning(_("Outlined area is too large."));
	return 0;
    }

    /* store the perimeter points */

    perimeter->npoints = 0;
    prev = vertex_count - 1;
    for (i = 0; i < vertex_count; prev = i++) {
	edge2perimeter(perimeter, vertex_points[prev].x,
		       vertex_points[prev].y, vertex_points[i].x,
		       vertex_points[i].y);
    }

    /*
     * now decide which verticies should be included
     *    local extrema are excluded
     *    local non-extrema are included
     *    verticies of horizontal edges which are pseudo-extrema
     *      are excluded.
     *    one vertex of horizontal edges which are pseudo-non-extrema
     *      are included.
     */

    prev = vertex_count - 1;
    i = 0;
    do {
	next = i + 1;
	if (next >= vertex_count)
	    next = 0;

	if (extrema
	    (vertex_points[prev].y, vertex_points[i].y,
	     vertex_points[next].y))
	    skip = 1;
	else if (non_extrema
		 (vertex_points[prev].y, vertex_points[i].y,
		  vertex_points[next].y))
	    skip = 0;
	else {
	    skip = 0;
	    if (++next >= vertex_count)
		next = 0;
	    if (extrema
		(vertex_points[prev].y, vertex_points[i].y,
		 vertex_points[next].y))
		skip = 1;
	}

	if (!skip)
	    perimeter_add_point(perimeter, vertex_points[i].x,
				vertex_points[i].y);

	i = next;
	prev = i - 1;
    }
    while (i != 0);

    G_free(vertex_points);

    /* sort the edge points by row and then by col */
    qsort(perimeter->points, (size_t) perimeter->npoints,
	  sizeof(IClass_point), edge_order);

    return 1;

}

/*!
   \brief Converts edge to cells.

   It rasterizes edge given by two vertices.
   Resterized points are added to perimeter.

   \param perimeter perimeter
   \param x0,y0 first edge point row and cell
   \param x1,y1 second edge point row and cell

   \return 1 on success
   \return 0 on error
 */
int edge2perimeter(IClass_perimeter * perimeter, int x0, int y0, int x1,
		   int y1)
{
    float m;

    float x;

    if (y0 == y1)
	return 0;

    x = x0;
    m = (float)(x0 - x1) / (float)(y0 - y1);

    if (y0 < y1) {
	while (++y0 < y1) {
	    x0 = (x += m) + .5;
	    perimeter_add_point(perimeter, x0, y0);
	}
    }
    else {
	while (--y0 > y1) {
	    x0 = (x -= m) + .5;
	    perimeter_add_point(perimeter, x0, y0);
	}
    }

    return 1;
}

/*!
   \brief Adds point to perimeter.

   \a perimeter has to have allocated space for \c points memeber.

   \param perimeter perimeter
   \param x,y point row and cell
 */
void perimeter_add_point(IClass_perimeter * perimeter, int x, int y)
{
    int n;

    G_debug(5, "perimeter_add_point(): x: %d, y: %d", x, y);

    n = perimeter->npoints++;
    perimeter->points[n].x = x;
    perimeter->points[n].y = y;
}

/*!
   \brief Determines points order during sorting.

   \param aa first IClass_point
   \param bb second IClass_point
 */
int edge_order(const void *aa, const void *bb)
{
    const IClass_point *a = aa;

    const IClass_point *b = bb;

    if (a->y < b->y)
	return -1;
    if (a->y > b->y)
	return 1;

    if (a->x < b->x)
	return -1;
    if (a->x > b->x)
	return 1;

    return 0;
}
