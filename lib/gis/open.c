/*!
 * \file lib/gis/open.c
 * 
 * \brief GIS Library - Open file functions
 *
 * (C) 1999-2015 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author USACERL and many others
 */

#include <grass/config.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "gis_local_proto.h"

/*!
  \brief Lowest level open routine.

  Opens the file <i>name</i> in <i>element</i> ("cell", etc.) in subproject <i>subproject</i>
  according to the i/o <i>mode</i>.

   - mode = 0 (read) will look for <i>name</i> in <i>subproject</i> and
               open the file for read only the file must exist
 
   - mode = 1 (write) will create an empty file <i>name</i> in the
               current subproject and open the file for write only
               <i>subproject</i> ignored

   - mode = 2 (read and write) will open a file in the current subproject
               for reading and writing creating a new file if
               necessary <i>subproject</i> ignored

  \param element database element name
  \param name map file name
  \param subproject subproject containing map <i>name</i>
  \param mode r/w mode 0=read, 1=write, 2=read/write
 
  \return open file descriptor (int)
  \return -1 could not open
*/
static int G__open(const char *element,
		   const char *name, const char *subproject, int mode)
{
    int fd;
    int is_tmp;
    char path[GPATH_MAX];
    char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];
    
    G__check_gisinit();

    is_tmp = (element && strncmp(element, ".tmp", 4) == 0);

    /* READ */
    if (mode == 0) {
	if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	    if (*subproject && strcmp(xsubproject, subproject) != 0) {
		G_warning(_("G__open(read): subproject <%s> doesn't match xsubproject <%s>"),
			  subproject, xsubproject);
		return -1;
	    }
	    name = xname;
	    subproject = xsubproject;
	}

        if (!is_tmp) {
            subproject = G_find_file2(element, name, subproject);
            
            if (!subproject)
                return -1;

            G_file_name(path, element, name, subproject);
        }
        else {
            G_file_name_tmp(path, element, name, subproject);
        }
        
	if ((fd = open(path, 0)) < 0)
	    G_warning(_("G__open(read): Unable to open '%s': %s"),
	              path, strerror(errno));
	return fd;
    }
    /* WRITE */
    if (mode == 1 || mode == 2) {
	subproject = G_subproject();
	if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	    if (strcmp(xsubproject, subproject) != 0) {
		G_warning(_("G__open(write): xsubproject <%s> != G_subproject() <%s>"),
			  xsubproject, subproject);
		return -1;
	    }
	    name = xname;
	}

	if (*name && G_legal_filename(name) == -1)
	    return -1;

        if (!is_tmp)
            G_file_name(path, element, name, subproject);
        else
            G_file_name_tmp(path, element, name, subproject);
        
	if (mode == 1 || access(path, 0) != 0) {
            if (is_tmp)
                G_make_subproject_element_tmp(element);
            else
                G_make_subproject_element(element);
	    close(open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	if ((fd = open(path, mode)) < 0)
	    G_warning(_("G__open(write): Unable to open '%s': %s"),
	              path, strerror(errno));
	return fd;
    }
    return -1;
}

/*!
  \brief Open a new database file

  Creates <i>name</i> in the current subproject and opens it
  for write only.
  
  The database file <i>name</i> under the <i>element</i> in the
  current subproject is created and opened for writing (but not reading).
  The UNIX open() routine is used to open the file. If the file does
  not exist, -1 is returned. Otherwise the file is positioned at the
  end of the file and the file descriptor from the open() is returned.
 
  \param element database element name
  \param name map file name

  \return open file descriptor (int)
  \return -1 could not open
*/

int G_open_new(const char *element, const char *name)
{
    return G__open(element, name, G_subproject(), 1);
}


/*!
  \brief Open a database file for reading
  
  The database file <i>name</i> under the <i>element</i> in the
  specified <i>subproject</i> is opened for reading (but not for writing).
  The UNIX open() routine is used to open the file. If the file does
  not exist, -1 is returned. Otherwise the file descriptor from the
  open() is returned.
  
  \param element database element name
  \param name map file name
  \param subproject subproject containing map <i>name</i>

  \return open file descriptor (int)
  \return -1 could not open
*/
int G_open_old(const char *element, const char *name, const char *subproject)
{
    return G__open(element, name, subproject, 0);
}

/*!
  \brief Open a database file for update
 
  The database file <i>name</i> under the <i>element</i> in the
  current subproject is opened for reading and writing.  The UNIX open()
  routine is used to open the file. If the file does not exist, -1 is
  returned. Otherwise the file is positioned at the end of the file
  and the file descriptor from the open() is returned.
  
  \param element database element name
  \param name map file name

  \return open file descriptor (int)
  \return -1 could not open
 */

int G_open_update(const char *element, const char *name)
{
    int fd;

    fd = G__open(element, name, G_subproject(), 2);
    if (fd >= 0)
	lseek(fd, 0L, SEEK_END);

    return fd;
}


/*!
  \brief Open a new database file
  
  The database file <i>name</i> under the <i>element</i> in the
  current subproject is created and opened for writing (but not reading).
  The UNIX fopen() routine, with "w" write mode, is used to open the
  file.  If the file does not exist, the NULL pointer is
  returned. Otherwise the file is positioned at the end of the file
  and the file descriptor from the fopen() is returned.
  
  \param element database element name
  \param name map file name

  \return open file descriptor (FILE *)
  \return NULL on error
 */

FILE *G_fopen_new(const char *element, const char *name)
{
    int fd;

    fd = G__open(element, name, G_subproject(), 1);
    if (fd < 0) {
        G_debug(1, "G_fopen_new(): element = %s, name = %s : NULL",
                element, name);
	return (FILE *) 0;
    }

    G_debug(2, "\tfile open: new (mode = w)");
    return fdopen(fd, "w");
}


/*!
  \brief Open a database file for reading
  
  The database file <i>name</i> under the <i>element</i> in the
  specified <i>subproject</i> is opened for reading (but not for writing).
  The UNIX fopen() routine, with "r" read mode, is used to open the
  file.  If the file does not exist, the NULL pointer is
  returned. Otherwise the file descriptor from the fopen() is
  returned.
 
  \param element database element name
  \param name map file name
  \param subproject subproject name containing map <i>name</i>

  \return open file descriptor (FILE *)
  \return NULL on error
*/
FILE *G_fopen_old(const char *element, const char *name, const char *subproject)
{
    int fd;

    fd = G__open(element, name, subproject, 0);
    if (fd < 0)
	return (FILE *) NULL;

    G_debug(2, "\tfile open: read (mode = r)");
    return fdopen(fd, "r");
}

/*!
  \brief Open a database file for update (append mode)
  
  The database file <i>name</i> under the <i>element</i> in the
  current subproject is opened for for writing. The UNIX fopen() routine,
  with "a" append mode, is used to open the file.  If the file does not
  exist, the NULL pointer is returned. Otherwise the file descriptor
  from the fopen() is returned.
 
  \param element database element name
  \param name map file name

  \return open file descriptor (FILE *)
  \return NULL on error
*/
FILE *G_fopen_append(const char *element, const char *name)
{
    int fd;

    fd = G__open(element, name, G_subproject(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek(fd, 0L, SEEK_END);

    G_debug(2, "\tfile open: append (mode = a)");
    return fdopen(fd, "a");
}

/*!
  \brief Open a database file for update (r+ mode)
  
  The database file <i>name</i> under the <i>element</i> in the
  current subproject is opened for for writing. The UNIX fopen() routine,
  with "r+" append mode, is used to open the file.  If the file does not
  exist, the NULL pointer is returned. Otherwise the file descriptor
  from the fopen() is returned.
 
  \param element database element name
  \param name map file name

  \return open file descriptor (FILE *)
  \return NULL on error
*/
FILE *G_fopen_modify(const char *element, const char *name)
{
    int fd;

    fd = G__open(element, name, G_subproject(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek(fd, 0L, SEEK_END);

    G_debug(2, "\tfile open: modify (mode = r+)");
    return fdopen(fd, "r+");
}
