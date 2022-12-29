/*
 **  Original Algorithm:    H. Mitasova, L. Mitas, J. Hofierka, M. Zlocha 
 **  GRASS Implementation:  J. Caplan, M. Ruesink  1995
 **
 **  US Army Construction Engineering Research Lab, University of Illinois 
 **
 **  Copyright  M. Ruesink, J. Caplan, H. Mitasova, L. Mitas, J. Hofierka, 
 **     M. Zlocha  1995
 **
 **This program is free software; you can redistribute it and/or
 **modify it under the terms of the GNU General Public License
 **as published by the Free Software Foundation; either version 2
 **of the License, or (at your option) any later version.
 **
 **This program is distributed in the hope that it will be useful,
 **but WITHOUT ANY WARRANTY; without even the implied warranty of
 **MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **GNU General Public License for more details.
 **
 **You should have received a copy of the GNU General Public License
 **along with this program; if not, write to the Free Software
 **Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 **
 */


#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "r.flow.h"
#include "io.h"
#include "mem.h"
#include "aspect.h"


/* Function prototypes */
static void precompute_aspects(void);
static void reflect_and_sentinel(void);
static void interpolate_border(void);
static void upslope_correction(void);
static void precompute_epsilons(void);
static void precompute_ew_dists(void);

/**************************** PRECOMPUTATION ****************************/

void precompute(void)
{
    G_verbose_message(_("Precomputing e/w distances..."));
    precompute_ew_dists();
    G_verbose_message(_("Precomputing quantization tolerances..."));
    precompute_epsilons();
    if (parm.up) {
	G_verbose_message(_("Precomputing inverted elevations..."));
	upslope_correction();
    }
    if (!parm.aspin) {
	G_verbose_message(_("Precomputing interpolated border elevations..."));
	interpolate_border();
    }

    if (!parm.mem) {
	if (parm.aspin) {
	    G_verbose_message(_("Precomputing re-oriented aspects..."));
	    reflect_and_sentinel();
	}
	else {
	    G_verbose_message(_("Precomputing aspects..."));
	    precompute_aspects();
	}
    }
}


static void precompute_ew_dists(void)
{
    int row;
    double northing;

    G_begin_distance_calculations();

    if (G_projection() == PROJECTION_LL) {
	for (row = 0; row < region.rows; row++) {
	    northing = Rast_row_to_northing(row + 0.5, &region);
	    ew_dist[row] = G_distance(Rast_col_to_easting(0., &region), northing,
				      Rast_col_to_easting(1., &region),
				      northing);
	}
    }
    else
	for (row = 0; row < region.rows; row++)
	    ew_dist[row] = region.ew_res;
}

static void precompute_epsilons(void)
{
    int row;
    double x, y, t, a;

    for (row = 0; row < region.rows; row++) {
	x = ew_dist[row];
	y = region.ns_res;
	if (x < y) {
	    t = y;
	    y = x;
	    x = t;
	}

	if ((a = atan2(y, x)) <= 0.5 * DEG2RAD) {
	    if ((G_projection() == PROJECTION_LL))
		/* probably this doesn't work at all with LatLong? -MN 2005 */
		G_fatal_error(_("Resolution too unbalanced:\n"
				"atan2(%f deg, %f deg) =%f < %f tolerance\n"
				"please resample input map"),
			      region.ew_res, region.ns_res, a, 0.5 * DEG2RAD);
	    else
		G_fatal_error(_("Resolution too unbalanced (%f x %f); "
				"please resample input map"),
			      region.ew_res, region.ns_res);
	}

	epsilon[HORIZ][row] = (y / tan(a - 0.5 * DEG2RAD)) - x;
	epsilon[VERT][row] = (x * tan(a + 0.5 * DEG2RAD)) - y;

	G_debug(3, "ROW %d: HORIZ %f, VERT %f\n", row, epsilon[HORIZ][row],
		epsilon[VERT][row]);
    }
}

static void upslope_correction(void)
{
    int row, col;

    for (row = 0; row < region.rows; row++)
	for (col = 0; col < region.cols; col++)
	    put(el, row, col, -get(el, row, col));

    /* rotation of 180 degrees */
    if (parm.aspin) {
	for (row = 0; row < region.rows; row++) {
	    for (col = 0; col < region.cols; col++) {
		if (aspect(row, col) <= 180)
		    put(as, row, col, aspect(row, col) + 180);
		else if (aspect(row, col) <= 360)
		    put(as, row, col, aspect(row, col) - 180);
	    }
	}
    }
}

static void interpolate_border(void)
{
    int i, r = region.rows, c = region.cols;

    for (i = 0; i < c; i++) {
	put(el, -1, i, get(el, 0, i) * 2 - get(el, 1, i));
	put(el, r, i, get(el, r - 1, i) * 2 - get(el, r - 2, i));
    }
    for (i = 0; i < r; i++) {
	put(el, i, -1, get(el, i, 0) * 2 - get(el, i, 1));
	put(el, i, c, get(el, i, c - 1) * 2 - get(el, i, c - 2));
    }
    put(el, -1, -1, 3 * get(el, 0, 0) - get(el, 0, 1) - get(el, 1, 0));
    put(el, -1, c,
	3 * get(el, 0, c - 1) - get(el, 0, c - 2) - get(el, 1, c - 1));
    put(el, r, -1,
	3 * get(el, r - 1, 0) - get(el, r - 2, 0) - get(el, r - 1, 1));
    put(el, r, c,
	3 * get(el, r - 1, c - 1) - get(el, r - 2, c - 1) - get(el, r - 1,
								c - 2));
}

static void reflect_and_sentinel(void)
{
    int row, col;

    /* reflection along diagonal y = x, sentineling of 0 cells */
    for (row = 0; row < region.rows; row++) {
	for (col = 0; col < region.cols; col++) {
	    if (aspect(row, col) == 0)
		/* put(as, row, col, (int) UNDEF); */
		Rast_set_d_null_value(&(as.buf[row][col]), 1);
	    else if (aspect(row, col) < 90)
		put(as, row, col, 90 - aspect(row, col));
	    else
		put(as, row, col, 450 - aspect(row, col));
	}
    }
}

static void precompute_aspects(void)
{
    int row, col;
    double d;
    DCELL *n, *c, *s, temp;

    for (row = 0; row < region.rows; row++) {
	n = get_row(el, row - 1);
	c = get_row(el, row);
	s = get_row(el, row + 1);
	d = ew_dist[row];

	for (col = 0; col < region.cols; col++) {
	    temp = aspect_fly(n++, c++, s++, d);
	    if (temp == UNDEF)
		Rast_set_d_null_value(&(as.buf[row][col]), 1);
	    else
		put(as, row, col, temp);
	}
    }
}
