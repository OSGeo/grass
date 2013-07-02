/*!
 * \file gis/remove.c
 *
 * \brief GIS Library - File remove functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grass/gis.h>

static int recursive_remove(const char *path);
static int G__remove(int misc, const char *dir, const char *element,
		     const char *name);

/*!
 * \brief Remove a database file.
 *
 * The file or directory <i>name</i> under the database <i>element</i>
 * directory in the current mapset is removed.
 * 
 * If <i>name</i> is a directory, everything within the directory is
 * removed as well.
 *
 * \param element element name
 * \param name file name
 *
 * \return 0 if <i>name</i> does not exist
 * \return 1 if successful
 * \return -1 on error
 */

int G_remove(const char *element, const char *name)
{
    return G__remove(0, NULL, element, name);
}

/*!
 * \brief Remove a database misc file.
 *
 * The file or directory <i>name</i> under the database <i>element</i>
 * directory in the current mapset is removed.
 * 
 * If <i>name</i> is a directory, everything within the directory is
 * removed as well.
 *
 * \param element element name
 * \param name file name
 *
 * \return 0 if <i>name</i> does not exist
 * \return 1 if successful
 * \return -1 on error
 */
int G_remove_misc(const char *dir, const char *element, const char *name)
{
    return G__remove(1, dir, element, name);
}

static int G__remove(int misc, const char *dir, const char *element,
		     const char *name)
{
    char path[GPATH_MAX];
    const char *mapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    /* name in mapset legal only if mapset is current mapset */
    mapset = G_mapset();
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(mapset, xmapset) != 0)
	    return -1;
	name = xname;
    }

    if (G_legal_filename(name) < 0)
	return -1;

    if (misc)
	G_file_name_misc(path, dir, element, name, mapset);
    else
	G_file_name(path, element, name, mapset);

    /* if file does not exist, return 0 */
    if (access(path, 0) != 0)
	return 0;

    if (recursive_remove(path) == 0)
	return 1;

    return -1;
}

/* equivalent to rm -rf path */
static int recursive_remove(const char *path)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;
    char path2[GPATH_MAX];

    if (G_lstat(path, &sb))
	return 1;
    if (!S_ISDIR(sb.st_mode))
	return remove(path) == 0 ? 0 : 1;

    if ((dirp = opendir(path)) == NULL)
	return 1;
    while ((dp = readdir(dirp)) != NULL) {
	if (dp->d_name[0] == '.')
	    continue;
	if (strlen(path) + strlen(dp->d_name) + 2 > sizeof(path2))
	    continue;
	sprintf(path2, "%s/%s", path, dp->d_name);
	recursive_remove(path2);
    }
    closedir(dirp);

    return rmdir(path) == 0 ? 0 : 1;
}
