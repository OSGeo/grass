/*!
 * \file lib/gis/mkstemp.c
 *
 * \brief GIS Library - Temporary file functions.
 *
 * (C) 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define MAX_REPLACE 5

static int next(char **replace, int num_replace)
{
    int i;

    for (i = 0; i < num_replace; i++) {
	char *p = replace[i];
	if (*p < 'z') {
	    (*p)++;
	    return 1;
	}
	else
	    *p = 'a';
    }

    return 0;
}

static int G__mkstemp(char *template, int flags, int mode)
{
    char *replace[MAX_REPLACE];
    int num_replace = 0;
    char *ptr = template;
    int fd;

    while (num_replace < MAX_REPLACE) {
	char *p = strchr(ptr, 'X');
	if (!p)
	    break;
	replace[num_replace++] = p;
	*p = 'a';
	ptr = p + 1;
    }

    if (!num_replace)
	return -1;

    for (;;) {
	if (!next(replace, num_replace))
	    return -1;

	if (access(template, F_OK) == 0)
	    continue;

	if (!flags)
	    return 0;

	fd = open(template, flags, mode);
	if (fd < 0) {
	    if (errno == EEXIST)
		continue;
	    return -1;
	}

	return fd;
    }

    return -1;
}


/*!
 * \brief Opens a temporary file.
 *
 * This routine opens the file.
 *
 * The last two take the arguments "flags" and "mode". "flags" should be
 * O_WRONLY or O_RDWR, plus any other desired flags (e.g. O_APPEND).
 * "mode" is the file mode (0666 would be typical).
 *
 * The functions does not use the PID, although the caller can do so.
 *
 * In theory, up to 26^5 (= ~12 million) filenames will be attempted
 * until it finds one which doesn't exist.
 *
 * <b>Note:</b> <i>G_mktemp()</i> as such it is prone to race
 * conditions (some other process may create that file after G_mktemp()
 * returns).
 *
 * \return file name
 */
char *G_mktemp(char *template)
{
    return G__mkstemp(template, 0, 0) < 0 ? NULL : template;
}

/*!
 * \brief Returns a file descriptor.
 *
 * This routine opens the file and returns a descriptor.
 *
 * The last two take the arguments "flags" and "mode". "flags" should be
 * O_WRONLY or O_RDWR, plus any other desired flags (e.g. O_APPEND).
 * "mode" is the file mode (0666 would be typical).
 *
 * The functions does not use the PID, although the caller can do so.
 *
 * In theory, up to 26^5 (= ~12 million) filenames will be attempted
 * until it finds one which doesn't exist.
 *
 *
 * \return file descriptor
 */

int G_mkstemp(char *template, int flags, int mode)
{

    switch (flags & O_ACCMODE) {
    case O_RDONLY:
	G_fatal_error(_("Attempt to create read-only temporary file"));
	return -1;
    case O_WRONLY:
    case O_RDWR:
	break;
    default:
	G_fatal_error(_("Unrecognised access mode: %o"), flags & O_ACCMODE);
	return -1;
    }

    return G__mkstemp(template, flags | O_CREAT | O_EXCL, mode);
}

/*!
 * \brief Returns a file descriptor.
 *
 * This routine opens the file and returns a FILE*.
 *
 * The last two take the arguments "flags" and "mode". "flags" should be
 * O_WRONLY or O_RDWR, plus any other desired flags (e.g. O_APPEND).
 * "mode" is the file mode (0666 would be typical).
 *
 * The functions does not use the PID, although the caller can do so.
 *
 * In theory, up to 26^5 (= ~12 million) filenames will be attempted
 * until it finds one which doesn't exist.
 *
 * \return FILE*
 */

FILE *G_mkstemp_fp(char *template, int flags, int mode)
{
    const char *fmode = ((flags & O_ACCMODE) == O_RDWR)
	? ((flags & O_APPEND) ? "a+" : "w+")
	: ((flags & O_APPEND) ? "a" : "w");
    int fd = G_mkstemp(template, flags, mode);
    if (fd < 0)
	return NULL;
    return fdopen(fd, fmode);
}
