/*!
 * \file lib/gis/rename.c
 *
 * \brief GIS Library - Rename file functions.
 *
 * (C) 2001-2015 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>


/*!
  \brief Rename a file or a directory in the filesystem.
  
  The file or directory <i>oldname</i> is renamed to <i>newname</i>.

  \param oldname current name
  \param newname new name
  
  \return 0 if successful
  \return -1 on error
*/
int G_rename_file(const char *oldname, const char *newname)
{
    int ret;

#ifdef __MINGW32__
    remove(newname);
#endif
    
    ret = rename(oldname, newname);

    if (ret == -1) {
	/* if fails, try to copy file and then remove */
	if (1 == G_copy_file(oldname, newname)) {
	    if (remove(oldname) != -1)
		ret = 0;
	}
    }

    return ret;
}

/*!
  \brief Rename a database file.

  The file or directory <i>oldname</i> under the database <i>element</i>
  directory in the current mapset is renamed to <i>newname</i>.

  \bug This routine does not check to see if the <i>newname</i> 
  name is a valid database file name.

  \param element element name
  \param oldname current name
  \param newname new name

  \return 0 if <i>oldname</i> does not exist
  \return 1 if successful
  \return -1 on error
*/
int G_rename(const char *element, const char *oldname, const char *newname)
{
    const char *mapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char from[GPATH_MAX], to[GPATH_MAX];

    /* name in mapset legal only if mapset is current mapset */
    mapset = G_mapset();
    if (G_name_is_fully_qualified(oldname, xname, xmapset)
	&& strcmp(mapset, xmapset))
	return -1;
    if (G_name_is_fully_qualified(newname, xname, xmapset)
	&& strcmp(mapset, xmapset))
	return -1;

    /* if file does not exist return 0 */
    if (access(G_file_name(from, element, oldname, mapset), 0) != 0)
	return 0;

    G_file_name(to, element, newname, mapset);

    /* return result of rename */
    return G_rename_file(from, to) == 0 ? 1 : -1;
}
