#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static const char *lookup_proj(const char *);
static const char *lookup_units(const char *);
static int equal(const char *, const char *);
static int lower(char);

static int initialized;
static struct Key_Value *proj_info, *proj_units;

static void init(void)
{
    if (G_is_initialized(&initialized))
	return;

    proj_info = G_get_projinfo();
    proj_units = G_get_projunits();

    G_initialize_done(&initialized);
}

/*!
 * \brief database units
 *
 * Returns a
 * string describing the database grid units. It returns a plural form (eg. feet)
 * if <b>plural</b> is true. Otherwise it returns a singular form (eg. foot).
 *
 *  \param plural
 *  \return char * 
 */

const char *G_database_unit_name(int plural)
{
    int n;
    const char *name;

    switch (n = G_projection()) {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
    case PROJECTION_SP:
	return G__unit_name(G__projection_units(n), plural);
    }

    name = lookup_units(plural ? "units" : "unit");
    if (!name)
	return plural ? "units" : "unit";

    return name;
}


/*!
 * \brief query cartographic projection
 *
 * Returns a pointer to a string which is a printable name for
 * projection code <b>proj</b> (as returned by <i>G_projection</i>). Returns
 * NULL if <b>proj</b> is not a valid projection.
 *
 *  \param proj
 *  \return char * 
 */

const char *G_database_projection_name(void)
{
    int n;
    const char *name;

    switch (n = G_projection()) {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
    case PROJECTION_SP:
	return G__projection_name(n);
    }

    name = lookup_proj("name");
    if (!name)
	return _("Unknown projection");

    return name;
}


/*!
 * \brief conversion to meters
 *
 * Returns a factor which converts the grid unit to meters (by
 * multiplication).  If the database is not metric (eg. imagery) then 0.0 is
 * returned.
 *
 *  \param void
 *  \return double
 */

double G_database_units_to_meters_factor(void)
{
    const char *unit;
    const char *buf;
    double factor;
    int n;

    static const struct
    {
	char *unit;
	double factor;
    } table[] = {
	{"unit",  1.0},
	{"meter", 1.0},
	{"foot", .3048},
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

/***********************************************************************
 * G_database_datum_name(void)
 *
 * return name of datum of current database
 *
 * returns pointer to valid name if ok
 * NULL otherwise
 ***********************************************************************/


/*!
 * \brief get datum name for database
 *
 * Returns a pointer to the name of the map datum of the current database. If 
 * there is no map datum explicitely associated with the acutal database, the 
 * standard map datum WGS84 is returned, on error a NULL pointer is returned. 
 *
 *  \return char * 
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

/***********************************************************************
 * G_database_ellipse_name(void)
 *
 * return name of ellipsoid of current database
 *
 * returns pointer to valid name if ok
 * NULL otherwise
 ***********************************************************************/

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

static const char *lookup_proj(const char *key)
{
    init();
    return G_find_key_value(key, proj_info);
}

static const char *lookup_units(const char *key)
{
    init();
    return G_find_key_value(key, proj_units);
}

static int equal(const char *a, const char *b)
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

static int lower(char c)
{
    if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
    return c;
}
