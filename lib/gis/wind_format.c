/**
 * \file wind_format.c
 *
 * \brief Window formatting functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <grass/gis.h>


static int format_double(double,char *);


/**
 * \fn int G_format_northing (double north, char *buf, int projection)
 *
 * \brief Northing to ASCII.
 *
 * Converts the double representation of the <b>north</b> coordinate to 
 * its ASCII representation (into <b>buf</b>).
 *
 * \param[in] north northing
 * \param[in,out] buf buffer to hold formatted string
 * \param[in] projection
 * \return always returns 0
 */

int G_format_northing ( double north, char *buf,int projection)
{
    if (projection == PROJECTION_LL)
	G_lat_format (north, buf);
    else
	format_double (north, buf);

    return 0;
}


/**
 * \fn int G_format_easting (double east, char *buf, int projection)
 *
 * \brief Easting to ASCII.
 *
 * Converts the double representation of the <b>east</b> coordinate to
 * its ASCII representation (into <b>buf</b>).
 *
 * \param[in] east easting
 * \param[in,out] buf buffer to hold formatted string
 * \param[in] projection
 * \return always returns 0
 */

int G_format_easting ( double east, char *buf,int projection)
{
    if (projection == PROJECTION_LL)
	G_lon_format (east, buf);
    else
	format_double (east, buf);

    return 0;
}


/**
 * \fn int G_format_resolution (double res, char *buf, int projection)
 *
 * \brief Resolution to ASCII.
 *
 * Converts the double representation of the <b>resolution</b> to its 
 * ASCII representation (into <b>buf</b>).
 *
 *  \param[in] resolution
 *  \param[in,out] buf buffer to hold formatted string
 *  \param[in] projection
 *  \return always returns 0
 */

int G_format_resolution ( double res, char *buf,int projection)
{
    if (projection == PROJECTION_LL)
	G_llres_format (res, buf);
    else
	format_double (res, buf);

    return 0;
}

static int format_double ( double value, char *buf)
{
    sprintf (buf, "%.8f", value);
    G_trim_decimal (buf);

    return 0;
}
