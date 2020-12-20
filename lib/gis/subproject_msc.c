/*!
   \file lib/gis/subproject_msc.c

   \brief GIS library - Subproject user permission routines.

   (C) 1999-2014 The GRASS development team

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

static int make_subproject_element(const char *, const char *);

/*!
   \brief Create element in the current subproject.

   Make the specified element in the current subproject will check for the
   existence of the element and do nothing if it is found so this
   routine can be called even if the element already exists.
   
   Calls G_fatal_error() on failure.
   
   \param p_element element to be created in subproject

   \return 0 no element defined
   \return 1 on success
 */
int G_make_subproject_element(const char *p_element)
{
    char path[GPATH_MAX];
    
    G_file_name(path, NULL, NULL, G_subproject());
    return make_subproject_element(path, p_element);
}

/*!
   \brief Create element in the temporary directory.

   See G_file_name_tmp() for details.

   \param p_element element to be created in subproject

   \return 0 no element defined
   \return 1 on success
 */
int G_make_subproject_element_tmp(const char *p_element)
{
    char path[GPATH_MAX];
    
    G_file_name_tmp(path, NULL, NULL, G_subproject());
    return make_subproject_element(path, p_element);
}

int make_subproject_element(const char *p_path, const char *p_element)
{
    char path[GPATH_MAX], *p;
    const char *element;

    element = p_element;
    if (*element == 0)
	return 0;

    strncpy(path, p_path, GPATH_MAX);
    p = path;
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
		    G_fatal_error(_("Unable to make subproject element %s (%s): %s"),
				  p_element, path, strerror(errno));
	    }
	    if (access(path, 0) != 0)  /* directory not accessible */
		G_fatal_error(_("Unable to access subproject element %s (%s): %s"),
			      p_element, path, strerror(errno));
	    if (*element == 0)
		return 1;
	}
	*p++ = *element++;
    }
}

/*!
   \brief Create misc element in the current subproject.

   \param dir directory path
   \param name element to be created in subproject

   \return 0 no element defined
   \return 1 on success
 */
int G__make_subproject_element_misc(const char *dir, const char *name)
{
    char buf[GNAME_MAX * 2 + 1];

    sprintf(buf, "%s/%s", dir, name);
    return G_make_subproject_element(buf);
}

static int check_owner(const struct stat *info)
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
   \brief Check for user subproject permission

   \param subproject subproject name

   \return 1 subproject exists, and user has permission
   \return 0 subproject exists, BUT user denied permission
   \return -1 subproject does not exist
 */
int G_subproject_permissions(const char *subproject)
{
    char path[GPATH_MAX];
    struct stat info;

    G_file_name(path, "", "", subproject);

    if (G_stat(path, &info) != 0)
	return -1;
    if (!S_ISDIR(info.st_mode))
	return -1;

    if (!check_owner(&info))
	return 0;

    return 1;
}

/*!
   \brief Check for user subproject permission

   \param gisdbase full path to GISDBASE
   \param project project name
   \param subproject subproject name

   \return 1 subproject exists, and user has permission
   \return 0 subproject exists, BUT user denied permission
   \return -1 subproject does not exist
 */
int G_subproject_permissions2(const char *gisdbase, const char *project,
			   const char *subproject)
{
    char path[GPATH_MAX];
    struct stat info;

    sprintf(path, "%s/%s/%s", gisdbase, project, subproject);

    if (G_stat(path, &info) != 0)
	return -1;
    if (!S_ISDIR(info.st_mode))
	return -1;

    if (!check_owner(&info))
	return 0;

    return 1;
}
