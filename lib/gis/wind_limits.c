
/*!
 * \file lib/gis/wind_limits.c
 *
 * \brief GIS Library - Projection limit functions.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2014
 */

#include <grass/gis.h>


/**
 * \brief Function not yet implemented...
 *
 * If the projection has absolute limits (like lat/lon), then
 * this routine modifies the input coordinate to be within the
 * limit.<br>
 *
 * <b>Note:</b> Function not yet implemented.
 *
 * \param[in] east
 * \param[in] proj
 * \return 1 no change
 * \return 0 changed
 */

int G_limit_east(double *east, int proj)
{
    return 1;
}


/**
 * \brief Function not yet implemented...
 *
 * If the projection has absolute limits (like lat/lon), then
 * this routine modifies the input coordinate to be within the
 * limit.<br>
 *
 * <b>Note:</b> Function not yet implemented.
 *
 * \param[in] west
 * \param[in] proj
 * \return 1 no change
 * \return 0 changed
 */

int G_limit_west(double *west, int proj)
{
    return 1;
}


/**
 * \brief Limit north (y) coordinate
 *
 * If the projection has absolute limits (like lat/lon), then
 * this routine modifies the input coordinate to be within the
 * limit.<br>
 *
 * \param[in,out] north north coordinate
 * \param[in] proj projection id
 * \return 1 no change
 * \return 0 changed
 */

int G_limit_north(double *north, int proj)
{
    if (proj == PROJECTION_LL) {
	if (*north > 90.0) {
	    *north = 90.0;
	    return 0;
	}
	if (*north < -90) {
	    *north = -90;
	    return 0;
	}
    }

    return 1;
}


/**
 * \brief Limit south (y) coordinate
 *
 * If the projection has absolute limits (like lat/lon), then
 * this routine modifies the input coordinate to be within the
 * limit.<br>
 *
 * \param[in] south south coordinate
 * \param[in] proj projection id
 * \return 1 no change
 * \return 0 changed
 */

int G_limit_south(double *south, int proj)
{
    if (proj == PROJECTION_LL) {
	if (*south > 90.0) {
	    *south = 90.0;
	    return 0;
	}
	if (*south < -90) {
	    *south = -90;
	    return 0;
	}
    }

    return 1;
}
