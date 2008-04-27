/**
 * \file rename.c
 *
 * \brief Rename file functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>


/**
 **\fn int G_rename_file (char *oldname, char *newname)
 **
 **\brief Rename a file in the filesystem.
 **
 **The file or directory <b>oldname</b> is renamed to <b>newname</b>.<br>
 **
 **
 ** \param[in] oldname
 ** \param[in] newname
 ** \return 0 if successful
 ** \return -1 on error
 **/

int G_rename_file ( const char *oldname, const char *newname )
{

    #ifdef __MINGW32__
      remove(newname);
    #endif

    return rename(oldname, newname); 
}

/**
 * \fn int G_rename (char *element, char *oldname, char *newname)
 *
 * \brief Rename a database file.
 *
 * The file or directory <b>oldname</b> under the database <b>element</b>
 * directory in the current mapset is renamed to <b>newname</b>.<br>
 *
 * <b>Bug:</b> This routine does not check to see if the <b>newname</b> 
 * name is a valid database file name.
 *
 * \param[in] element
 * \param[in] oldname
 * \param[in] newname
 * \return 0 if <b>oldname</b> does not exist
 * \return 1 if successful
 * \return -1 on error
 */

int G_rename ( const char *element,
    const char *oldname, const char *newname)
{
    const char *mapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char from[512], to[512];

    /* name in mapset legal only if mapset is current mapset */
    mapset = G_mapset();
    if (G__name_is_fully_qualified (oldname, xname, xmapset)
    && strcmp (mapset, xmapset))
	    return -1;
    if (G__name_is_fully_qualified (newname, xname, xmapset)
    && strcmp (mapset, xmapset))
	    return -1;

    /* if file does not exist return 0 */
    if (access (G__file_name (from, element, oldname, mapset),0) != 0)
	    return 0;

    G__file_name (to, element, newname, mapset);

    /* return result of rename */
    return G_rename_file(from, to) == 0 ? 1 : -1;
}
