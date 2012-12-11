/*!
   \file mapset_msc.c

   \brief GIS library - Mapset user permission routines.

   (C) 1999-2008 The GRASS development team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.
 */

#include <grass/config.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
   \brief Create element in the current mapset.

   Make the specified element in the current mapset
   will check for the existence of the element and
   do nothing if it is found so this routine
   can be called even if the element already exists.

   \param element element to be created in mapset

   \return 0 ?
   \return ?
 */
int G__make_mapset_element(const char *p_element)
{
    char path[GPATH_MAX];
    char *p;
    const char *element;

    element = p_element;
    if (*element == 0)
	return 0;

    G_file_name(p = path, "", "", G_mapset());
    while (*p)
	p++;
    /* add trailing slash if missing */
    --p;
    if (*p++ != '/') {
	*p++ = '/';
	*p = 0;
    }

    /* now append element, one directory at a time, to path */
    while (1) {
	if (*element == '/' || *element == 0) {
	    *p = 0;
	    if (access(path, 0) != 0) { /* directory not yet created */
		if (G_mkdir(path) != 0)
		    G_fatal_error(_("Unable to make mapset element %s (%s): %s"),
				  p_element, path, strerror(errno));
	    }
	    if (access(path, 0) != 0)  /* directory not accessible */
		G_fatal_error(_("Unable to access mapset element %s (%s): %s"),
			      p_element, path, strerror(errno));
	    if (*element == 0)
		return 1;
	}
	*p++ = *element++;
    }
}

/*!
   \brief Create misc element in the current mapset.

   \param dir directory path
   \param name element name

   \return 0 ?
   \return ?
 */
int G__make_mapset_element_misc(const char *dir, const char *name)
{
    char buf[GNAME_MAX * 2 + 1];

    sprintf(buf, "%s/%s", dir, name);
    return G__make_mapset_element(buf);
}

static int check_owner(const STRUCT_STAT *info)
{
#if defined(__MINGW32__) || defined(SKIP_MAPSET_OWN_CHK)
    return 1;
#else
    const char *check = getenv("GRASS_SKIP_MAPSET_OWNER_CHECK");
    if (check && *check)
	return 1;
    if (info->st_uid != getuid())
	return 0;
    if (info->st_uid != geteuid())
	return 0;
    return 1;
#endif
}

/*!
   \brief Check for user mapset permission

   \param mapset mapset name

   \return 1 mapset exists, and user has permission
   \return 0 mapset exists, BUT user denied permission
   \return -1 mapset does not exist
 */
int G__mapset_permissions(const char *mapset)
{
    char path[GPATH_MAX];
    STRUCT_STAT info;

    G_file_name(path, "", "", mapset);

    if (G_stat(path, &info) != 0)
	return -1;
    if (!S_ISDIR(info.st_mode))
	return -1;

    if (!check_owner(&info))
	return 0;

    return 1;
}

/*!
   \brief Check for user mapset permission

   \param gisdbase full path to GISDBASE
   \param location location name
   \param mapset mapset name

   \return 1 mapset exists, and user has permission
   \return 0 mapset exists, BUT user denied permission
   \return -1 mapset does not exist
 */
int G__mapset_permissions2(const char *gisdbase, const char *location,
			   const char *mapset)
{
    char path[GPATH_MAX];
    STRUCT_STAT info;

    sprintf(path, "%s/%s/%s", gisdbase, location, mapset);

    if (G_stat(path, &info) != 0)
	return -1;
    if (!S_ISDIR(info.st_mode))
	return -1;

    if (!check_owner(&info))
	return 0;

    return 1;
}
