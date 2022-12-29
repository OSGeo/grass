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
 * Calls G_fatal_error() if location doesn't exist.
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
 * \return -2 illegal name 
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

    /* check if mapset name is legal */
    if (G_legal_filename(mapset_name) != 1)
        return -2;
    
    /* Check if location exists */
    sprintf(path, "%s/%s", gisdbase_name, location_name);
    if (access(path, F_OK ) == -1)
        G_fatal_error(_("Location <%s> doesn't exist"), location_name);
    
    /* Make the mapset */
    sprintf(path, "%s/%s/%s", gisdbase_name, location_name, mapset_name);
    if (G_mkdir(path) != 0) {
        perror("G_make_mapset");
	return -1;
    }
    G_create_alt_env();

    /* Get PERMANENT default window */
    G_setenv_nogisrc("GISDBASE", gisdbase_name);
    G_setenv_nogisrc("LOCATION_NAME", location_name);
    G_setenv_nogisrc("MAPSET", "PERMANENT");
    G_get_default_window(&default_window);

    /* Change to the new mapset */
    G_setenv_nogisrc("MAPSET", mapset_name);

    /* Copy default window/regions to new mapset */
    G_put_element_window(&default_window, "", "WIND");

    /* And switch back to original environment */
    G_switch_env();

    return 0;
}

