/*!
 * \file lib/gis/short_way.c
 *
 * \brief GIS Library - Shortest path functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Shortest way between two eastings.
 *
 * For lat-lon projection (<tt>PROJECTION_LL</tt>), <i>east1</i>, 
 * <i>east2</i> are changed so that they are no more than 180 degrees 
 * apart. Their true locations are not changed. For all other 
 * projections, this function does nothing.
 *
 * \param[in,out] east1 east (x) coordinate of first point
 * \param[in,out] east2 east (x) coordinate of second point
 */

void G_shortest_way(double *east1, double *east2)
{
    if (G_projection() == PROJECTION_LL) {
	if (*east1 > *east2)
	    while ((*east1 - *east2) > 180)
		*east2 += 360;
	else if (*east2 > *east1)
	    while ((*east2 - *east1) > 180)
		*east1 += 360;
    }
}
