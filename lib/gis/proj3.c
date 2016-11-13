/*!
  \file lib/gis/proj3.c

  \brief GIS Library - Projection support (database)
  
  (C) 2001-2014 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static const char *lookup_proj(const char *);
static const char *lookup_units(const char *);
static const char *lookup_epsg();
static int equal(const char *, const char *);
static int lower(char);

static int initialized;
static struct Key_Value *proj_info, *proj_units, *proj_epsg;

static void init(void)
{
    if (G_is_initialized(&initialized))
	return;

    proj_info = G_get_projinfo();
    proj_units = G_get_projunits();
    proj_epsg = G_get_projepsg();

    G_initialize_done(&initialized);
}

/*!
  \brief Get units (localized) name for the current location
  
  Returns a string describing the database grid units. It returns a
  plural form (eg. 'feet') if <i>plural</i> is non-zero. Otherwise it
  returns a singular form (eg. 'foot').
  
  \param plural plural form if non-zero
  
  \return localized units name
*/
const char *G_database_unit_name(int plural)
{
    int units;
    units = G_database_unit();
    return G_get_units_name(units, plural, FALSE);
}

/*!
  \brief Get units id for the current location
  
  \return units id
*/
int G_database_unit()
{
    int units;
    const char *name;
    
    units = G_projection_units(G_projection());
    
    if (units == U_UNDEFINED) {
	name = lookup_units("unit");
	if (!name)
	    return U_UNKNOWN;
	
	if (strcasecmp(name, "meter") == 0 || strcasecmp(name, "metre") == 0 
            || strcasecmp(name, "meters") == 0 || strcasecmp(name, "metres") == 0)
	    units = U_METERS;
	else if (strcasecmp(name, "kilometer") == 0 || strcasecmp(name, "kilometre") == 0
                 || strcasecmp(name, "kilometers") == 0 || strcasecmp(name, "kilometres") == 0)
	    units = U_KILOMETERS;
	else if (strcasecmp(name, "acre") == 0 || strcasecmp(name, "acres") == 0)
	    units = U_ACRES;
	else if (strcasecmp(name, "hectare") == 0 || strcasecmp(name, "hectares") == 0)
	    units = U_HECTARES;
	else if (strcasecmp(name, "mile") == 0 || strcasecmp(name, "miles") == 0)
	    units = U_MILES;
	else if (strcasecmp(name, "foot") == 0 || strcasecmp(name, "feet") == 0)
	    units = U_FEET;
	else if (strcasecmp(name, "foot_us") == 0 || strcasecmp(name, "foot_uss") == 0)
	    units = U_USFEET;
	else if (strcasecmp(name, "degree") == 0 || strcasecmp(name, "degrees") == 0)
	    units = U_DEGREES;
	else
	    units = U_UNKNOWN;
    }
    return units;
}

/*!
  \brief Query cartographic projection for the current location
  
  Returns a pointer to a string which is a printable name for
  projection code <i>proj</i> (as returned by G_projection). Returns
  NULL if <i>proj</i> is not a valid projection.
  
  \return projection name
*/
const char *G_database_projection_name(void)
{
    int n;
    const char *name;

    switch (n = G_projection()) {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
	return G_projection_name(n);
    }

    name = lookup_proj("name");
    if (!name)
	return _("Unknown projection");

    return name;
}

/*!
  \brief Conversion to meters
  
  Returns a factor which converts the grid unit to meters (by
  multiplication). If the database is not metric (eg. imagery) then
  0.0 is returned.
  
  \return value
*/
double G_database_units_to_meters_factor(void)
{
    const char *unit;
    const char *buf;
    double factor;
    int n;

    /* TODO: sync with definitions in ../proj/units.table */
    static const struct
    {
	char *unit;
	double factor;
    } table[] = {
	{"unit",  1.0},
	{"meter", 1.0},
	{"foot",  .3048},
	{"foot_us", 1200/3937.},
	{"inch", .0254},
	{NULL, 0.0}
    };

    factor = 0.0;
    buf = lookup_units("meters");
    if (buf)
	sscanf(buf, "%lf", &factor);
    if (factor <= 0.0) {
	unit = G_database_unit_name(0);
	for (n = 0; table[n].unit; n++)
	    if (equal(unit, table[n].unit)) {
		factor = table[n].factor;
		break;
	    }
    }
    return factor;
}

/*!
  \brief Get datum name for the current location
  
  Returns a pointer to the name of the map datum of the current
  database. If there is no map datum explicitely associated with the
  actual database, the standard map datum WGS84 is returned, on error
  a NULL pointer is returned.
  
  \return datum name
*/
const char *G_database_datum_name(void)
{
    const char *name;
    char buf[256], params[256];
    int datumstatus;

    name = lookup_proj("datum");
    if (name)
	return name;
    else if (!proj_info)
	return NULL;
    else
	datumstatus = G_get_datumparams_from_projinfo(proj_info, buf, params);

    if (datumstatus == 2)
	return G_store(params);
    else
	return NULL;
}

/*!
  \brief Get ellipsoid name for the current location
  
  \return pointer to valid name if ok
  \return NULL on error
*/
const char *G_database_ellipse_name(void)
{
    const char *name;

    name = lookup_proj("ellps");
    if (!name) {
	char buf[256];
	double a, es;

	G_get_ellipsoid_parameters(&a, &es);
	sprintf(buf, "a=%.16g es=%.16g", a, es);
	name = G_store(buf);
    }

    /* strcpy (name, "Unknown ellipsoid"); */
    return name;
}

/*!
  \brief Get EPGS code for the current location
  
  \return pointer to valid EPSG code on success
  \return NULL on error
*/
const char *G_database_epsg_code(void)
{
    return lookup_epsg();
}

const char *lookup_proj(const char *key)
{
    init();
    return G_find_key_value(key, proj_info);
}

const char *lookup_units(const char *key)
{
    init();
    return G_find_key_value(key, proj_units);
}

const char *lookup_epsg()
{
    init();
    return G_find_key_value("epsg", proj_epsg);
}

int equal(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
	return a == b;
    while (*a && *b)
	if (lower(*a++) != lower(*b++))
	    return 0;
    if (*a || *b)
	return 0;
    return 1;
}

int lower(char c)
{
    if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
    return c;
}
