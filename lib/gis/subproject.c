/*!
  \file lib/gis/subproject.c
  
  \brief GIS library - environment routines (subproject)
  
  (C) 2001-2009, 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

/*!
  \brief Get current subproject name
  
  Returns the name of the current subproject in the current project. This
  routine is often used when accessing files in the current
  subproject. See Subprojects for an explanation of subprojects.
  
  G_fatal_error() is called on error.
  
  \return subproject name
*/
const char *G_subproject(void)
{
    const char *m = G__subproject();

    if (!m)
	G_fatal_error(_("MAPSET is not set"));

    return m;
}

/*!
  \brief Get current subproject name (internal use only)
  
  See G_subproject().
  
  \return pointer subproject name
  \return NULL on error
*/
const char *G__subproject(void)
{
    return G_getenv_nofatal("MAPSET");
}

/*!
  \brief Get current subproject UNIX-like path
 
  Allocated buffer should be freed by G_free(). See
  G__subproject_path().

  Returns the full UNIX path name of the current subproject. For example,
  if the user is working in subproject <i>user1</i>, project
  <i>spearfish</i> in the <i>/home/user/grassdata</i> database
  directory, this routine will return a string which looks like
  <i>/home/user/grassdata/spearfish/user1</i>.
  
  This function also checks if subproject path is readable by the current
  user. It calls G_fatal_error() on failure.

  \return buffer with project path
*/
char *G_subproject_path(void)
{
    char *subproject;

    subproject = G__subproject_path();
    if (access(subproject, F_OK) != 0) {
	perror("access");
	G_fatal_error(_("MAPSET <%s> not available"), subproject);
    }

    return subproject;
}

/*!
  \brief Get current subproject UNIX-like path (internal use only)
  
  Allocated buffer should be freed by G_free(). See also
  G_subproject_path().
  
  \todo Support also Windows-like path (?)
  
  \return buffer with subproject path
*/
char *G__subproject_path(void)
{
    const char *subproject = G__subproject();
    const char *project = G_project();
    const char *base = G_gisdbase();
    
    char *subproject_path = G_malloc(strlen(base) + strlen(project) +
                                 strlen(subproject) + 3);

    sprintf(subproject_path, "%s/%s/%s", base, project, subproject);

    return subproject_path;
}
