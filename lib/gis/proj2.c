/*!
  \file gis/proj2.c

  \brief GIS Library - Projection support
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Get units code

  \param n project code

  \retun units code
*/
int G__projection_units(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return 0;
    case PROJECTION_UTM:
	return METERS;
    case PROJECTION_SP:
	return FEET;
    case PROJECTION_LL:
	return DEGREES;
    default:
	return -1;
    }
}

/*!
  \brief Get units name

  \param unit units code
  \param plural plural form

  \return units name
*/
const char *G__unit_name(int unit, int plural)
{
    switch (unit) {
    case 0:
	return plural ? "units" : "unit";
    case METERS:
	return plural ? "meters" : "meter";
    case FEET:
	return plural ? "feet" : "foot";
    case DEGREES:
	return plural ? "degrees" : "degree";
    default:
	return NULL;
    }
}

/*!
  \brief Get projection name
  
  \param n projection code

  \return projection name
*/
const char *G__projection_name(int n)
{
    switch (n) {
    case PROJECTION_XY:
	return "x,y";
    case PROJECTION_UTM:
	return "UTM";
    case PROJECTION_SP:
	return "State Plane";
    case PROJECTION_LL:
	return _("Latitude-Longitude");
    case PROJECTION_OTHER:
	return _("Other Projection");
    default:
	return NULL;
    }
}
