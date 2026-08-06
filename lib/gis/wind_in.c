/*!
 * \file lib/gis/wind_in.c
 *
 * \brief Point in region functions.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
*
 * \author Hamish Bowman. (c) Hamish Bowman, and the GRASS Development Team
 *
 * \date February 2007
 */

#include <grass/gis.h>

/*!
 * \brief Returns TRUE if coordinate is within the current region settings.
 *
 * \param easting
 * \param northing
 * \return int
 *
 */
int G_point_in_region(double easting, double northing)
{
    struct Cell_head window;

    G_get_window(&window);

    return G_point_in_window(easting, northing, &window);
}

/*!
 * \brief Returns TRUE if coordinate is within the given map region.
 *
 * Use instead of G_point_in_region() when used in a loop (it's more
 * efficient to only fetch the window once) or for checking if a point
 * is in another region (e.g. contained with a raster map's bounds).
 *
 * \param easting
 * \param northing
 * \param window
 * \return int
 *
 */
int G_point_in_window(double easting, double northing,
                      const struct Cell_head *window)
{

    if (easting > window->east || easting < window->west ||
        northing > window->north || northing < window->south)
        return FALSE;

    return TRUE;
}
