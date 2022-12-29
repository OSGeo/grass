/*!
 * \file lib/gis/wind_format.c
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

static void format_double(double, char *, int);

/*!
 * \brief Northing to ASCII.
 *
 * Converts the double representation of the <i>north</i> coordinate to 
 * its ASCII representation (into <i>buf</i>).
 *
 * \param north northing
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code, or -1 to force full precision FP
 */
void G_format_northing(double north, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_lat_format(north, buf);
    else if (projection == -1)
	format_double(north, buf, TRUE);
    else
	format_double(north, buf, FALSE);
}

/*!
 * \brief Easting to ASCII.
 *
 * Converts the double representation of the <i>east</i> coordinate to
 * its ASCII representation (into <i>buf</i>).
 *
 * \param east easting
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code, or -1 to force full precision FP
 */
void G_format_easting(double east, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_lon_format(east, buf);
    else if (projection == -1)
	format_double(east, buf, TRUE);
    else
	format_double(east, buf, FALSE);
}

/*!
 * \brief Resolution to ASCII.
 *
 * Converts the double representation of the <i>resolution</i> to its 
 * ASCII representation (into <i>buf</i>).
 *
 * \param resolution resolution value
 * \param[out] buf buffer to hold formatted string
 * \param projection projection code, or -1 to force full precision FP
 */
void G_format_resolution(double res, char *buf, int projection)
{
    if (projection == PROJECTION_LL)
	G_llres_format(res, buf);
    else if (projection == -1)
	format_double(res, buf, TRUE);
    else
	format_double(res, buf, FALSE);
}

/*
 * 'full_prec' is boolean, FALSE uses %.8f,  TRUE uses %.15g
 * The reason to have this is that for lat/lon "%.8f" is not
 * enough to preserve fidelity once converted back into D:M:S,
 * which leads to rounding errors, especially for resolution.
 */
static void format_double(double value, char *buf, int full_prec)
{
    if (full_prec)
	sprintf(buf, "%.15g", value);
    else
	sprintf(buf, "%.8f", value);

    G_trim_decimal(buf);
}
