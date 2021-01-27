/*!
 * \file lib/gis/make_loc.c
 *
 * \brief GIS Library - Functions to create a new location
 *
 * Creates a new location automatically given a "Cell_head", PROJ_INFO
 * and PROJ_UNITS information.
 *
 * (C) 2000-2013 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Frank Warmerdam
 */

#include <grass/gis.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <grass/glocale.h>

/*!
 * \brief Create a new location
 *
 * This function creates a new location in the current database,
 * initializes the projection, default window and current window.
 *
 * \param location_name Name of the new location. Should not include
 *                      the full path, the location will be created within
 *                      the current database.
 * \param wind          default window setting for the new location.
 *                      All fields should be set in this
 *                      structure, and care should be taken to ensure that
 *                      the proj/zone fields match the definition in the
 *                      proj_info parameter(see G_set_cellhd_from_projinfo()).
 *
 * \param proj_info     projection definition suitable to write to the
 *                      PROJ_INFO file, or NULL for PROJECTION_XY.
 *
 * \param proj_units    projection units suitable to write to the PROJ_UNITS
 *                      file, or NULL.
 *
 * \return 0 on success
 * \return -1 to indicate a system error (check errno).
 * \return -2 failed to create projection file (currently not used)
 * \return -3 illegal name
 */
int G_make_location(const char *location_name,
                    struct Cell_head *wind,
                    const struct Key_Value *proj_info,
                    const struct Key_Value *proj_units)
{
    char path[GPATH_MAX];

    /* check if location name is legal */
    if (G_legal_filename(location_name) != 1)
        return -3;

    /* Try to create the location directory, under the gisdbase. */
    sprintf(path, "%s/%s", G_gisdbase(), location_name);
    if (G_mkdir(path) != 0)
	return -1;

    /* Make the PERMANENT mapset. */
    sprintf(path, "%s/%s/%s", G_gisdbase(), location_name, "PERMANENT");
    if (G_mkdir(path) != 0) {
	return -1;
    }

    /* make these the new current location and mapset */
    G_setenv_nogisrc("LOCATION_NAME", location_name);
    G_setenv_nogisrc("MAPSET", "PERMANENT");

    /* Create the default, and current window files */
    G_put_element_window(wind, "", "DEFAULT_WIND");
    G_put_element_window(wind, "", "WIND");

    /* Write out the PROJ_INFO, and PROJ_UNITS if available. */
    if (proj_info != NULL) {
	G_file_name(path, "", "PROJ_INFO", "PERMANENT");
	G_write_key_value_file(path, proj_info);
    }

    if (proj_units != NULL) {
	G_file_name(path, "", "PROJ_UNITS", "PERMANENT");
	G_write_key_value_file(path, proj_units);
    }

    return 0;
}

/*!
 * \brief Create a new location
 *
 * This function creates a new location in the current database,
 * initializes the projection, default window and current window,
 * and sets the EPSG code if present
 *
 * \param location_name Name of the new location. Should not include
 *                      the full path, the location will be created within
 *                      the current database.
 * \param wind          default window setting for the new location.
 *                      All fields should be set in this
 *                      structure, and care should be taken to ensure that
 *                      the proj/zone fields match the definition in the
 *                      proj_info parameter(see G_set_cellhd_from_projinfo()).
 *
 * \param proj_info     projection definition suitable to write to the
 *                      PROJ_INFO file, or NULL for PROJECTION_XY.
 *
 * \param proj_units    projection units suitable to write to the PROJ_UNITS
 *                      file, or NULL.
 *
 * \param proj_epsg     EPSG code suitable to write to the PROJ_EPSG
 *                      file, or NULL.
 *
 * \return 0 on success
 * \return -1 to indicate a system error (check errno).
 * \return -2 failed to create projection file (currently not used)
 * \return -3 illegal name
 */
int G_make_location_epsg(const char *location_name,
			 struct Cell_head *wind,
			 const struct Key_Value *proj_info,
			 const struct Key_Value *proj_units,
			 const struct Key_Value *proj_epsg)
{
    int ret;
    char path[GPATH_MAX];

    ret = G_make_location(location_name, wind, proj_info, proj_units);

    if (ret != 0)
	return ret;

    /* Write out the PROJ_EPSG if available. */
    if (proj_epsg != NULL) {
	G_file_name(path, "", "PROJ_EPSG", "PERMANENT");
	G_write_key_value_file(path, proj_epsg);
    }

    return 0;
}

/*!
 * \brief Create a new location
 *
 * This function creates a new location in the current database,
 * initializes the projection, default window and current window,
 * and sets WKT, srid, and EPSG code if present
 *
 * \param location_name Name of the new location. Should not include
 *                      the full path, the location will be created within
 *                      the current database.
 * \param wind          default window setting for the new location.
 *                      All fields should be set in this
 *                      structure, and care should be taken to ensure that
 *                      the proj/zone fields match the definition in the
 *                      proj_info parameter(see G_set_cellhd_from_projinfo()).
 *
 * \param proj_info     projection definition suitable to write to the
 *                      PROJ_INFO file, or NULL for PROJECTION_XY.
 *
 * \param proj_units    projection units suitable to write to the PROJ_UNITS
 *                      file, or NULL.
 *
 * \param proj_epsg     EPSG code suitable to write to the PROJ_EPSG
 *                      file, or NULL.
 *
 * \param proj_wkt      WKT defintion suitable to write to the PROJ_WKT
 *                      file, or NULL.
 *
 * \param proj_srid     Spatial reference ID suitable to write to the PROJ_SRID
 *                      file, or NULL.
 *
 * \return 0 on success
 * \return -1 to indicate a system error (check errno).
 * \return -2 failed to create projection file (currently not used)
 * \return -3 illegal name
 */
int G_make_location_crs(const char *location_name,
			struct Cell_head *wind,
			const struct Key_Value *proj_info,
			const struct Key_Value *proj_units,
			const char *proj_srid,
			const char *proj_wkt)
{
    int ret;
    char path[GPATH_MAX];

    ret = G_make_location(location_name, wind, proj_info, proj_units);

    if (ret != 0)
	return ret;

    /* Write out PROJ_SRID if srid is available. */
    if (proj_srid != NULL) {
	G_write_projsrid(location_name, proj_srid);
    }

    /* Write out PROJ_WKT if WKT is available. */
    if (proj_wkt != NULL) {
	G_write_projwkt(location_name, proj_wkt);
    }

    return 0;
}

/*!
 * \brief Compare projections including units
 *
 *  \param proj_info1   projection info to compare
 *  \param proj_units1  projection units to compare
 *  \param proj_info2   projection info to compare
 *  \param proj_units2  projection units to compare

 *  \return -1 if not the same projection
 *  \return -2 if linear unit translation to meters fails
 *  \return -3 if not the same datum,
 *  \return -4 if not the same ellipsoid,
 *  \return -5 if UTM zone differs
 *  \return -6 if UTM hemisphere differs,
 *  \return -7 if false easting differs
 *  \return -8 if false northing differs,
 *  \return -9 if center longitude differs,
 *  \return -10 if center latitude differs,
 *  \return -11 if standard parallels differ,
 *  \return 1  if projections match.
 */
int G_compare_projections(const struct Key_Value *proj_info1,
                          const struct Key_Value *proj_units1,
                          const struct Key_Value *proj_info2,
                          const struct Key_Value *proj_units2)
{
    const char *proj1, *proj2;

    if (proj_info1 == NULL && proj_info2 == NULL)
	return TRUE;

    /* -------------------------------------------------------------------- */
    /*      Are they both in the same projection?                           */
    /* -------------------------------------------------------------------- */
    /* prevent seg fault in G_find_key_value */
    if (proj_info1 == NULL || proj_info2 == NULL)
	return -1;

    proj1 = G_find_key_value("proj", proj_info1);
    proj2 = G_find_key_value("proj", proj_info2);

    if (proj1 == NULL || proj2 == NULL || strcmp(proj1, proj2))
	return -1;

    /* -------------------------------------------------------------------- */
    /*      Verify that the linear unit translation to meters is OK.        */
    /* -------------------------------------------------------------------- */
    /* prevent seg fault in G_find_key_value */
    if (proj_units1 == NULL && proj_units2 == NULL)
	return 1;

    if (proj_units1 == NULL || proj_units2 == NULL)
	return -2;

    {
	double a1 = 0, a2 = 0;

	if (G_find_key_value("meters", proj_units1) != NULL)
	    a1 = atof(G_find_key_value("meters", proj_units1));
	if (G_find_key_value("meters", proj_units2) != NULL)
	    a2 = atof(G_find_key_value("meters", proj_units2));

	if (a1 && a2 && (fabs(a2 - a1) > 0.000001))
	    return -2;
    }
    /* compare unit name only if there is no to meter conversion factor */
    if (G_find_key_value("meters", proj_units1) == NULL ||
        G_find_key_value("meters", proj_units2) == NULL) {
	const char *u_1 = NULL, *u_2 = NULL;

	u_1 = G_find_key_value("unit", proj_units1);
	u_2 = G_find_key_value("unit", proj_units2);

	if ((u_1 && !u_2) || (!u_1 && u_2))
	    return -2;

	/* the unit name can be arbitrary: the following can be the same
	 * us-ft			(proj.4 keyword)
	 * U.S. Surveyor's Foot	(proj.4 name)
	 * US survey foot		(WKT)
	 * Foot_US			(WKT)
	 */
	if (u_1 && u_2 && G_strcasecmp(u_1, u_2))
	    return -2;
    }

    /* -------------------------------------------------------------------- */
    /*      Do they both have the same datum?                               */
    /* -------------------------------------------------------------------- */
    {
	const char *d_1 = NULL, *d_2 = NULL;

	d_1 = G_find_key_value("datum", proj_info1);
	d_2 = G_find_key_value("datum", proj_info2);

	if ((d_1 && !d_2) || (!d_1 && d_2))
	    return -3;

	if (d_1 && d_2 && strcmp(d_1, d_2)) {
	    /* different datum short names can mean the same datum,
	     * see lib/gis/datum.table */
	    G_debug(1, "Different datum names");
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Do they both have the same ellipsoid?                           */
    /* -------------------------------------------------------------------- */
    {
	const char *e_1 = NULL, *e_2 = NULL;

	e_1 = G_find_key_value("ellps", proj_info1);
	e_2 = G_find_key_value("ellps", proj_info2);

	if (e_1 && e_2 && strcmp(e_1, e_2))
	    return -4;

	if (e_1 == NULL || e_2 == NULL) {
	    double a1 = 0, a2 = 0;
	    double es1 = 0, es2 = 0;

	    /* it may happen that one proj_info has ellps,
	     * while the other has a, es: translate ellps to a, es */
	    if (e_1)
		G_get_ellipsoid_by_name(e_1, &a1, &es1);
	    else {
		if (G_find_key_value("a", proj_info1) != NULL)
		    a1 = atof(G_find_key_value("a", proj_info1));
		if (G_find_key_value("es", proj_info1) != NULL)
		    es1 = atof(G_find_key_value("es", proj_info1));
	    }

	    if (e_2)
		G_get_ellipsoid_by_name(e_2, &a2, &es2);
	    else {
		if (G_find_key_value("a", proj_info2) != NULL)
		    a2 = atof(G_find_key_value("a", proj_info2));
		if (G_find_key_value("es", proj_info2) != NULL)
		    es2 = atof(G_find_key_value("es", proj_info2));
	    }

	    /* it should be an error if a = 0 */
	    if ((a1 == 0 && a2 != 0) || (a1 != 0 && a2 == 0))
		return -4;

	    if (a1 && a2 && (fabs(a2 - a1) > 0.000001))
		return -4;

	    if ((es1 == 0 && es2 != 0) || (es1 != 0 && es2 == 0))
		return -4;

	    if (es1 && es2 && (fabs(es2 - es1) > 0.000001))
		return -4;
	}
    }

    /* -------------------------------------------------------------------- */
    /*      Zone check specially for UTM                                    */
    /* -------------------------------------------------------------------- */
    if (!strcmp(proj1, "utm") && !strcmp(proj2, "utm")
	&& atof(G_find_key_value("zone", proj_info1))
	!= atof(G_find_key_value("zone", proj_info2)))
	return -5;

    /* -------------------------------------------------------------------- */
    /*      Hemisphere check specially for UTM                              */
    /* -------------------------------------------------------------------- */
    if (!strcmp(proj1, "utm") && !strcmp(proj2, "utm")
	&& !!G_find_key_value("south", proj_info1)
	!= !!G_find_key_value("south", proj_info2))
	return -6;

    /* -------------------------------------------------------------------- */
    /*      Do they both have the same false easting?                       */
    /* -------------------------------------------------------------------- */

    {
	const char *x_0_1 = NULL, *x_0_2 = NULL;

	x_0_1 = G_find_key_value("x_0", proj_info1);
	x_0_2 = G_find_key_value("x_0", proj_info2);

	if ((x_0_1 && !x_0_2) || (!x_0_1 && x_0_2))
	    return -7;

	if (x_0_1 && x_0_2 && (fabs(atof(x_0_1) - atof(x_0_2)) > 0.000001))
	    return -7;
    }

    /* -------------------------------------------------------------------- */
    /*      Do they both have the same false northing?                       */
    /* -------------------------------------------------------------------- */

    {
	const char *y_0_1 = NULL, *y_0_2 = NULL;

	y_0_1 = G_find_key_value("y_0", proj_info1);
	y_0_2 = G_find_key_value("y_0", proj_info2);

	if ((y_0_1 && !y_0_2) || (!y_0_1 && y_0_2))
	    return -8;

	if (y_0_1 && y_0_2 && (fabs(atof(y_0_1) - atof(y_0_2)) > 0.000001))
	    return -8;
    }

    /* -------------------------------------------------------------------- */
    /*      Do they have the same center longitude?                         */
    /* -------------------------------------------------------------------- */

    {
	const char *l_1 = NULL, *l_2 = NULL;

	l_1 = G_find_key_value("lon_0", proj_info1);
	l_2 = G_find_key_value("lon_0", proj_info2);

	if ((l_1 && !l_2) || (!l_1 && l_2))
	    return -9;

	if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001))
	    return -9;

    /* -------------------------------------------------------------------- */
    /*      Do they have the same center latitude?                          */
    /* -------------------------------------------------------------------- */

	l_1 = l_2 = NULL;
	l_1 = G_find_key_value("lat_0", proj_info1);
	l_2 = G_find_key_value("lat_0", proj_info2);

	if ((l_1 && !l_2) || (!l_1 && l_2))
	    return -10;

	if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001))
	    return -10;

    /* -------------------------------------------------------------------- */
    /*      Do they have the same standard parallels?                       */
    /* -------------------------------------------------------------------- */

	l_1 = l_2 = NULL;
	l_1 = G_find_key_value("lat_1", proj_info1);
	l_2 = G_find_key_value("lat_1", proj_info2);

	if ((l_1 && !l_2) || (!l_1 && l_2))
	    return -11;

	if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001)) {
	    /* lat_1 differ */
	    /* check for swapped lat_1, lat_2 */
	    l_2 = G_find_key_value("lat_2", proj_info2);

	    if (!l_2)
		return -11;
	    if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001)) {
		return -11;
	    }
	}

	l_1 = l_2 = NULL;
	l_1 = G_find_key_value("lat_2", proj_info1);
	l_2 = G_find_key_value("lat_2", proj_info2);

	if ((l_1 && !l_2) || (!l_1 && l_2))
	    return -11;

	if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001)) {
	    /* lat_2 differ */
	    /* check for swapped lat_1, lat_2 */
	    l_2 = G_find_key_value("lat_1", proj_info2);

	    if (!l_2)
		return -11;
	    if (l_1 && l_2 && (fabs(atof(l_1) - atof(l_2)) > 0.000001)) {
		return -11;
	    }
	}
    }

    /* towgs84 ? */

    /* -------------------------------------------------------------------- */
    /*      Add more details in later.                                      */
    /* -------------------------------------------------------------------- */

    return 1;
}

/*!
   \brief Write WKT definition to file

   Any WKT string and version recognized by PROJ is supported.

   \param location_name name of the location to write the WKT definition
   \param wktstring pointer to WKT string

   \return 0 success
   \return -1 error writing
 */

int G_write_projwkt(const char *location_name, const char *wktstring)
{
    FILE *fp;
    char path[GPATH_MAX];
    int err, n;

    if (!wktstring)
	return 0;

    if (location_name && *location_name)
	sprintf(path, "%s/%s/%s/%s", G_gisdbase(), location_name, "PERMANENT", WKT_FILE);
    else
	G_file_name(path, "", WKT_FILE, "PERMANENT");

    fp = fopen(path, "w");

    if (!fp)
	G_fatal_error(_("Unable to open output file <%s>: %s"), path, strerror(errno));

    err = 0;
    n = strlen(wktstring);
    if (wktstring[n - 1] != '\n') {
	if (n != fprintf(fp, "%s\n", wktstring))
	    err = -1;
    }
    else {
	if (n != fprintf(fp, "%s", wktstring))
	    err = -1;
    }

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing output file <%s>: %s"), path, strerror(errno));

    return err;
}


/*!
   \brief Write srid (spatial reference id) to file

    A srid consists of an authority name and code and must be known to
    PROJ.

   \param location_name name of the location to write the srid
   \param sridstring pointer to srid string

   \return 0 success
   \return -1 error writing
 */

int G_write_projsrid(const char *location_name, const char *sridstring)
{
    FILE *fp;
    char path[GPATH_MAX];
    int err, n;

    if (!sridstring)
	return 0;

    if (location_name && *location_name)
	sprintf(path, "%s/%s/%s/%s", G_gisdbase(), location_name, "PERMANENT", SRID_FILE);
    else
	G_file_name(path, "", SRID_FILE, "PERMANENT");

    fp = fopen(path, "w");

    if (!fp)
	G_fatal_error(_("Unable to open output file <%s>: %s"), path, strerror(errno));

    err = 0;
    n = strlen(sridstring);
    if (sridstring[n - 1] != '\n') {
	if (n != fprintf(fp, "%s\n", sridstring))
	    err = -1;
    }
    else {
	if (n != fprintf(fp, "%s", sridstring))
	    err = -1;
    }

    if (fclose(fp) != 0)
	G_fatal_error(_("Error closing output file <%s>: %s"), path, strerror(errno));

    return err;
}
