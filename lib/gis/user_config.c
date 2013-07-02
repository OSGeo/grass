
/**
 * \file user_config.c
 *
 * \brief GIS Library - Routines related to user's GRASS configuration, tmp, and 
 * miscellaneous files.
 *
 * Functions related to the user's GRASS configuration, tmp, and 
 * miscellaneous files. Provides a set of routines for creating and 
 * accessing elements within the user's "rc" directory. The 
 * directory is in $HOME/.grass.<br>
 *
 * <b>NOTE:</b> As of 2001-03-25 this file is not hooked up.  It is 
 * provided as a candidate for handling $HOME/.grass files and 
 * subdirectories. There may be more functionality desired (such as 
 * deletion routines, directory globs).<br>
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Eric G Miller - egm2 at jps net
 *
 * \date 2007-04-14
 */

#include <grass/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#ifndef __MINGW32__
#include <pwd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <grass/gis.h>


/**************************************************************************
 * _make_toplevel(): make user's toplevel config directory if it doesn't
 * already exist.  Adjust perms to 1700. Returns the toplevel directory
 * path [caller must G_free ()] on success, or NULL on failure
 *************************************************************************/

#ifndef __MINGW32__		/* TODO */
static char *_make_toplevel(void)
{
    size_t len;
    int status;

#ifdef __MINGW32__
    char *defaulthomedir = "c:";
    char *homedir = getenv("HOME");
#else
    uid_t me;
    struct passwd *my_passwd;
#endif
    struct stat buf;
    char *path;

    errno = 0;

    /* Query whatever database to get user's home dir */
#ifdef __MINGW32__
    if (NULL == homedir) {
	homedir = defaulthomedir;
    }

    len = strlen(homedir) + 8;	/* + "/.grass\0" */
    if (NULL == (path = G_calloc(1, len))) {
	return NULL;
    }
    sprintf(path, "%s%s", homedir, "/.grass");
#else
    me = getuid();
    my_passwd = getpwuid(me);
    if (my_passwd == NULL)
	return NULL;

    len = strlen(my_passwd->pw_dir) + 8;	/* + "/.grass\0" */
    if (NULL == (path = G_calloc(1, len)))
	return NULL;

    sprintf(path, "%s%s", my_passwd->pw_dir, "/.grass");
#endif

    status = G_lstat(path, &buf);

    /* If errno == ENOENT, the directory doesn't exist */
    if (status != 0) {
	if (errno == ENOENT) {
	    status = G_mkdir(path);

	    if (status != 0) {	/* mkdir failed */
		G_free(path);
		return NULL;
	    }

	    /* override umask settings, if possible */
	    chmod(path, S_IRWXU);

	    /* otherwise mkdir succeeded, we're done here */
	    return path;
	}

	/* other errors should not be defined ??? give up */
	G_free(path);
	return NULL;
    }
    /* implicit else */

    /* Examine the stat "buf" */
    /* It better be a directory */
    if (!S_ISDIR(buf.st_mode)) {	/* File, link, something else */
	errno = ENOTDIR;	/* element is not a directory, but should be */
	G_free(path);
	return NULL;
    }

    /* No read/write/execute ??? */
    if (!((S_IRUSR & buf.st_mode) &&
	  (S_IWUSR & buf.st_mode) && (S_IXUSR & buf.st_mode)
	)
	) {
	errno = EACCES;		/* Permissions error */
	G_free(path);
	return NULL;
    }

    /* We'll assume that if the user grants greater permissions
     * than we would, that they know what they're doing
     * -- so we're done here...
     */

    return path;
}


/**************************************************************************
 * _elem_count_split: Does a couple things:
 * 1) Counts the number of elements in "elems"
 * 2) Replaces occurrences of '/' with '\0'
 * 3) Checks that no element begins with a '.'
 * 4) Checks there are no '//'
 *
 * Therefore, THE STRING THAT IS PASSED IN IS MODIFIED
 * Returns 0 if there are no elements, or an element
 * beginning with a '.' or containing a '//' is found.
 *************************************************************************/
static int _elem_count_split(char *elems)
{
    int i;
    size_t len;
    char *begin, *end;

    /* Some basic assertions */
    assert(elems != NULL);
    assert((len = strlen(elems)) > 0);
    assert(*elems != '/');

    begin = elems;
    for (i = 0; begin != NULL && len > begin - elems; i++) {
	/* check '.' condition */
	if (*begin == '.')
	    return 0;
	end = strchr(begin, '/');
	/* check '//' condition */
	if (end != NULL && end == begin)
	    return 0;
	/* okay, change '/' into '\0' */
	begin = end;
	if (begin != NULL) {
	    *begin = '\0';	/* begin points at '/', change it */
	    begin++;		/* increment begin to next char */
	}
    }

    /* That's it */
    return i;
}


/**************************************************************************
 * _make_sublevels(): creates subelements as necessary from the passed
 * "elems" string.  It returns the full path if successful or NULL
 * if it fails.  "elems" must not be NULL, zero length, or have any
 * elements that begin with a '.' or any occurrences of '//'.
 *************************************************************************/
static char *_make_sublevels(const char *elems)
{
    int i, status;
    char *cp, *path, *top, *ptr;
    struct stat buf;

    /* Get top level path */
    if (NULL == (top = _make_toplevel()))
	return NULL;

    /* Make a copy of elems */
    if (NULL == (cp = G_store(elems))) {
	G_free(top);
	return NULL;
    }

    /* Do element count, sanity checking and "splitting" */
    if ((i = _elem_count_split(cp)) < 1) {
	G_free(cp);
	G_free(top);
	return NULL;
    }

    /* Allocate our path to be large enough */
    if ((path = G_calloc(1, strlen(top) + strlen(elems) + 2)) == NULL) {
	G_free(top);
	G_free(cp);
	return NULL;
    }

    /* Now loop along adding directories if they don't exist
     * make sure the thing is a directory as well.
     * If there was a trailing '/' in the original "elem", it doesn't
     * make it into the returned path.
     */
    for (; i > 0; i--) {
	sprintf(path, "%s/%s", top, cp);
	errno = 0;
	status = G_lstat(path, &buf);
	if (status != 0) {
	    /* the element doesn't exist */
	    status = G_mkdir(path);
	    if (status != 0) {
		/* Some kind of problem... */
		G_free(top);
		G_free(cp);
		return NULL;
	    }
	    /* override umask settings, if possible */
	    chmod(path, S_IRWXU);
	}
	else {
	    /* Examine the stat "buf" */
	    /* It better be a directory */
	    if (!S_ISDIR(buf.st_mode)) {	/* File, link, something else */
		errno = ENOTDIR;	/* element is not a directory, but should be */
		G_free(path);
		return NULL;
	    }

	    /* No read/write/execute ??? */
	    if (!((S_IRUSR & buf.st_mode) &&
		  (S_IWUSR & buf.st_mode) && (S_IXUSR & buf.st_mode)
		)
		) {
		errno = EACCES;	/* Permissions error */
		G_free(path);
		return NULL;
	    }

	    /* okay continue ... */
	}

	ptr = strchr(cp, '\0');
	*ptr = '/';
    }

    /* All done, free memory */
    G_free(top);
    G_free(cp);

    return path;
}


/**
 * \brief Returns path to <b>element</b> and <b>item</b>.
 *
 * Either <b>element</b> or <b>item</b> can be NULL, but not both.  If 
 * <b>element</b> is NULL, then the file is assumed to live at the top 
 * level. If file is NULL, then it is assumed the caller is not 
 * interested in the file.  If the element or rc dir do not exist, they 
 * are created. However, the file is never checked for.
 *
 * \param[in] element
 * \param[in] item
 * \return Pointer to string path
 */

char *G_rc_path(const char *element, const char *item)
{
    size_t len;
    char *path, *ptr;

    assert(!(element == NULL && item == NULL));

    /* Simple item in top-level */
    if (element == NULL) {
	path = _make_toplevel();
    }
    else if (item == NULL) {
	return _make_sublevels(element);
    }
    else {
	path = _make_sublevels(element);
    }


    assert(*item != '.');
    assert(path != NULL);
    ptr = strchr(item, '/');	/* should not have slashes */
    assert(ptr == NULL);
    len = strlen(path) + strlen(item) + 2;
    if ((ptr = G_realloc(path, len)) == NULL) {
	G_free(path);
	return NULL;
    }
    path = ptr;
    ptr = strchr(path, '\0');
    sprintf(ptr, "/%s", item);

    return path;
}				/* G_rc_path */


/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
#endif
