#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>


int read_stats(FILE * fd, long *cat1, long *cat2, double *value)
{
    char buf[1024];

    if (fgets(buf, sizeof(buf), fd) == NULL)
	return 0;

    if (sscanf(buf, "%ld %ld %lf", cat1, cat2, value) == 3) {
	G_debug(3, "base: %ld  cover: %ld  val: %lf", *cat1, *cat2, *value);

	return 1;
    }

    G_fatal_error(_("Reading r.stats output"));

    return -1;
}
