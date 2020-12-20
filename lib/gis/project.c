/*!
  \file lib/gis/project.c
  
  \brief GIS library - environment routines (project)
  
  (C) 2001-2008, 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

/*!
  \brief Get current project name
  
  Returns the name of the current database project. This routine
  should be used by modules that need to display the current project
  to the user. See Projects for an explanation of projects.
  
  \return project name
*/
const char *G_project(void)
{
    return G_getenv("LOCATION_NAME");
}

/*!
  \brief Get current project UNIX-like path
 
  Allocated buffer should be freed by G_free(). See
  G__project_path().

  Returns the full UNIX path name of the current database
  project. For example, if the user is working in project
  <i>spearfish</i> in the <i>/home/user/grassdata</i> database
  directory, this routine will return a string which looks like
  <i>/home/user/grassdata/spearfish</i>.
  
  This function also checks if project path is readable by the
  current user. It calls G_fatal_error() on failure.

  \return buffer with project path
 */
char *G_project_path(void)
{
    char *project;

    project = G__project_path();
    if (access(project, F_OK) != 0) {
	perror("access");
	G_fatal_error(_("LOCATION <%s> not available"), project);
    }

    return project;
}


/*!
  \brief Get current project UNIX-like path (internal use only)
  
  Allocated buffer should be freed by G_free(). See also
  G_project_path().
  
  \todo Support also Windows-like path (?)
  
  \return buffer with project path
 */
char *G__project_path(void)
{
    const char *name = G_project();
    const char *base = G_gisdbase();
    char *project = G_malloc(strlen(base) + strlen(name) + 2);

    sprintf(project, "%s%c%s", base, HOST_DIRSEP, name);

    return project;
}
