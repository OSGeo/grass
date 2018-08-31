/*!
  \file lib/gis/mapset.c
  
  \brief GIS library - environment routines (mapset)
  
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
  \brief Get current mapset name
  
  Returns the name of the current mapset in the current location. This
  routine is often used when accessing files in the current
  mapset. See Mapsets for an explanation of mapsets.
  
  G_fatal_error() is called on error.
  
  \return mapset name
*/
const char *G_mapset(void)
{
    const char *m = G__mapset();

    if (!m)
	G_fatal_error(_("MAPSET is not set"));

    return m;
}

/*!
  \brief Get current mapset name (internal use only)
  
  See G_mapset().
  
  \return pointer mapset name
  \return NULL on error
*/
const char *G__mapset(void)
{
    return G_getenv_nofatal("MAPSET");
}

/*!
  \brief Get current mapset UNIX-like path
 
  Allocated buffer should be freed by G_free(). See
  G__mapset_path().

  Returns the full UNIX path name of the current mapset. For example,
  if the user is working in mapset <i>user1</i>, location
  <i>spearfish</i> in the <i>/home/user/grassdata</i> database
  directory, this routine will return a string which looks like
  <i>/home/user/grassdata/spearfish/user1</i>.
  
  This function also checks if mapset path is readable by the current
  user. It calls G_fatal_error() on failure.

  \return buffer with location path
*/
char *G_mapset_path(void)
{
    char *mapset;

    mapset = G__mapset_path();
    if (access(mapset, F_OK) != 0) {
	perror("access");
	G_fatal_error(_("MAPSET <%s> not available"), mapset);
    }

    return mapset;
}

/*!
  \brief Get current mapset UNIX-like path (internal use only)
  
  Allocated buffer should be freed by G_free(). See also
  G_mapset_path().
  
  \todo Support also Windows-like path (?)
  
  \return buffer with mapset path
*/
char *G__mapset_path(void)
{
    const char *mapset = G__mapset();
    const char *location = G_location();
    const char *base = G_gisdbase();
    
    char *mapset_path = G_malloc(strlen(base) + strlen(location) +
                                 strlen(mapset) + 3);

    sprintf(mapset_path, "%s/%s/%s", base, location, mapset);

    return mapset_path;
}
