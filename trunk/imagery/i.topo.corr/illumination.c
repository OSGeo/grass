/* File: illumination.c
 *
 *  AUTHOR:    E. Jorge Tizado, Spain 2010
 *
 *  COPYRIGHT: (c) 2007-10 E. Jorge Tizado
 *             This program is free software under the GNU General Public
 *             License (>=v2). Read the file COPYING that comes with GRASS
 *             for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "local_proto.h"

void eval_cosi(Gfile * out, Gfile * dem, double zenith, double azimuth)
{
    struct Cell_head window;

    int row, col, nrows, ncols;
    DCELL *cell[3], *temp;
    DCELL *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9;
    double H, V, dx, dy, key, north, east, south, west, center;
    double cos_i, cos_z, sin_z, slope, aspect;

    Rast_get_window(&window);

    G_begin_distance_calculations();
    north = Rast_row_to_northing(0.5, &window);
    center = Rast_row_to_northing(1.5, &window);
    south = Rast_row_to_northing(2.5, &window);
    east = Rast_col_to_easting(2.5, &window);
    west = Rast_col_to_easting(0.5, &window);
    V = G_distance(east, north, east, south) * 4;
    H = G_distance(east, center, west, center) * 4;

    zenith *= D2R;
    azimuth *= D2R;

    cos_z = cos(zenith);
    sin_z = sin(zenith);

    /* Making cos_i raster ... */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    cell[0] = (DCELL *) G_calloc(ncols + 1, sizeof(DCELL));
    Rast_set_d_null_value(cell[0], ncols);
    cell[1] = (DCELL *) G_calloc(ncols + 1, sizeof(DCELL));
    Rast_set_d_null_value(cell[1], ncols);
    cell[2] = (DCELL *) G_calloc(ncols + 1, sizeof(DCELL));
    Rast_set_d_null_value(cell[2], ncols);

    /* First row is null */
    Rast_set_null_value((DCELL *) out->rast, Rast_window_cols(), DCELL_TYPE);
    Rast_put_row(out->fd, out->rast, DCELL_TYPE);
    /* Next rows ... */
    for (row = 2; row < nrows; row++) {
	G_percent(row, nrows, 2);
	temp = cell[0];
	cell[0] = cell[1];
	cell[1] = cell[2];
	cell[2] = temp;
	Rast_get_d_row_nomask(dem->fd, cell[2], row);

	c1 = cell[0];
	c2 = c1 + 1;
	c3 = c1 + 2;
	c4 = cell[1];
	c5 = c4 + 1;
	c6 = c4 + 2;
	c7 = cell[2];
	c8 = c7 + 1;
	c9 = c7 + 2;

	for (col = 1; col < ncols - 1;
	     col++, c1++, c2++, c3++, c4++, c5++, c6++, c7++, c8++, c9++) {
	    if (Rast_is_d_null_value(c1) || Rast_is_d_null_value(c2) ||
		Rast_is_d_null_value(c3) || Rast_is_d_null_value(c4) ||
		Rast_is_d_null_value(c5) || Rast_is_d_null_value(c6) ||
		Rast_is_d_null_value(c7) || Rast_is_d_null_value(c8) ||
		Rast_is_d_null_value(c9)) {
		Rast_set_d_null_value((DCELL *) out->rast + col, 1);
	    }
	    else {
		dx = ((*c1 + *c4 + *c4 + *c7) - (*c3 + *c6 + *c6 + *c9)) / H;
		dy = ((*c1 + *c2 + *c2 + *c3) - (*c7 + *c8 + *c8 + *c9)) / V;
		key = dx * dx + dy * dy;
		slope = atan(sqrt(key));
		aspect = atan2(dx, -dy);
		if (aspect < 0.0)
		    aspect += 2 * PI;

		cos_i =
		    cos_z * cos(slope) + sin_z * sin(slope) * cos(azimuth -
								  aspect);

		((DCELL *) out->rast)[col] = (DCELL) cos_i;
	    }
	}

	Rast_put_row(out->fd, out->rast, DCELL_TYPE);
    }
    /* Last row is null */
    Rast_set_null_value((DCELL *) out->rast, Rast_window_cols(), DCELL_TYPE);
    Rast_put_row(out->fd, out->rast, DCELL_TYPE);
}

