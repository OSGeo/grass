/*!
 * \file gis/wind_format.c
 *
 * \brief GIS Library - Window formatting functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdio.h>
#include <grass/gis.h>

static void format_double(double, char *);

/*!
 * \brief Northing to ASCII.
 *
 * Converts the double representation of the <i>north</i> coordinate to 
 * its ASCII representation (into <i>buf</i>).
 *
 * \param north northing
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code
 */
void G_format_northing(double north, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_lat_format(north, buf);
    else
	format_double(north, buf);
}

/*!
 * \brief Easting to ASCII.
 *
 * Converts the double representation of the <i>east</i> coordinate to
 * its ASCII representation (into <i>buf</i>).
 *
 * \param east easting
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code
 */
void G_format_easting(double east, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_lon_format(east, buf);
    else
	format_double(east, buf);
}

/*!
 * \brief Resolution to ASCII.
 *
 * Converts the double representation of the <i>resolution</i> to its 
 * ASCII representation (into <i>buf</i>).
 *
 * \param resolution resolution value
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code
 */
void G_format_resolution(double res, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_llres_format(res, buf);
    else
	format_double(res, buf);
}

static void format_double(double value, char *buf)
{
    /* if the programmer has lied to G_format_resolution() about the
        projection type in order to get FP values for lat/lon coords,
        "%.8f" is not enough to preserve fidelity once converted
        back into D:M:S, which leads to rounding errors. */
    if (G_projection() == PROJECTION_LL)
	sprintf(buf, "%.15g", value);
    else
	sprintf(buf, "%.8f", value);
    G_trim_decimal(buf);
}
