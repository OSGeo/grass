/*!
  \file gis/location.c
  
  \brief GIS library - environment routines (location)
  
  (C) 2001-2008 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/*!
 * \brief Get current location name
 *
 * Returns the name of the current database location. This routine
 * should be used by modules that need to display the current location
 * to the user. See Locations for an explanation of locations.
 *
 *  \return char* tolocation name
 */

const char *G_location(void)
{
    return G_getenv("LOCATION_NAME");
}

/*!
 * \brief Get current location directory
 *
 * Returns the full UNIX path name of the current database
 * location. For example, if the user is working in location
 * <i>spearfish</i> in the <i>/home/user/grassdata</i> database
 * directory, this routine will return a string which looks like
 * <i>/home/user/grassdata/spearfish</i>.
 *
 *  \return char * 
 */

char *G_location_path(void)
{
    char *location;

    location = G__location_path();
    if (access(location, F_OK) != 0) {
	perror("access");
	G_fatal_error(_("LOCATION << %s >> not available"), location);
    }

    return location;
}


/*!
 * \brief Get current location path
 *
 *  \return char* to location path
 */
char *G__location_path(void)
{
    const char *name = G_location();
    const char *base = G_gisdbase();
    char *location = G_malloc(strlen(base) + strlen(name) + 2);

    sprintf(location, "%s/%s", base, name);

    return location;
}
