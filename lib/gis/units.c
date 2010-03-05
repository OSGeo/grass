/*!
  \file gis/units.c

  \brief GIS Library - Units management and conversion
  
  (C) 2001-2010 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Adopted for libgis by Martin Landa <landa.martin gmail.com> (2010)
 */

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Units conversion to meters

  Units codes (gis.h):
   - U_METERS
   - U_KILOMETERS
   - U_MILES
   - U_FEET

  Returns a factor which converts the grid unit to meters (by
  multiplication).
 
  \param units units code

  \return factor
*/
double G_units_to_meters_factor(int units)
{
    switch (units) {
    case U_METERS:
	return 1.0;
	break;
	
    case U_KILOMETERS:
	return 1.0e-3;
	break;
	
    case U_MILES:
	return 6.21371192237334e-4;	        /*  1 / (0.0254 * 12 * 5280)    */
	break;
	
    case U_FEET:
	return 3.28083989501312;	        /*  1 / (0.0254 * 12)    */
	break;
	
    default:
	return 1.0;
	break;
    }
 
    return 1.0;
}

/*!
  \brief Units conversion to square meters

  Units codes (gis.h):
   - U_METERS
   - U_KILOMETERS
   - U_ACRES
   - U_HECTARES
   - U_MILES
   - U_FEET

  Returns a factor which converts the grid unit to square meters (by
  multiplication).
 
  \param units units code

  \return factor
*/
double G_units_to_meters_factor_sq(int units)
{
    switch (units) {
    case U_METERS:
	return 1.0;
	break;
	
    case U_KILOMETERS:
	return 1.0e-6;
	break;
	
    case U_ACRES:
	return 2.47105381467165e-4;	/* 640 acres in a sq mile */
	break;
	
    case U_HECTARES:
	return 1.0e-4;
	break;
	
    case U_MILES:
	return 3.86102158542446e-7;	/*  1 / (0.0254 * 12 * 5280)^2  */
	break;
	
    case U_FEET:
	return 10.7639104167097;	        /*  1 / (0.0254 * 12)^2  */
	break;
	
    default:
	return 1.0;
	break;
    }
 
    return 1.0;
}

/*!
  \brief Get localized units name

  Units codes (gis.h):
   - U_METERS
   - U_KILOMETERS
   - U_ACRES
   - U_HECTARES
   - U_MILES
   - U_FEET

  \param units units code
  \param plural plural form if true
  \param square area units if true

  \return units name
  \return NULL if units not found
*/
const char *G_get_units_name(int units, int plural, int square)
{
    switch (units) {
    case U_UNKNOWN:
	if (square)
	    return plural ? _("square units") : _("square unit");
	else
	    return plural ? _("units") : _("unit");
	break;
	  
    case U_METERS:
	if (square) 
	    return plural ? _("square meters") : _("square meter");
	else
	    return plural ? _("meters") : _("meter");
	break;
	
    case U_KILOMETERS:
	if (square)
	    return plural ? _("square kilometers") : _("square kilometer");
	else
	    return plural ? _("kilometers") : _("kilometer");
	break;
	
    case U_ACRES:
	if (square)
	    return plural ? _("acres") : _("acre");
	else
	    return G_get_units_name(G_units(G_database_unit_name(1)),
				    plural, square);
	break;
	
    case U_HECTARES:
	if (square)
	    return plural ? _("hectares") : _("hectare");
	else
	    return G_get_units_name(G_units(G_database_unit_name(1)),
				    plural, square);
	break;
	
    case U_MILES:
	if (square)
	    return plural ? _("square miles") : _("square mile");
	else
	    return plural ? _("miles") : _("mile");
	break;
	
    case U_FEET:
	if (square)
	    return plural ? _("square feet") : _("square foot");
	else
	    return plural ? _("feet") : _("foot");
	break;

    case U_DEGREES:
	if (square)
	    return plural ? _("square degrees") : _("square degree");
	else
	    return plural ? _("degrees") : _("degree");
	break;

    }
    
    return NULL;
}

/*!
  \brief Get units code by name

  Units codes (gis.h):
   - U_METERS
   - U_KILOMETERS
   - U_ACRES
   - U_HECTARES
   - U_MILES
   - U_FEET

  \param units_name units name (singular or plural form)

  \return units code
  \return U_UNKNOWN if not found
*/
int G_units(const char *units_name)
{
    if (units_name == NULL) {
	return G_units(G_database_unit_name(1));
    }
	
    if (strcasecmp(units_name, "meter") == 0 ||
	strcasecmp(units_name, "meters") == 0)
	return U_METERS;
    else if (strcasecmp(units_name, "kilometer") == 0 ||
	     strcasecmp(units_name, "kilometers") == 0)
	return U_KILOMETERS;
    else if (strcasecmp(units_name, "acre") == 0 ||
	     strcasecmp(units_name, "acres") == 0)
	return U_ACRES;
    else if (strcasecmp(units_name, "hectare") == 0 ||
	     strcasecmp(units_name, "hectares") == 0)
	return U_HECTARES;
    else if (strcasecmp(units_name, "mile") == 0 ||
	     strcasecmp(units_name, "miles") == 0)
	return U_MILES;
    else if (strcasecmp(units_name, "foot") == 0 ||
	     strcasecmp(units_name, "feet") == 0)
	return U_FEET;
    else if (strcasecmp(units_name, "degree") == 0 ||
	     strcasecmp(units_name, "degrees") == 0)
	return U_DEGREES;

    return U_UNKNOWN;
}
