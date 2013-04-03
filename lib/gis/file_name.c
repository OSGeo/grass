/*!
   \file lib/gis/file_name.c

   \brief GIS library - Determine GRASS data base file name

   (C) 2001-2008, 2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

#include "gis_local_proto.h"

/*!
  \brief Builds full path names to GIS data files

  If name is of the form "nnn@ppp" then path is set as if name had
  been nnn and mapset had been ppp (mapset parameter itself is ignored
  in this case).
  
  \param[out] path buffer to hold resultant full path to file
  \param element database element (eg, "cell", "cellhd", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param mapset mapset name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name(char *path,
		   const char *element, const char *name, const char *mapset)
{
    char xname[GNAME_MAX];
    char xmapset[GMAPSET_MAX];
    const char *pname = name;
    char *location = G__location_path();

    /*
     * if a name is given, build a file name
     * must split the name into name, mapset if it is
     * in the name@mapset format
     */
    if (name && *name && G_name_is_fully_qualified(name, xname, xmapset)) {
	pname = xname;
	sprintf(path, "%s/%s", location, xmapset);
    }
    else if (mapset && *mapset)
	sprintf(path, "%s/%s", location, mapset);
    else
	sprintf(path, "%s/%s", location, G_mapset());

    G_free(location);

    if (!element && !pname)
        return path;
    
    if (element && *element) {
	strcat(path, "/");
	strcat(path, element);
    }

    if (pname && *pname) {
	strcat(path, "/");
	strcat(path, pname);
    }

    G_debug(2, "G_file_name(): path = %s", path);
    
    return path;
}

char *G_file_name_misc(char *path,
			const char *dir,
			const char *element,
			const char *name, const char *mapset)
{
    char xname[GNAME_MAX];
    char xmapset[GMAPSET_MAX];
    const char *pname = name;
    char *location = G__location_path();

    /*
     * if a name is given, build a file name
     * must split the name into name, mapset if it is
     * in the name@mapset format
     */
    if (name && *name && G_name_is_fully_qualified(name, xname, xmapset)) {
	pname = xname;
	sprintf(path, "%s/%s", location, xmapset);
    }
    else if (mapset && *mapset)
	sprintf(path, "%s/%s", location, mapset);
    else
	sprintf(path, "%s/%s", location, G_mapset());

    G_free(location);

    if (dir && *dir) {
	strcat(path, "/");
	strcat(path, dir);
    }

    if (pname && *pname) {
	strcat(path, "/");
	strcat(path, pname);
    }

    if (element && *element) {
	strcat(path, "/");
	strcat(path, element);
    }

    return path;
}
