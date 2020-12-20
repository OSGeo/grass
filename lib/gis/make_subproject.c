/*!
 * \file lib/gis/make_subproject.c
 *
 * \brief GIS Library - Functions to create a new subproject within an
 * existing project
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
 * \brief Create a new subproject
 * 
 * This function creates a new subproject in the given project,
 * initializes default window and the current window.
 *
 * Calls G_fatal_error() if project doesn't exist.
 *
 * \param gisdbase_name full path of GISDBASE to create subproject in
 *                      (NULL for the current GISDBASE)
 * \param project_name name of project to create subproject in
 *                      (NULL for the current project)
 * \param subproject_name   Name of the new subproject. Should not include
 *                      the full path, the subproject will be created within
 *                      the specified database and project.
 *
 * \return 0 on success
 * \return -1 to indicate a system error (check errno).
 * \return -2 illegal name 
 */
int G_make_subproject(const char *gisdbase_name, const char *project_name,
                  const char *subproject_name)
{
    char path[GPATH_MAX];
    struct Cell_head default_window;

    /* Get project */
    if (project_name == NULL)
	project_name = G_project();

    /* Get GISDBASE */
    if (gisdbase_name == NULL)
	gisdbase_name = G_gisdbase();

    /* TODO: Should probably check that user specified project and gisdbase are valid */

    /* check if subproject name is legal */
    if (G_legal_filename(subproject_name) != 1)
        return -2;
    
    /* Check if project exists */
    sprintf(path, "%s/%s", gisdbase_name, project_name);
    if (access(path, F_OK ) == -1)
        G_fatal_error(_("Project <%s> doesn't exist"), project_name);
    
    /* Make the subproject */
    sprintf(path, "%s/%s/%s", gisdbase_name, project_name, subproject_name);
    if (G_mkdir(path) != 0) {
        perror("G_make_subproject");
	return -1;
    }
    G_create_alt_env();

    /* Get PERMANENT default window */
    G_setenv_nogisrc("GISDBASE", gisdbase_name);
    G_setenv_nogisrc("LOCATION_NAME", project_name);
    G_setenv_nogisrc("MAPSET", "PERMANENT");
    G_get_default_window(&default_window);

    /* Change to the new subproject */
    G_setenv_nogisrc("MAPSET", subproject_name);

    /* Copy default window/regions to new subproject */
    G_put_element_window(&default_window, "", "WIND");

    /* And switch back to original environment */
    G_switch_env();

    return 0;
}

