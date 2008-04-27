/*
 *****************************************************************
 * open routines
 *
 * G__open (element, name, mapset, mode)
 *      char *element         database element name
 *      char *name            map file name
 *      char *mapset          mapset containing map "name"
 *      int mode              0=read, 1=write, 2=read/write
 * 
 *      this is the lowest level open routine.
 *      opens the file 'name' in 'element' ("cell", etc)
 *      in mapset 'mapset' according to the i/o 'mode'
 *
 *      mode = 0 (read) will look for 'name' in 'mapset'
 *               and open the file for read only
 *               the file must exist
 *
 *      mode = 1 (write) will create an empty file 'name' in the
 *               current mapset and open the file for write only
 *               'mapset' ignored
 *
 *      mode = 2 (read and write) will open a file in the
 *               current mapset for reading and writing
 *               creating a new file if necessary
 *               'mapset' ignored
 *
 *      returns: open file descriptor (int)
 *               or -1 could not open
 *
 *******************************************************************
 * G_open_new (element, name)
 *      char *element         database element name
 *      char *name            map file name
 *
 *      creates 'name' in the current mapset and opens it
 *      for write only.
 *
 *      returns: open file descriptor (int)
 *               or -1 could not open
 *
 *******************************************************************
 * G_open_old (element, name, mapset)
 *      char *element         database element name
 *      char *name            map file name
 *      char *mapset          mapset containing map "name"
 *
 *      opens 'name' in 'mapset' for read only.
 *
 *      returns: open file descriptor (int)
 *               or -1 could not open
 *
 *******************************************************************
 * G_fopen_new (element, name)
 *      char *element         database element name
 *      char *name            map file name
 *
 *      creates 'name' in the current mapset and opens it
 *      for write only.
 *
 *      returns: open file descriptor (FILE *)
 *               or NULL could not open
 *
 *******************************************************************
 * G_fopen_old (element, name, mapset)
 *      char *element         database element name
 *      char *name            map file name
 *
 *      opens 'name' in 'mapset' for read only.
 *
 *      returns: open file descriptor (FILE *)
 *               or NULL could not open
 *******************************************************************/

#include <grass/config.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <grass/gis.h>
#include <unistd.h>
#include <fcntl.h>

static int G__open (
    const char *element,
    const char *name,
    const char *mapset,
    int mode)
{
    char path[GPATH_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX], *dummy;


    G__check_gisinit();

/* READ */
    if (mode == 0)
    {
	if (G__name_is_fully_qualified (name, xname, xmapset))
	{
	    if (strcmp (xmapset, mapset) != 0) {
		fprintf(stderr, "G__open(r): mapset (%s) doesn't match xmapset (%s)\n",
			mapset,xmapset);
		    return -1;
	    }
	    name = xname;
	}
	if ((dummy = G_find_file2 (element, name, mapset)) == NULL)
	    return -1;
	G__file_name (path, element, name, mapset);

	return open (path, 0);
    }
/* WRITE */
    if (mode == 1 || mode == 2)
    {
	if (G__name_is_fully_qualified (name, xname, xmapset))
	{
	    if (strcmp (xmapset, G_mapset()) != 0) {
		fprintf(stderr, "G__open(w): xmapset (%s) != G_mapset() (%s)\n",
			xmapset,G_mapset());
		return -1;
	    }
	    name = xname;
	}

	if (G_legal_filename(name) == -1)
	    return -1;

	G__file_name (path, element, name, G_mapset());
	if(mode == 1 || access(path,0) != 0)
	{
	    G__make_mapset_element (element);
	    close (open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666));
	}

	return open (path, mode);
    }
    return -1;
}


/*!
 * \brief open a new database file
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current mapset is created and opened for writing (but not reading).
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file is positioned at the end of the file and
 * the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \return int
 */

int G_open_new (const char *element,const char *name)
{
    return G__open (element, name, G_mapset(), 1);
}


/*!
 * \brief open a database file for reading
 *
 * The database file <b>name</b> under the
 * <b>element</b> in the specified <b>mapset</b> is opened for reading (but
 * not for writing).
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \param mapset
 *  \return int
 */

int G_open_old (const char *element,const char *name,const char *mapset)
{
    return G__open (element, name, mapset, 0);
}


/*!
 * \brief open a database file for update
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current mapset is opened for reading and writing.
 * The UNIX open( ) routine is used to open the file. If the file does not exist,
 * -1 is returned. Otherwise the file is positioned at the end of the file and
 * the file descriptor from the open( ) is returned.
 *
 *  \param element
 *  \param name
 *  \return int
 */

int G_open_update (const char *element,const char *name)
{
    int fd;
    fd = G__open (element, name, G_mapset(), 2);
    if (fd >= 0) lseek (fd, 0L, SEEK_END);

    return fd;
}


/*!
 * \brief open a new database file
 *
 * The database file <b>name</b> under the <b>element</b> in the
 * current mapset is created and opened for writing (but not reading).
 * The UNIX fopen( ) routine, with "w" write mode, is used to open the file.  If
 * the file does not exist, the NULL pointer is returned. Otherwise the file is
 * positioned at the end of the file and the file descriptor from the fopen( ) is
 * returned.
 *
 *  \param element
 *  \param name
 *  \return FILE * 
 */

FILE *G_fopen_new (const char *element,const char *name)
{
    int fd;

    fd = G__open (element, name, G_mapset(), 1);
    if (fd < 0)
	return (FILE *) 0;

    return fdopen (fd, "w");
}


/*!
 * \brief open a database file for reading
 *
 * The database file <b>name</b> under the
 * <b>element</b> in the specified <b>mapset</b> is opened for reading (but
 * not for writing).
 * The UNIX fopen( ) routine, with "r" read mode, is used to open the file.  If
 * the file does not exist, the NULL pointer is returned. Otherwise the file
 * descriptor from the fopen( ) is returned.
 *
 *  \param element
 *  \param name
 *  \param mapset
 *  \return FILE * 
 */

FILE *
G_fopen_old (const char *element,const char *name,const char *mapset)
{
    int fd;

    fd = G__open (element, name, mapset, 0);
    if (fd < 0)
	return (FILE *) 0;

    return fdopen (fd, "r");
}

FILE *
G_fopen_append (const char *element,const char *name)
{
    int fd;

    fd = G__open (element, name, G_mapset(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek (fd, 0L, SEEK_END);

    return fdopen (fd, "a");
}

FILE *G_fopen_modify (const char *element,const char *name)
{
    int fd;

    fd = G__open (element, name, G_mapset(), 2);
    if (fd < 0)
	return (FILE *) 0;
    lseek (fd, 0L, SEEK_END);

    return fdopen (fd, "r+");
}
