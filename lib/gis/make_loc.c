
/******************************************************************************
 *
 * Project:  libgrass
 * Purpose:  Function to create a new location automatically given a 
 *           "Cell_head", PROJ_INFO and PROJ_UNITS information.
 * Author:   Frank Warmerdam, warmerda@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************
 *
 */

#include <grass/gis.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

/*
 * Returns 0 on success.
 * Returns -1 to indicate a system error (check errno).
 */


int G__make_location(const char *location_name,
		     struct Cell_head *wind,
		     struct Key_Value *proj_info,
		     struct Key_Value *proj_units, FILE * report_file)
{
    char path[GPATH_MAX];

    /* Try to create the location directory, under the gisdbase. */
    sprintf(path, "%s/%s", G_gisdbase(), location_name);
    if (G_mkdir(path) != 0)
	return -1;

    /* Make the PERMANENT mapset. */
    sprintf(path, "%s/%s/%s", G_gisdbase(), location_name, "PERMANENT");
    if (G_mkdir(path) != 0)
	return -1;

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
 * \brief  create a new location
 * 
 * This function creates a new location in the current database,
 * initializes the projection, default window and current window.  
 *
 * \param location_name
 *                      The name of the new location.  Should not include
 *                      the full path, the location will be created within
 *                      the current database.
 * \param wind
 *                      Contains the default window setting for the
 *                      new location.  All fields should be set in this
 *                      structure, and care should be taken to ensure that
 *                      the proj/zone fields match the definition in the
 *                      proj_info parameter (see G_set_cellhd_from_projinfo()).
 *
 * \param proj_info
 *                      Projection definition suitable to write to the
 *                      PROJ_INFO file, or NULL for PROJECTION_XY.
 *
 * \param proj_units
 *                      Projection units suitable to write to the PROJ_UNITS
 *                      file, or NULL.
 *
 * \param report_file 
 *                      File to which creation information should be written
 *                      (can be stdout).  Currently not used.
 *
 * \return Returns 0 on success, or generates a fatal error on failure.  
 *         The G__make_location() function operates the same, but returns a
 *         non-zero error code on failure, instead of terminating. 
 */

int G_make_location(const char *location_name,
		    struct Cell_head *wind,
		    struct Key_Value *proj_info,
		    struct Key_Value *proj_units, FILE * report_file)
{
    int err;

    err = G__make_location(location_name, wind, proj_info, proj_units,
			   report_file);

    if (err == 0)
	return 0;

    if (err == -1) {
	perror("G_make_location");
    }

    G_fatal_error("G_make_location failed.");

    return 1;
}


/************************************************************************/
/*                       G_compare_projections()                        */

/************************************************************************/

/*!
 * \brief compare projections
 *
 *  \param proj_info1
 *  \param proj_units1
 *  \param proj_info2
 *  \param proj_units2
 *  \return -1 if not the same projection, -2 if linear unit translation to 
 *          meters fails, -4 if not the same ellipsoid,
 *          -5 if UTM zone differs, -6 if UTM hemisphere differs,
 *          -7 if false easting differs, -8 if false northing differs,
 *          else TRUE if projections match.
 *          
 */

int
G_compare_projections(const struct Key_Value *proj_info1,
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
	return TRUE;

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

    return TRUE;
}
