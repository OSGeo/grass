/*!
   \file lib/gis/mapset_msc.c

   \brief GIS library - Mapset user permission routines.

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

static int make_mapset_element(const char *, const char *);
static int make_mapset_element_no_fail_on_race(const char *, const char *);
static int make_mapset_element_impl(const char *, const char *, bool);

/*!
   \brief Create element in the current mapset.

   Make the specified element in the current mapset will check for the
   existence of the element and do nothing if it is found so this
   routine can be called even if the element already exists.

   Calls G_fatal_error() on failure.

   \deprecated
   This function is deprecated due to confusion in element terminology.
   Use G_make_mapset_object_group() or G_make_mapset_dir_object() instead.

   \param p_element element to be created in mapset

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_element(const char *p_element)
{
    char path[GPATH_MAX];

    G_file_name(path, NULL, NULL, G_mapset());
    return make_mapset_element(path, p_element);
}

/*!
<<<<<<< HEAD
<<<<<<< HEAD
   \brief Create directory for group of elements of a given type.

   Creates the specified element directory in the current mapset.
   It will check for the existence of the element and do nothing
   if it is found so this routine can be called even if the element
   already exists to ensure that it exists.

   If creation fails, but the directory exists after the failure,
   the function reports success. Therefore, two processes creating
   a directory in this way can work in parallel.

   Calls G_fatal_error() on failure.

   \param type object type (e.g., `cell`)

   \return 0 no element defined
   \return 1 on success

   \sa G_make_mapset_dir_object()
   \sa G_make_mapset_object_group_tmp()
=======
    \brief Create directory for group of elements of a given type.
=======
   \brief Create directory for group of elements of a given type.
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

   Creates the specified element directory in the current mapset.
   It will check for the existence of the element and do nothing
   if it is found so this routine can be called even if the element
   already exists to ensure that it exists.

   If creation fails, but the directory exists after the failure,
   the function reports success. Therefore, two processes creating
   a directory in this way can work in parallel.

   Calls G_fatal_error() on failure.

   \param type object type (e.g., `cell`)

   \return 0 no element defined
   \return 1 on success

<<<<<<< HEAD
    \sa G_make_mapset_dir_object()
    \sa G_make_mapset_object_group_tmp()
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
   \sa G_make_mapset_dir_object()
   \sa G_make_mapset_object_group_tmp()
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
 */
int G_make_mapset_object_group(const char *type)
{
    char path[GPATH_MAX];

    G_file_name(path, NULL, NULL, G_mapset());
    return make_mapset_element_no_fail_on_race(path, type);
}

/*!
<<<<<<< HEAD
<<<<<<< HEAD
   \brief Create directory for an object of a given type.

   Creates the specified element directory in the current mapset.
   It will check for the existence of the element and do nothing
   if it is found so this routine can be called even if the element
   already exists to ensure that it exists.

   Any failure to create it, including the case when it exists
   (i.e., was created by another process after the existence test)
   is considered a failure because two processes should not attempt
   to create two objects of the same name (and type).

   This function is for objects which are directories
   (the function does not create files).

   Calls G_fatal_error() on failure.

   \param type object type (e.g., `vector`)
   \param name object name (e.g., `bridges`)

   \return 0 no element defined
   \return 1 on success

   \sa G_make_mapset_object_group()
=======
    \brief Create directory for an object of a given type.
=======
   \brief Create directory for an object of a given type.
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

   Creates the specified element directory in the current mapset.
   It will check for the existence of the element and do nothing
   if it is found so this routine can be called even if the element
   already exists to ensure that it exists.

   Any failure to create it, including the case when it exists
   (i.e., was created by another process after the existence test)
   is considered a failure because two processes should not attempt
   to create two objects of the same name (and type).

   This function is for objects which are directories
   (the function does not create files).

   Calls G_fatal_error() on failure.

   \param type object type (e.g., `vector`)
   \param name object name (e.g., `bridges`)

   \return 0 no element defined
   \return 1 on success

<<<<<<< HEAD
    \sa G_make_mapset_object_group()
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
   \sa G_make_mapset_object_group()
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
 */
int G_make_mapset_dir_object(const char *type, const char *name)
{
    char path[GPATH_MAX];

    G_make_mapset_object_group(type);
    G_file_name(path, type, NULL, G_mapset());
    return make_mapset_element(path, name);
}

/*!
   \brief Create element in the temporary directory.

   See G_file_name_tmp() for details.

   \param p_element element to be created in mapset (e.g., `elevation`)

   \note
   Use G_make_mapset_object_group_tmp() for creating common, shared
   directories which are for multiple concrete elements (objects).

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_element_tmp(const char *p_element)
{
    char path[GPATH_MAX];

    G_file_name_tmp(path, NULL, NULL, G_mapset());
    return make_mapset_element(path, p_element);
}

/*!
<<<<<<< HEAD
<<<<<<< HEAD
   \brief Create directory for type of objects in the temporary directory.

   See G_file_name_tmp() for details.

   \param type object type (e.g., `cell`)

   \note
   Use G_make_mapset_object_group_tmp() for creating common, shared
   directories which are for multiple concrete elements (objects).

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_object_group_tmp(const char *type)
=======
    \brief Create directory for type of objects in the temporary directory.
=======
   \brief Create directory for type of objects in the temporary directory.
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

   See G_file_name_tmp() for details.

   \param type object type (e.g., `cell`)

   \note
   Use G_make_mapset_object_group_tmp() for creating common, shared
   directories which are for multiple concrete elements (objects).

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_object_group_tmp(const char *type)
{
    char path[GPATH_MAX];

    G_file_name_tmp(path, NULL, NULL, G_mapset());
    return make_mapset_element_no_fail_on_race(path, type);
}

<<<<<<< HEAD
int make_mapset_element_impl(const char *p_path, const char *p_element, bool race_ok)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
/*!
   \brief Create directory for type of objects in the temporary directory.

   See G_file_name_basedir() for details.

   \param type object type (e.g., `cell`)

   \note
   Use G_make_mapset_object_group_basedir() for creating common, shared
   directories for temporary data.

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_object_group_basedir(const char *type, const char *basedir)
{
    char path[GPATH_MAX];

    G_file_name_basedir(path, NULL, NULL, G_mapset(), basedir);
    return make_mapset_element_no_fail_on_race(path, type);
}

int make_mapset_element_impl(const char *p_path, const char *p_element,
                             bool race_ok)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    char path[GPATH_MAX];

    G_file_name_tmp(path, NULL, NULL, G_mapset());
    return make_mapset_element_no_fail_on_race(path, type);
}

/*!
   \brief Create directory for type of objects in the temporary directory.

   See G_file_name_basedir() for details.

   \param type object type (e.g., `cell`)

   \note
   Use G_make_mapset_object_group_basedir() for creating common, shared
   directories for temporary data.

   \return 0 no element defined
   \return 1 on success
 */
int G_make_mapset_object_group_basedir(const char *type, const char *basedir)
{
    char path[GPATH_MAX];

    G_file_name_basedir(path, NULL, NULL, G_mapset(), basedir);
    return make_mapset_element_no_fail_on_race(path, type);
}

int make_mapset_element_impl(const char *p_path, const char *p_element,
                             bool race_ok)
{
    char path[GPATH_MAX] = {'\0'};
    char *p;
    const char *element;

    element = p_element;
    if (*element == 0)
        return 0;

    strncpy(path, p_path, GPATH_MAX - 1);
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
<<<<<<< HEAD
<<<<<<< HEAD
        if (*element == '/' || *element == 0) {
            *p = 0;
            char *msg = NULL;

=======
	if (*element == '/' || *element == 0) {
	    *p = 0;
            char *msg = NULL;
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
        if (*element == '/' || *element == 0) {
            *p = 0;
            char *msg = NULL;

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
            if (access(path, 0) != 0) {
                /* Assuming that directory does not exist. */
                if (G_mkdir(path) != 0) {
                    msg = G_store(strerror(errno));
                }
            }
            if (access(path, 0) != 0 || (msg && !race_ok)) {
<<<<<<< HEAD
<<<<<<< HEAD
                /* Directory is not accessible even after attempt to create it.
                 */
                if (msg) {
                    /* Error already happened when mkdir. */
                    G_fatal_error(
                        _("Unable to make mapset element %s (%s): %s"),
                        p_element, path, strerror(errno));
                }
                else {
                    /* Access error is not related to mkdir. */
                    G_fatal_error(
                        _("Unable to access mapset element %s (%s): %s"),
                        p_element, path, strerror(errno));
                }
            }
            if (*element == 0)
                return 1;
        }
        *p++ = *element++;
=======
                /* Directory is not accessible even after attempt to create it. */
=======
                /* Directory is not accessible even after attempt to create it.
                 */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
                if (msg) {
                    /* Error already happened when mkdir. */
                    G_fatal_error(
                        _("Unable to make mapset element %s (%s): %s"),
                        p_element, path, strerror(errno));
                }
                else {
                    /* Access error is not related to mkdir. */
                    G_fatal_error(
                        _("Unable to access mapset element %s (%s): %s"),
                        p_element, path, strerror(errno));
                }
            }
<<<<<<< HEAD
	    if (*element == 0)
		return 1;
	}
	*p++ = *element++;
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
            if (*element == 0)
                return 1;
        }
        *p++ = *element++;
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
    }
}

int make_mapset_element(const char *p_path, const char *p_element)
{
    return make_mapset_element_impl(p_path, p_element, false);
}

<<<<<<< HEAD
<<<<<<< HEAD
int make_mapset_element_no_fail_on_race(const char *p_path,
                                        const char *p_element)
=======
int make_mapset_element_no_fail_on_race(const char *p_path, const char *p_element)
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
int make_mapset_element_no_fail_on_race(const char *p_path,
                                        const char *p_element)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
{
    return make_mapset_element_impl(p_path, p_element, true);
}

<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
/*!
   \brief Create misc element in the current mapset.

   \param dir directory path (e.g., `cell_misc`)
   \param name element to be created in mapset (e.g., `elevation`)

   \return 0 no element defined
   \return 1 on success
 */
int G__make_mapset_element_misc(const char *dir, const char *name)
{
<<<<<<< HEAD
<<<<<<< HEAD
    return G_make_mapset_dir_object(dir, name);
=======
    G_make_mapset_dir_object(dir, name);
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
    return G_make_mapset_dir_object(dir, name);
>>>>>>> 27faeed049 (r.in.pdal: use fabs for double values (#1752))
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
   \brief Check for user mapset permission

   \param mapset mapset name

   \return 1 mapset exists, and user has permission
   \return 0 mapset exists, BUT user denied permission
   \return -1 mapset does not exist
 */
int G_mapset_permissions(const char *mapset)
{
    char path[GPATH_MAX];
    struct stat info;

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
int G_mapset_permissions2(const char *gisdbase, const char *location,
                          const char *mapset)
{
    char path[GPATH_MAX];
    struct stat info;

    sprintf(path, "%s/%s/%s", gisdbase, location, mapset);

    if (G_stat(path, &info) != 0)
        return -1;
    if (!S_ISDIR(info.st_mode))
        return -1;

    if (!check_owner(&info))
        return 0;

    return 1;
}
