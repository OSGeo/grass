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
#include "r.proj.h"

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void debug(const char *name, const struct Cell_head *hd)
{
    G_debug(3, "%s: xmin: %f; xmax: %f; ymin: %f; ymax: %f",
	    name, hd->west, hd->east, hd->south, hd->north);
}

static void update(struct Cell_head *to_hd, double hx, double hy)
{
    to_hd->east  = MAX(to_hd->east,  hx);
    to_hd->west  = MIN(to_hd->west,  hx);
    to_hd->north = MAX(to_hd->north, hy);
    to_hd->south = MIN(to_hd->south, hy);
}

static void intersect(struct Cell_head *to_hd, const struct Cell_head *from_hd)
{
    to_hd->east  = MIN(to_hd->east,  from_hd->east);
    to_hd->west  = MAX(to_hd->west,  from_hd->west);
    to_hd->north = MIN(to_hd->north, from_hd->north);
    to_hd->south = MAX(to_hd->south, from_hd->south);
}

static int inside(const struct Cell_head *ref_hd, double hx, double hy)
{
    return 
	(hx <= ref_hd->east) &&
	(hx >= ref_hd->west) &&
	(hy <= ref_hd->north) &&
	(hy >= ref_hd->south);
}

static void invert(struct Cell_head *cur_hd, const struct Cell_head *ref_hd,
		   double epsilon)
{
    cur_hd->east  = ref_hd->west  - epsilon;
    cur_hd->west  = ref_hd->east  + epsilon;
    cur_hd->north = ref_hd->south - epsilon;
    cur_hd->south = ref_hd->north + epsilon;
}

static void proj_update(const struct pj_info *from_pj, const struct pj_info *to_pj,
			const struct pj_info *trans_pj, int dir, 
			struct Cell_head *to_hd, double hx, double hy)
{
	if (GPJ_transform(from_pj, to_pj, trans_pj, dir,
			  &hx, &hy, NULL) < 0)
	    return;
	update(to_hd, hx, hy);
}

void bordwalk1(const struct pj_info *from_pj, const struct pj_info *to_pj,
	       const struct pj_info *trans_pj, int dir,
	       const struct Cell_head *from_hd, struct Cell_head *to_hd)
{
    double idx;

    /* Top */
    for (idx = from_hd->west + from_hd->ew_res / 2; idx < from_hd->east;
	 idx += from_hd->ew_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->north - from_hd->ns_res / 2);

    debug("Top", to_hd);

    /* Right */
    for (idx = from_hd->north - from_hd->ns_res / 2; idx > from_hd->south;
	 idx -= from_hd->ns_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->east - from_hd->ew_res / 2, idx);

    debug("Right", to_hd);

    /* Bottom */
    for (idx = from_hd->east - from_hd->ew_res / 2; idx > from_hd->west;
	 idx -= from_hd->ew_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->south + from_hd->ns_res / 2);

    debug("Bottom", to_hd);

    /* Left */
    for (idx = from_hd->south + from_hd->ns_res / 2; idx < from_hd->north;
	 idx += from_hd->ns_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->west + from_hd->ew_res / 2, idx);

    debug("Left", to_hd);
}

static int proj_inside(const struct pj_info *from_pj, const struct pj_info *to_pj,
		       const struct pj_info *trans_pj, int dir, 
		       const struct Cell_head *ref_hd, double hx, double hy)
{
    if (GPJ_transform(from_pj, to_pj, trans_pj, -dir, &hx, &hy, NULL) < 0)
	return 0;
    return inside(ref_hd, hx, hy);
}

static void reverse_check(const struct pj_info *from_pj,
			  const struct pj_info *to_pj,
			  const struct pj_info *trans_pj, int dir,
			  const struct Cell_head *from_hd,
			  const struct Cell_head *to_hd,
			  struct Cell_head *cur_hd)
{
    if (cur_hd->west > to_hd->west) {
	double hx = to_hd->west + to_hd->ew_res / 2;
	double hy = to_hd->south + (to_hd->north - to_hd->south) / 2;
	if (proj_inside(from_pj, to_pj, trans_pj, dir, from_hd, hx, hy))
	    cur_hd->west = hx;
    }

    if (cur_hd->east < to_hd->east) {
	double hx = to_hd->east - to_hd->ew_res / 2;
	double hy = to_hd->south + (to_hd->north - to_hd->south) / 2;
	if (proj_inside(from_pj, to_pj, trans_pj, dir, from_hd, hx, hy))
	    cur_hd->east = hx;
    }

    if (cur_hd->south > to_hd->south) {
	double hx = to_hd->west + (to_hd->east - to_hd->west) / 2;
	double hy = to_hd->south + to_hd->ns_res / 2;
	if (proj_inside(from_pj, to_pj, trans_pj, dir, from_hd, hx, hy))
	    cur_hd->south = hy;
    }

    if (cur_hd->north < to_hd->north) {
	double hx = to_hd->west + (to_hd->east - to_hd->west) / 2;
	double hy = to_hd->north - to_hd->ns_res / 2;
	if (proj_inside(from_pj, to_pj, trans_pj, dir, from_hd, hx, hy))
	    cur_hd->north = hy;
    }
}

static int outside(const struct Cell_head *cur_hd, const struct Cell_head *ref_hd)
{
    return
	(cur_hd->west  > ref_hd->east ) ||
	(cur_hd->east  < ref_hd->west ) ||
	(cur_hd->south > ref_hd->north) ||
	(cur_hd->north < ref_hd->south);
}

static void snap_to_grid(struct Cell_head *cur_hd, const struct Cell_head *ref_hd)
{
    int lidx = (int) floor(Rast_easting_to_col( cur_hd->west,  ref_hd));
    int ridx = (int) floor(Rast_easting_to_col( cur_hd->east,  ref_hd));
    int bidx = (int) floor(Rast_northing_to_row(cur_hd->south, ref_hd));
    int tidx = (int) floor(Rast_northing_to_row(cur_hd->north, ref_hd));

    cur_hd->west  = Rast_col_to_easting( lidx + 0.0, ref_hd);
    cur_hd->east  = Rast_col_to_easting( ridx + 1.0, ref_hd);
    cur_hd->south = Rast_row_to_northing(bidx + 1.0, ref_hd);
    cur_hd->north = Rast_row_to_northing(tidx + 0.0, ref_hd);
}

void bordwalk(const struct Cell_head *from_hd, struct Cell_head *to_hd,
	      const struct pj_info *from_pj, const struct pj_info *to_pj,
	      const struct pj_info *trans_pj, int dir)
{
    struct Cell_head cur_hd;

    /* Set some (un)reasonable defaults before we walk the borders */

    invert(&cur_hd, to_hd, 1.0e-6);

    /* Start walking */

    bordwalk1(from_pj, to_pj, trans_pj, dir, from_hd, &cur_hd);

    intersect(&cur_hd, to_hd);

    /* check some special cases by reversing the projection */

    reverse_check(from_pj, to_pj, trans_pj, dir, from_hd, to_hd, &cur_hd);

    debug("Extra check", &cur_hd);

    /* if we still have some unresonable default minmax left, then abort */

    if (outside(&cur_hd, to_hd))
	G_fatal_error(_("Input raster map is outside current region"));

    intersect(&cur_hd, to_hd);

    /* adjust to edges */

    snap_to_grid(&cur_hd, to_hd);

    intersect(to_hd, &cur_hd);

    debug("Final check", to_hd);
}

void bordwalk_edge(const struct Cell_head *from_hd, struct Cell_head *to_hd,
	           const struct pj_info *from_pj, const struct pj_info *to_pj,
		   const struct pj_info *trans_pj, int dir)
{
    double idx;
    double hx, hy;

    /* like bordwalk1, but use cell edges instead of cell centers */

    /* start with cell head center */
    hx = (from_hd->west + from_hd->east) / 2.0;
    hy = (from_hd->north + from_hd->south) / 2.0;

    if (GPJ_transform(from_pj, to_pj, trans_pj, dir,
		      &hx, &hy, NULL) < 0)
	G_fatal_error(_("Unable to reproject map center"));

    to_hd->east  = hx;
    to_hd->west  = hx;
    to_hd->north = hy;
    to_hd->south = hy;

    /* Top */
    for (idx = from_hd->west; idx < from_hd->east;
	 idx += from_hd->ew_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->north);
    idx = from_hd->east;
    proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->north);

    debug("Top", to_hd);

    /* Right */
    for (idx = from_hd->north; idx > from_hd->south;
	 idx -= from_hd->ns_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->east, idx);
    idx = from_hd->south;
    proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->east, idx);

    debug("Right", to_hd);

    /* Bottom */
    for (idx = from_hd->east; idx > from_hd->west;
	 idx -= from_hd->ew_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->south);
    idx = from_hd->west;
    proj_update(from_pj, to_pj, trans_pj, dir, to_hd, idx, from_hd->south);

    debug("Bottom", to_hd);

    /* Left */
    for (idx = from_hd->south; idx < from_hd->north;
	 idx += from_hd->ns_res)
	proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->west, idx);
    idx = from_hd->north;
    proj_update(from_pj, to_pj, trans_pj, dir, to_hd, from_hd->west, idx);

    debug("Left", to_hd);
}
