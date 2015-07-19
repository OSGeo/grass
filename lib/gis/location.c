/*!
  \file lib/gis/location.c
  
  \brief GIS library - environment routines (location)
  
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
  \brief Get current location name
  
  Returns the name of the current database location. This routine
  should be used by modules that need to display the current location
  to the user. See Locations for an explanation of locations.
  
  \return location name
*/
const char *G_location(void)
{
    return G_getenv("LOCATION_NAME");
}

/*!
  \brief Get current location UNIX-like path
 
  Allocated buffer should be freed by G_free(). See
  G__location_path().

  Returns the full UNIX path name of the current database
  location. For example, if the user is working in location
  <i>spearfish</i> in the <i>/home/user/grassdata</i> database
  directory, this routine will return a string which looks like
  <i>/home/user/grassdata/spearfish</i>.
  
  This function also checks if location path is readable by the
  current user. It calls G_fatal_error() on failure.

  \return buffer with location path
 */
char *G_location_path(void)
{
    char *location;

    location = G__location_path();
    if (access(location, F_OK) != 0) {
	perror("access");
	G_fatal_error(_("LOCATION <%s> not available"), location);
    }

    return location;
}


/*!
  \brief Get current location UNIX-like path (internal use only)
  
  Allocated buffer should be freed by G_free(). See also
  G_location_path().
  
  \todo Support also Windows-like path (?)
  
  \return buffer with location path
 */
char *G__location_path(void)
{
    const char *name = G_location();
    const char *base = G_gisdbase();
    char *location = G_malloc(strlen(base) + strlen(name) + 2);

    sprintf(location, "%s%c%s", base, HOST_DIRSEP, name);

    return location;
}
