/*!
 * \file lib/gis/home.c
 *
 * \brief GIS Library - Get user's home or config directory.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

/*!
 * \brief Get user's home directory
 *
 * Returns a pointer to a string which is the full path name of the
 * user's home directory.
 *
 * Calls G_fatal_error() on failure.
 *
 * \return pointer to string
 * \return NULL on error
 */
const char *G_home(void)
{
    const char *home = G__home();

    if (home)
	return home;

    G_fatal_error(_("Unable to determine user's home directory"));

    return NULL;
}

/*!
 * \brief Get user's home directory (internal use only)
 *
 * Returns a pointer to a string which is the full path name of the
 * user's home directory.
 *
 * \return pointer to string
 * \return NULL on error
 */
const char *G__home(void)
{
    static int initialized;
    static const char *home = 0;

    if (G_is_initialized(&initialized))
	return home;

#ifdef __MINGW32__
    {
	char buf[GPATH_MAX];

	/* TODO: we should probably check if the dir exists */
	home = getenv("USERPROFILE");

	if (!home) {
	    sprintf(buf, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));

	    if (strlen(buf) >= 0)
		home = G_store(buf);
	}

	if (!home)
	    home = getenv("HOME");
    }
#else
    home = getenv("HOME");
#endif
    G_initialize_done(&initialized);
    return home;
}

/*!
 * \brief Get user's config path directory
 *
 * Returns a pointer to a string which is the full path name of the
 * user's GRASS config directory in their home directory.
 *
 * The path is not guaranteed to exist.
 *
 * \todo should it be? see possible TODO below
 *
 * \return pointer to string
 * \return NULL on error
 */
const char *G_config_path(void)
{
    static int initialized_config;
    static const char *config_path = 0;
    char buf[GPATH_MAX];
    
    if (G_is_initialized(&initialized_config))
        return config_path;
    
#ifdef __MINGW32__
    sprintf(buf, "%s%c%s", getenv("APPDATA"), HOST_DIRSEP, CONFIG_DIR);
#else    
    sprintf(buf, "%s%c%s", G_home(), HOST_DIRSEP, CONFIG_DIR);
#endif
    config_path = G_store(buf);

#if 0
    /* create it if it doesn't exist */
#include <errno.h>
    int ret;
    ret = G_mkdir(rcpath);
    if (ret == -1 && errno != EEXIST)
	G_fatal_error(_("Failed to create directory [%s]"), rcpath);
#endif

    G_initialize_done(&initialized_config);

    return config_path;
}
