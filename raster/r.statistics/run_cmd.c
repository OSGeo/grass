#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/gis.h>
#include "method.h"

FILE *run_stats(struct Popen *child,
		const char *basemap, const char *covermap, const char *mode)
{
    char input[6 + GNAME_MAX + 1 + GMAPSET_MAX + 1 + GNAME_MAX + 1 + GMAPSET_MAX + 1];
    const char *argv[5];
    FILE *fp;

    sprintf(input, "input=%s,%s", basemap, covermap);

    argv[0] = "r.stats";
    argv[1] = mode;
    argv[2] = input;
    argv[3] = "separator=space";
    argv[4] = NULL;

    /* maybe use r.stats's output= option instead of reading from stdout here, whatever's easier. */
    fp = G_popen_read(child, argv[0], argv);
    if (!fp)
	G_fatal_error("error running r.stats");

    return fp;
}

FILE *run_reclass(struct Popen *child, const char *basemap, const char *outputmap)
{
    char input[6 + GNAME_MAX + 1 + GMAPSET_MAX + 1];
    char output[7 + GNAME_MAX + 1];
    const char *argv[5];
    FILE *fp;

    sprintf(input, "input=%s", basemap);
    sprintf(output, "output=%s", outputmap);

    argv[0] = "r.reclass";
    argv[1] = input;
    argv[2] = output;
    argv[3] = "rules=-";
    argv[4] = NULL;

    fp = G_popen_write(child, argv[0], argv);
    if (!fp)
	G_fatal_error("error running r.stats");

    return fp;
}

