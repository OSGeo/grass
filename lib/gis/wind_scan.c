
/**
 * \file wind_scan.c
 *
 * \brief GIS Library - Window scanning functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdio.h>
#include <grass/gis.h>


static int scan_double(const char *, double *);


/**
 * \brief ASCII northing to double.
 *
 * Converts the ASCII "northing" coordinate string in <b>buf</b> to its 
 * double representation (into <b>northing</b>).
 *
 * \param[in] buf buffer hold string northing
 * \param[in,out] northing
 * \param[in] projection
 * \return 0 on error
 * \return 1 on success
 */

int G_scan_northing(const char *buf, double *northing, int projection)
{
    if (projection == PROJECTION_LL) {
	if (G_lat_scan(buf, northing))
	    return 1;
	if (!scan_double(buf, northing))
	    return 0;

	return (*northing <= 90.0 && *northing >= -90.0);
    }

    return scan_double(buf, northing);
}


/**
 * \brief ASCII easting to double.
 *
 * Converts the ASCII "easting" coordinate string in <b>buf</b> to its 
 * double representation (into <b>easting</b>).
 *
 * \param[in] buf buffer containing string easting
 * \param[in,out] easting
 * \param[in] projection
 * \return 0 on error
 * \return 1 on success
 */

int G_scan_easting(const char *buf, double *easting, int projection)
{
    if (projection == PROJECTION_LL) {
	if (G_lon_scan(buf, easting))
	    return 1;
	if (!scan_double(buf, easting))
	    return 0;
	while (*easting > 180.0)
	    *easting -= 360.0;
	while (*easting < -180.0)
	    *easting += 360.0;

	return 1;
    }

    return scan_double(buf, easting);
}


/**
 * \brief ASCII resolution to double.
 *
 * Converts the ASCII "resolution" string in <b>buf</b> to its double 
 * representation (into resolution).
 *
 * \param[in] buf buffer containing string resolution
 * \param[in,out] resolution
 * \param[in] projection
 * \return 0 on error
 * \return 1 on success
 */

int G_scan_resolution(const char *buf, double *res, int projection)
{
    if (projection == PROJECTION_LL) {
	if (G_llres_scan(buf, res))
	    return 1;
    }

    return (scan_double(buf, res) && *res > 0.0);
}


static int scan_double(const char *buf, double *value)
{
    char junk[2];

    /* use sscanf to convert buf to double
     * make sure value doesn't have other characters after it */

    *junk = 0;
    *value = 0.0;

    if (sscanf(buf, "%lf%1s", value, junk) == 1 && *junk == 0) {
	while (*buf)
	    buf++;
	buf--;

	if (*buf >= 'A' && *buf <= 'Z')
	    return 0;
	if (*buf >= 'a' && *buf <= 'z')
	    return 0;

	return 1;		/* success */
    }

    return 0;			/* failure */
}
