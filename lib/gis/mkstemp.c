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


char *G_mktemp(char *template)
{
    return G__mkstemp(template, 0, 0) < 0 ? NULL : template;
}

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

FILE *G_mkstemp_fp(char *template, int flags, int mode)
{
    const char *fmode = (flags & O_ACCMODE == O_RDWR)
	? ((flags & O_APPEND) ? "a+" : "w+")
	: ((flags & O_APPEND) ? "a" : "w");
    int fd = G_mkstemp(template, flags, mode);
    if (fd < 0)
	return NULL;
    return fdopen(fd, fmode);
}
