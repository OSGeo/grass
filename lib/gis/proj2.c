/*!
  \file gis/proj2.c

  \brief GIS Library - Projection support (internal subroutines)
  
  (C) 2001-2010 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Get projection units code

  \param n projection code

  \return units code
  \return -1 if not defined
*/
int G__projection_units(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return U_UNKNOWN;
    case PROJECTION_UTM:
	return U_METERS;
    case PROJECTION_SP:
	return U_FEET;
    case PROJECTION_LL:
	return U_DEGREES;
    default:
	return -1;
    }
}

/*!
  \brief Get projection name
  
  \param n projection code

  \return projection name
  \return NULL on error
*/
const char *G__projection_name(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return "x,y";
    case PROJECTION_UTM:
	return "UTM";
    case PROJECTION_SP:
	return _("State Plane");
    case PROJECTION_LL:
	return _("Latitude-Longitude");
    case PROJECTION_OTHER:
	return _("Other Projection");
    default:
	return NULL;
    }
}
