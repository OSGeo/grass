/*!
  \file lib/gis/proj2.c

  \brief GIS Library - Projection support (internal subroutines)
  
  (C) 2001-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Get projection units code (for internal use only)

  \param n projection code

  Supported units (see gis.h):
   - U_UNKNOWN (XY)
   - U_METERS  (UTM)
   - U_FEET    (SP)
   - U_USFEET (a few SP)
   - U_DEGREES (LL)
   
  \return units code (see gis.h)
  \return U_UNDEFINED if not defined
*/
int G_projection_units(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return U_UNKNOWN;
    case PROJECTION_UTM:
	return U_METERS;
    case PROJECTION_LL:
	return U_DEGREES;
    default:
	return U_UNDEFINED;
    }
    return U_UNDEFINED;
}

/*!
  \brief Get projection name
  
  \param n projection code

  \return projection name
  \return NULL on error
*/
const char *G_projection_name(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return "x,y";
    case PROJECTION_UTM:
	return "UTM";
    case PROJECTION_LL:
	return _("Latitude-Longitude");
    case PROJECTION_OTHER:
	return _("Other Projection");
    default:
	return NULL;
    }
}
