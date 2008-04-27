#include <unistd.h>
#include <grass/gis.h>
#include "options.h"

static int line = 0;

int read_line (char *buf, int n)
{
    if (!G_getl (buf, n, Infile))
	return 0;
    G_strip (buf);
    line++;
    return 1;
}

int bad_line (char *buf)
{
    if (isatty(fileno(Infile)))
	fprintf (stderr, "???\n");
    else
	fprintf (stderr, "%s: WARNING: line %d invalid: %s\n",
		G_program_name(), line, buf);

    return 0;
}
