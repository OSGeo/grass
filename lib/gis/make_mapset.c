/*!
 * \file lib/gis/make_mapset.c
 *
 * \brief GIS Library - Functions to create a new mapset within an
 * existing location
 *
 * (C) 2006-2013 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Joel Pitt, joel.pitt@gmail.com
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Create a new mapset
 * 
 * This function creates a new mapset in the given location,
 * initializes default window and the current window.
 *
 * \param gisdbase_name full path of GISDBASE to create mapset in
 *                      (NULL for the current GISDBASE)
 * \param location_name name of location to create mapset in
 *                      (NULL for the current location)
 * \param mapset_name   Name of the new mapset. Should not include
 *                      the full path, the mapset will be created within
 *                      the specified database and location.
 *
 * \return 0 on success
 * \return -1 to indicate a system error (check errno).
 */
int G_make_mapset(const char *gisdbase_name, const char *location_name,
                  const char *mapset_name)
{
    char path[GPATH_MAX];
    struct Cell_head default_window;

    /* Get location */
    if (location_name == NULL)
	location_name = G_location();

    /* Get GISDBASE */
    if (gisdbase_name == NULL)
	gisdbase_name = G_gisdbase();

    /* TODO: Should probably check that user specified location and gisdbase are valid */

    if (G_legal_filename(mapset_name) != 1)
        return -1;
    
    /* Make the mapset. */
    sprintf(path, "%s/%s/%s", gisdbase_name, location_name, mapset_name);
    if (G_mkdir(path) != 0) {
        perror("G_make_mapset");
	return -1;
    }
    G__create_alt_env();

    /* Get PERMANENT default window */
    G__setenv("GISDBASE", gisdbase_name);
    G__setenv("LOCATION", location_name);
    G__setenv("MAPSET", "PERMANENT");
    G_get_default_window(&default_window);

    /* Change to the new mapset */
    G__setenv("MAPSET", mapset_name);

    /* Copy default window/regions to new mapset */
    G__put_window(&default_window, "", "WIND");

    /* And switch back to original environment */
    G__switch_env();

    return 0;
}

