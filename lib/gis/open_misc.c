
/****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements and the GRASS Development Team
 *
 * NOTE:         Based upon open.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <grass/config.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

static int G__open_misc(const char *dir,
			const char *element,
			const char *name, const char *subproject, int mode)
{
    int fd;
    char path[GPATH_MAX];
    char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];


    G__check_gisinit();

    /* READ */
    if (mode == 0) {
	if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	    if (*subproject && strcmp(xsubproject, subproject) != 0) {
 		G_warning(_("G__open_misc(read): subproject <%s> doesn't match xsubproject <%s>"),
 			  subproject, xsubproject);
		return -1;
	    }
	    name = xname;
	    subproject = xsubproject;
	}

	subproject = G_find_file2_misc(dir, element, name, subproject);

	if (!subproject)
	    return -1;

	G_file_name_misc(path, dir, element, name, subproject);

	if ((fd = open(path, 0)) < 0)
	    G_warning("G__open_misc(read): Unable to open '%s': %s",
	              path, strerror(errno));
	return fd;
    }
    /* WRITE */
    if (mode == 1 || mode == 2) {
	subproject = G_subproject();
	if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	    if (strcmp(xsubproject, subproject) != 0) {
 		G_warning(_("G__open_misc(write): xsubproject <%s> != G_subproject() <%s>"),
			  xsubproject, subproject);
		return -1;
	    }
	    name = xname;
	}

	if (G_legal_filename(name) == -1)
	    return -1;

	G_file_name_misc(path, dir, element, name, subproject);
	if (mode == 1 || access(path, 0) != 0) {
	    G__make_subproject_element_misc(dir, name);
	    close(creat(path, 0666));
	}

	if ((fd = open(path, mode)) < 0)
	    G_warning("G__open_misc(write): Unable to open '%s': %s",
	              path, strerror(errno));
	return fd;
    }
    return -1;
}


/*!
 * \brief open a new database file
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current subproject is created and opened for writing (but not reading).
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file is positioned at the end of the file and
 * the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \return int
 */

int G_open_new_misc(const char *dir, const char *element, const char *name)
{
    return G__open_misc(dir, element, name, G_subproject(), 1);
}


/*!
 * \brief open a database file for reading
 *
 * The database file <b>name</b> under the
 * <b>element</b> in the specified <b>subproject</b> is opened for reading (but
 * not for writing).
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \param subproject
 *  \return int
 */

int G_open_old_misc(const char *dir, const char *element, const char *name,
		    const char *subproject)
{
    return G__open_misc(dir, element, name, subproject, 0);
}


/*!
 * \brief open a database file for update
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current subproject is opened for reading and writing.
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file is positioned at the end of the file and
 * the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \return int
 */

int G_open_update_misc(const char *dir, const char *element, const char *name)
{
    int fd;

    fd = G__open_misc(dir, element, name, G_subproject(), 2);
    if (fd >= 0)
	lseek(fd, 0L, SEEK_END);

    return fd;
}


/*!
 * \brief open a new database file
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current subproject is created and opened for writing (but not reading).
 * The UNIX fopen( ) routine, with "w" write mode, is used to open the file.  If
 * the file does not exist, the NULL pointer is returned. Otherwise the file is
 * positioned at the end of the file and the file descriptor from the fopen( ) is
 * returned.
 *
 *  \param element
 *  \param name
 *  \return FILE * 
 */

FILE *G_fopen_new_misc(const char *dir, const char *element, const char *name)
{
    int fd;

    fd = G__open_misc(dir, element, name, G_subproject(), 1);
    if (fd < 0)
	return (FILE *) 0;

    return fdopen(fd, "w");
}


/*!
 * \brief open a database file for reading
 *
 * The database file <b>name</b> under the
 * <b>element</b> in the specified <b>subproject</b> is opened for reading (but
 * not for writing).
 * The UNIX fopen( ) routine, with "r" read mode, is used to open the file.  If
 * the file does not exist, the NULL pointer is returned. Otherwise the file
 * descriptor from the fopen( ) is returned.
 *
 *  \param element
 *  \param name
 *  \param subproject
 *  \return FILE * 
 */

FILE *G_fopen_old_misc(const char *dir, const char *element, const char *name,
		       const char *subproject)
{
    int fd;

    fd = G__open_misc(dir, element, name, subproject, 0);
    if (fd < 0)
	return (FILE *) 0;

    return fdopen(fd, "r");
}

FILE *G_fopen_append_misc(const char *dir, const char *element,
			  const char *name)
{
    int fd;

    fd = G__open_misc(dir, element, name, G_subproject(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek(fd, 0L, SEEK_END);

    return fdopen(fd, "a");
}

FILE *G_fopen_modify_misc(const char *dir, const char *element,
			  const char *name)
{
    int fd;

    fd = G__open_misc(dir, element, name, G_subproject(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek(fd, 0L, SEEK_END);

    return fdopen(fd, "r+");
}
