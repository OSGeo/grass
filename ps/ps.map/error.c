#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include "local_proto.h"
extern FILE *tracefd;

int error(const char *a, const char *b, const char *c)
{
    char msg[2000];

    sprintf(msg, "%s%s%s : %s", a, *b ? " " : "", b, c);

    if (tracefd != NULL
	&& !(isatty(fileno(tracefd)) && isatty(fileno(stderr))))
	fprintf(tracefd, "## error: %s\n", c);

    if (isatty(0))
	fprintf(stderr, "%s\n", msg);
    else
	G_fatal_error("%s", msg);

    reject();

    return 0;
}
