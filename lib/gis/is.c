
/*!
 * \file lib/gis/is.c
 *
 * \brief GIS Library - Tests for file existence.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2001-2014
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>


static int test_path_file(const char *, const char *);


static int test_path_file(const char *path, const char *file)
{
    int ret;
    char *buf;

    buf = (char *)G_malloc(strlen(path) + strlen(file) + 2);
    sprintf(buf, "%s/%s", path, file);

    ret = access(buf, F_OK);
    G_free(buf);

    if (ret == 0)
	return 1;

    return 0;
}


/**

 * \brief Test if specified directory is GISBASE.
 *
 * \param[in] path Path to directory
 * \return 1 The directory is GISBASE
 * \return 0 The directory is not GISBASE
 */

int G_is_gisbase(const char *path)
{
    return test_path_file(path, "etc/element_list");
}


/**
 * \brief Test if specified directory is project.
 *
 * \param[in] path Path to directory
 * \return 1 The directory is project
 * \return 0 The directory is not project
 */

int G_is_project(const char *path)
{
    return test_path_file(path, "PERMANENT/DEFAULT_WIND");
}


/**
 * \brief Test if specified directory is subproject.
 *
 * \param[in] path Path to directory
 * \return 1 The directory is subproject
 * \return 0 The directory is not subproject
 */

int G_is_subproject(const char *path)
{
    return test_path_file(path, "WIND");
}
