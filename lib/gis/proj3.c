#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static int lookup(const char *, const char *, char *, int);
static int equal(const char *, const char *);
static int lower(char);


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

char *G_database_unit_name(int plural)
{
    int n;
    static char name[256];

    switch (n = G_projection()) {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
    case PROJECTION_SP:
	return G__unit_name(G__projection_units(n), plural);
    }

    if (!lookup(UNIT_FILE, plural ? "units" : "unit", name, sizeof(name)))
	strcpy(name, plural ? "units" : "unit");
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

char *G_database_projection_name(void)
{
    int n;
    static char name[256];

    switch (n = G_projection()) {
    case PROJECTION_XY:
    case PROJECTION_UTM:
    case PROJECTION_LL:
    case PROJECTION_SP:
	return G__projection_name(n);
    }
    if (!lookup(PROJECTION_FILE, "name", name, sizeof(name)))
	strcpy(name, _("Unknown projection"));
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
    char *unit;
    double factor;
    char buf[256];
    int n;

    static struct
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
    if (lookup(UNIT_FILE, "meters", buf, sizeof(buf)))
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

char *G_database_datum_name(void)
{
    static char name[256], params[256];
    struct Key_Value *projinfo;
    int datumstatus;

    if (lookup(PROJECTION_FILE, "datum", name, sizeof(name)))
	return name;
    else if ((projinfo = G_get_projinfo()) == NULL)
	return NULL;
    else
	datumstatus = G_get_datumparams_from_projinfo(projinfo, name, params);

    G_free_key_value(projinfo);
    if (datumstatus == 2)
	return params;
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

char *G_database_ellipse_name(void)
{
    static char name[256];

    if (!lookup(PROJECTION_FILE, "ellps", name, sizeof(name))) {
	double a, es;

	G_get_ellipsoid_parameters(&a, &es);
	sprintf(name, "a=%.16g es=%.16g", a, es);
    }

    /* strcpy (name, "Unknown ellipsoid"); */
    return name;
}

static int lookup(const char *file, const char *key, char *value, int len)
{
    char path[GPATH_MAX];

    /*
       G__file_name (path, "", file, G_mapset());
       if (access(path,0) == 0)
       return G_lookup_key_value_from_file(path, key, value, len) == 1;
     */
    G__file_name(path, "", file, "PERMANENT");
    return G_lookup_key_value_from_file(path, key, value, len) == 1;
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
