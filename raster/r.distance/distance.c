
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Sort/reverse sort by distance by Huidae Cho
 *
 * PURPOSE:      Locates the closest points between objects in two 
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include <grass/raster.h>
#include <grass/glocale.h>

#include "defs.h"

/* given two CatEdgeLists, find the cells which are closest
 * and return east,north for the tow cells and the distance between them
 */

/* this code assumes that list1 and list2 have at least one cell in each */

void
find_minimum_distance(const struct CatEdgeList *list1, const struct CatEdgeList *list2,
		      double *east1, double *north1, double *east2, double *north2,
		      double *distance, const struct Cell_head *region, int overlap,
		      const char *name1, const char *name2)
{
    int i1, i2;
    double dist;
    double e1, n1, e2, n2;
    extern double G_distance();
    extern double Rast_row_to_northing();
    extern double Rast_col_to_easting();
    int zerro_row, zerro_col;

    int nulldistance = 0;


    if (overlap)
	nulldistance = null_distance(name1, name2, &zerro_row, &zerro_col);

    for (i1 = 0; i1 < list1->ncells; i1++) {
	e1 = Rast_col_to_easting(list1->col[i1] + 0.5, region);
	n1 = Rast_row_to_northing(list1->row[i1] + 0.5, region);

	for (i2 = 0; i2 < list2->ncells; i2++) {
	    if (!nulldistance) {
		e2 = Rast_col_to_easting(list2->col[i2] + 0.5, region);
		n2 = Rast_row_to_northing(list2->row[i2] + 0.5, region);
		dist = G_distance(e1, n1, e2, n2);
	    }
	    else {
		e2 = e1 = Rast_col_to_easting(zerro_col + 0.5, region);
		n2 = n1 = Rast_row_to_northing(zerro_row + 0.5, region);
		dist = 0.;
	    }

	    if ((i1 == 0 && i2 == 0) || (dist < *distance)) {
		*distance = dist;
		*east1 = e1;
		*north1 = n1;
		*east2 = e2;
		*north2 = n2;
	    }
	    if (nulldistance)
		break;
	}
	if (nulldistance)
	    break;
    }
}

int null_distance(const char *name1, const char *name2, int *zerro_row, int *zerro_col)
{
    RASTER_MAP_TYPE maptype1, maptype2;
    const char *mapset;
    int mapd1, mapd2;
    void *inrast1, *inrast2;
    int nrows, ncols, row, col;
    void *cell1, *cell2;

    /* NOTE: no need to control, if the map exists. it should be checked in edge.c */
    mapset = G_find_raster2(name1, "");
    maptype1 = Rast_map_type(name1, mapset);
    mapd1 = Rast_open_old(name1, mapset);
    inrast1 = Rast_allocate_buf(maptype1);

    mapset = G_find_raster2(name2, "");
    maptype2 = Rast_map_type(name2, mapset);
    mapd2 = Rast_open_old(name2, mapset);
    inrast2 = Rast_allocate_buf(maptype2);

    G_message(_("Reading maps  <%s,%s> while finding 0 distance ..."), name1,
	      name2);

    ncols = Rast_window_cols();
    nrows = Rast_window_rows();

    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 2);

	Rast_get_row(mapd1, inrast1, row, maptype1);
	Rast_get_row(mapd2, inrast2, row, maptype2);

	for (col = 0; col < ncols; col++) {

	    /* first raster */
	    switch (maptype1) {
	    case CELL_TYPE:
		cell1 = ((CELL **) inrast1)[col];
		break;
	    case FCELL_TYPE:
		cell1 = ((FCELL **) inrast1)[col];
		break;
	    case DCELL_TYPE:
		cell1 = ((DCELL **) inrast1)[col];
		break;
	    }
	    /* second raster */
	    switch (maptype2) {
	    case CELL_TYPE:
		cell2 = ((CELL **) inrast2)[col];
		break;
	    case FCELL_TYPE:
		cell2 = ((FCELL **) inrast2)[col];
		break;
	    case DCELL_TYPE:
		cell2 = ((DCELL **) inrast2)[col];
		break;
	    }

	    if (!Rast_is_null_value(&cell1, maptype1) &&
		!Rast_is_null_value(&cell2, maptype2)) {

		*zerro_row = row;
		*zerro_col = col;

		/* memory cleanup */
		G_free(inrast1);
		G_free(inrast2);

		/* closing raster maps */
		Rast_close(mapd1);
		Rast_close(mapd2);
		return 1;
	    }
	}
    }
    /* memory cleanup */
    G_free(inrast1);
    G_free(inrast2);

    /* closing raster maps */
    Rast_close(mapd1);
    Rast_close(mapd2);

    return 0;
}
