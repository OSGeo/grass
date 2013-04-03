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
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

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
 * \returns 0 on success
 * \returns -1 to indicate a system error (check errno).
 */
int G_make_location(const char *location_name,
                    struct Cell_head *wind,
                    const struct Key_Value *proj_info,
                    const struct Key_Value *proj_units)
{
    char path[GPATH_MAX];

    /* Try to create the location directory, under the gisdbase. */
    sprintf(path, "%s/%s", G_gisdbase(), location_name);
    if (G_mkdir(path) != 0)
	return -1;

    /* Make the PERMANENT mapset. */
    sprintf(path, "%s/%s/%s", G_gisdbase(), location_name, "PERMANENT");
    if (G_mkdir(path) != 0) {
        perror("G_make_location");
	return -1;
    }

    /* make these the new current location and mapset */
    G__setenv("LOCATION_NAME", location_name);
    G__setenv("MAPSET", "PERMANENT");

    /* Create the default, and current window files */
    G__put_window(wind, "", "DEFAULT_WIND");
    G__put_window(wind, "", "WIND");

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
 * \brief Compare projections including units
 *
 *  \param proj_info1   projection info to compare
 *  \param proj_units1  projection units to compare
 *  \param proj_info2   projection info to compare
 *  \param proj_units2  projection units to compare

 *  \return -1 if not the same projection
 *  \return -2 if linear unit translation to meters fails
 *  \return -4 if not the same ellipsoid,
 *  \return -5 if UTM zone differs
 *  \return -6 if UTM hemisphere differs,
 *  \return -7 if false easting differs
 *  \return -8 if false northing differs,
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

    /* -------------------------------------------------------------------- */
    /*      Do they both have the same ellipsoid?                           */
    /*      Lets just check the semi-major axis for now to keep it simple   */
    /* -------------------------------------------------------------------- */

    {
	double a1 = 0, a2 = 0;

	if (G_find_key_value("a", proj_info1) != NULL)
	    a1 = atof(G_find_key_value("a", proj_info1));
	if (G_find_key_value("a", proj_info2) != NULL)
	    a2 = atof(G_find_key_value("a", proj_info2));

	if (a1 && a2 && (fabs(a2 - a1) > 0.000001))
	    return -4;
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

	if (y_0_1 && y_0_2 && (fabs(atof(y_0_1) - atof(y_0_2)) > 0.000001))
	    return -8;
    }

    /* -------------------------------------------------------------------- */
    /*      Add more details in later.                                      */
    /* -------------------------------------------------------------------- */

    return 1;
}
