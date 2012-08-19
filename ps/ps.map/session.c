#include <stdio.h>
#include <grass/gis.h>
#include "local_proto.h"

/* TODO: when done, 
 * remove session file or save it to a user-specified location */

static char cur[2000];
static char *sessionfile = NULL;
static FILE *fd;

int add_to_session(int indent, char *buf)
{
    accept();
    sprintf(cur, "%s%s", indent ? "  " : "", buf);

    return 0;
}

int accept(void)
{
    if (sessionfile == NULL) {
	*cur = 0;
	sessionfile = G_tempfile();
	fd = fopen(sessionfile, "w");
	if (fd == NULL) {
	    error("session file", "", "can't open");
	    return 1;
	}
    }
    if (fd != NULL && *cur) {
	fprintf(fd, "%s\n", cur);
	fflush(fd);
	*cur = 0;
    }

    return 0;
}

int reject(void)
{
    *cur = 0;

    return 0;
}

int print_session(FILE * out)
{
    FILE *in;
    char buf[1024];

    if (sessionfile == NULL)
	return 1;
    if (fd != NULL)
	fflush(fd);
    if ((in = fopen(sessionfile, "r")) == NULL) {
	error("session file", "", "can't open");
	return 1;
    }
    while (fgets(buf, sizeof buf, in))
	fprintf(out, "%s", buf);
    fclose(in);

    return 0;
}
