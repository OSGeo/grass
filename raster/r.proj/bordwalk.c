/*
 * Function added to r.proj
 * GNU GPL by Morten Hulden <morten@untamo.net>, August 2000
 *
 * bordwalk.c - projects the border cell centers of a map or region
 * to check whether they are inside the borders of another map/region 
 * in a different location, and adjusts the cell header so
 * only overlapping areas are included. The function is called by main,
 * first to project the output region into the input map and trim the
 * borders of this in order to get a smaller map, and faster and less hungry 
 * memory allocation. Then main calls the function again, but reversed,
 * to project the input map on the output region, trimming this down to 
 * the smallest possible rectangular region.
 * 
 * Simply using corner and midpoints (original r.proj) will only work 
 * between cylindrical projections. In other projections, though he input 
 * map is always a rectangular area, the projected output can be of almost 
 * any shape and its position can be rotated any way. It can even be a 
 * discontinous area.
 *
 * In many projections, especially when large areas are displayed, the edges 
 * of rectangular GRASS regions do not necessarily represent east, west, north 
 * and south. Naming the region edges accordingly (as is regions and cellhd) can be 
 * misleading. (Well, invite code readers/writers to make assumptions anyway. Don't 
 * assume north is really direction north in this code ;)
 */

#include <math.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

void bordwalk(struct Cell_head *from_hd, struct Cell_head *to_hd,
	      struct pj_info *from_pj, struct pj_info *to_pj)
{
    double idx;
    double hx, hy;
    double xmin, xmax;
    double ymin, ymax;

    /* Set some (un)reasonable defaults before we walk the borders */

    xmax = to_hd->west - 0.000001;
    xmin = to_hd->east + 0.000001;
    ymin = to_hd->north + 0.000001;
    ymax = to_hd->south - 0.000001;

    /* Start walking */

    /* Top */
    for (idx = from_hd->west + from_hd->ew_res / 2; idx < from_hd->east;
	 idx += from_hd->ew_res) {
	hx = idx;
	hy = from_hd->north - from_hd->ns_res / 2;
	if (pj_do_proj(&hx, &hy, from_pj, to_pj) < 0)
	    continue;
	/* check if we are within the region, but allow for some 'almost inside' points */
	/* (should probably be a factor based on input and output resolutions) */
	if (!(hx < to_hd->west - to_hd->ew_res) &&
	    !(hx > to_hd->east + to_hd->ew_res) &&
	    !(hy < to_hd->south - to_hd->ns_res) &&
	    !(hy > to_hd->north + to_hd->ns_res)) {
	    xmin = !(hx > xmin) ? hx : xmin;
	    xmax = !(hx < xmax) ? hx : xmax;
	    ymin = !(hy > ymin) ? hy : ymin;
	    ymax = !(hy < ymax) ? hy : ymax;
	}
    }

    G_debug(3, "Top: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin, xmax,
	    ymin, ymax);

    /* Right */
    for (idx = from_hd->north - from_hd->ns_res / 2; idx > from_hd->south;
	 idx -= from_hd->ns_res) {
	hx = from_hd->east - from_hd->ew_res / 2;
	hy = idx;
	if (pj_do_proj(&hx, &hy, from_pj, to_pj) < 0)
	    continue;
	if (!(hx < to_hd->west - to_hd->ew_res) &&
	    !(hx > to_hd->east + to_hd->ew_res) &&
	    !(hy < to_hd->south - to_hd->ns_res) &&
	    !(hy > to_hd->north + to_hd->ns_res)) {
	    xmin = !(hx > xmin) ? hx : xmin;
	    xmax = !(hx < xmax) ? hx : xmax;
	    ymin = !(hy > ymin) ? hy : ymin;
	    ymax = !(hy < ymax) ? hy : ymax;
	}
    }

    G_debug(3, "Right: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin, xmax,
	    ymin, ymax);

    /* Bottom */
    for (idx = from_hd->east - from_hd->ew_res / 2; idx > from_hd->west;
	 idx -= from_hd->ew_res) {
	hx = idx;
	hy = from_hd->south + from_hd->ns_res / 2;
	if (pj_do_proj(&hx, &hy, from_pj, to_pj) < 0)
	    continue;
	if (!(hx < to_hd->west - to_hd->ew_res) &&
	    !(hx > to_hd->east + to_hd->ew_res) &&
	    !(hy < to_hd->south - to_hd->ns_res) &&
	    !(hy > to_hd->north + to_hd->ns_res)) {
	    xmin = !(hx > xmin) ? hx : xmin;
	    xmax = !(hx < xmax) ? hx : xmax;
	    ymin = !(hy > ymin) ? hy : ymin;
	    ymax = !(hy < ymax) ? hy : ymax;
	}
    }

    G_debug(3, "Bottom: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin, xmax,
	    ymin, ymax);

    /* Left */
    for (idx = from_hd->south + from_hd->ns_res / 2; idx < from_hd->north;
	 idx += from_hd->ns_res) {
	hx = from_hd->west + from_hd->ew_res / 2;
	hy = idx;
	if (pj_do_proj(&hx, &hy, from_pj, to_pj) < 0)
	    continue;
	if (!(hx < to_hd->west - to_hd->ew_res) &&
	    !(hx > to_hd->east + to_hd->ew_res) &&
	    !(hy < to_hd->south - to_hd->ns_res) &&
	    !(hy > to_hd->north + to_hd->ns_res)) {
	    xmin = !(hx > xmin) ? hx : xmin;
	    xmax = !(hx < xmax) ? hx : xmax;
	    ymin = !(hy > ymin) ? hy : ymin;
	    ymax = !(hy < ymax) ? hy : ymax;
	}
    }

    G_debug(3, "Left: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin, xmax,
	    ymin, ymax);

    /* check some special cases by reversing the projection */

    if (xmin > to_hd->west) {
	hx = to_hd->west + to_hd->ew_res / 2;
	hy = to_hd->south + (to_hd->north - to_hd->south) / 2;
	if (!(pj_do_proj(&hx, &hy, to_pj, from_pj) < 0) &&
	    !(hx < from_hd->west) && !(hx > from_hd->east) &&
	    !(hy < from_hd->south) && !(hy > from_hd->north))
	    xmin = to_hd->west + to_hd->ew_res / 2;
    }

    if (xmax < to_hd->east) {
	hx = to_hd->east - to_hd->ew_res / 2;
	hy = to_hd->south + (to_hd->north - to_hd->south) / 2;
	if (!(pj_do_proj(&hx, &hy, to_pj, from_pj) < 0) &&
	    !(hx < from_hd->west) && !(hx > from_hd->east) &&
	    !(hy < from_hd->south) && !(hy > from_hd->north))
	    xmax = to_hd->east - to_hd->ew_res / 2;
    }

    if (ymin > to_hd->south) {
	hx = to_hd->west + (to_hd->east - to_hd->west) / 2;
	hy = to_hd->south + to_hd->ns_res / 2;
	if (!(pj_do_proj(&hx, &hy, to_pj, from_pj) < 0) &&
	    !(hx < from_hd->west) && !(hx > from_hd->east) &&
	    !(hy < from_hd->south) && !(hy > from_hd->north))
	    ymin = to_hd->south + to_hd->ns_res / 2;
    }

    if (ymax < to_hd->north) {
	hx = to_hd->west + (to_hd->east - to_hd->west) / 2;
	hy = to_hd->north - to_hd->ns_res / 2;
	if (!(pj_do_proj(&hx, &hy, to_pj, from_pj) < 0) &&
	    !(hx < from_hd->west) && !(hx > from_hd->east) &&
	    !(hy < from_hd->south) && !(hy > from_hd->north))
	    ymax = to_hd->north - to_hd->ns_res / 2;
    }

    G_debug(3, "Extra check: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin,
	    xmax, ymin, ymax);

    /* if we still have some unresonable default minmax left, then abort */

    if ((xmin > to_hd->east) || (xmax < to_hd->west)
	|| (ymin > to_hd->north) || (ymax < to_hd->south))
	G_fatal_error(_("Input raster map is outside current region"));

    if (xmin < to_hd->west + to_hd->ew_res / 2)
	xmin = to_hd->west + to_hd->ew_res / 2;
    if (xmax > to_hd->east - to_hd->ew_res / 2)
	xmax = to_hd->east - to_hd->ew_res / 2;
    if (ymin < to_hd->south + to_hd->ns_res / 2)
	ymin = to_hd->south + to_hd->ns_res / 2;
    if (ymax > to_hd->north - to_hd->ns_res / 2)
	ymax = to_hd->north - to_hd->ns_res / 2;

    /* adjust to edges */

    idx = (int)floor(Rast_easting_to_col(xmin, to_hd));
    xmin = Rast_col_to_easting(idx + 0.0, to_hd);
    idx = (int)floor(Rast_easting_to_col(xmax, to_hd));
    xmax = Rast_col_to_easting(idx + 1.0, to_hd);
    idx = (int)floor(Rast_northing_to_row(ymin, to_hd));
    ymin = Rast_row_to_northing(idx + 1.0, to_hd);
    idx = (int)floor(Rast_northing_to_row(ymax, to_hd));
    ymax = Rast_row_to_northing(idx + 0.0, to_hd);

    to_hd->west = (xmin < to_hd->west) ? to_hd->west : xmin;
    to_hd->east = (xmax > to_hd->east) ? to_hd->east : xmax;
    to_hd->south = (ymin < to_hd->south) ? to_hd->south : ymin;
    to_hd->north = (ymax > to_hd->north) ? to_hd->north : ymax;

    G_debug(3, "Final check: xmin: %f; xmax: %f; ymin: %f; ymax: %f", xmin,
	    xmax, ymin, ymax);
}
