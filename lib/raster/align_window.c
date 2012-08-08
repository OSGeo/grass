/*!
 * \file lib/raster/align_window.c
 *
 * \brief GIS Library - Window alignment functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdio.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Align two regions.
 *
 * Modifies the input <i>window</i> to align to <i>ref</i> region. The
 * resolutions in <i>window</i> are set to match those in <i>ref</i>
 * and the <i>window</i> edges (north, south, east, west) are modified
 * to align with the grid of the <i>ref</i> region.
 *
 * The <i>window</i> may be enlarged if necessary to achieve the
 * alignment.  The north is rounded northward, the south southward,
 * the east eastward and the west westward. Lon-lon constraints are
 * taken into consideration to make sure that the north doesn't go
 * above 90 degrees (for lat/lon) or that the east does "wrap" past
 * the west, etc.
 *
 * \param[in,out] window pointer to Cell_head to be modified
 * \param ref pointer to Cell_head
 *
 * \return NULL on success
 */

void Rast_align_window(struct Cell_head *window, const struct Cell_head *ref)
{
    int preserve;

    window->ns_res = ref->ns_res;
    window->ew_res = ref->ew_res;
    window->zone = ref->zone;
    window->proj = ref->proj;

    preserve = window->proj == PROJECTION_LL &&
	window->east == (window->west + 360);
    window->south =
	Rast_row_to_northing(ceil(Rast_northing_to_row(window->south, ref)), ref);
    window->north =
	Rast_row_to_northing(floor(Rast_northing_to_row(window->north, ref)), ref);
    window->east =
	Rast_col_to_easting(ceil(Rast_easting_to_col(window->east, ref)), ref);
    window->west =
	Rast_col_to_easting(floor(Rast_easting_to_col(window->west, ref)), ref);

    if (window->proj == PROJECTION_LL) {
	while (window->north > 90.0)
	    window->north -= window->ns_res;
	while (window->south < -90.0)
	    window->south += window->ns_res;

	if (preserve)
	    window->east = window->west + 360;
	else
	    while (window->east - window->west > 360.0)
		window->east -= window->ew_res;
    }

    G_adjust_Cell_head(window, 0, 0);
}
