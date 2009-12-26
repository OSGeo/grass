#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include "method.h"

void run_stats(const char *basemap, const char *covermap, const char *mode,
	       const char *tempfile)
{
    char buf[6 + GNAME_MAX + 1 + GMAPSET_MAX + 1 + GNAME_MAX + 1 + GMAPSET_MAX + 1];
    const char *argv[10];
    int argc = 0;

    argv[argc++] = "r.stats";

    argv[argc++] = mode;

    argv[argc++] = "-n";

    sprintf(buf, "input=%s,%s", basemap, covermap);
    argv[argc++] = buf;

    argv[argc++] = "fs=space";

    argv[argc++] = SF_REDIRECT_FILE;
    argv[argc++] = SF_STDOUT;
    argv[argc++] = SF_MODE_OUT;
    argv[argc++] = tempfile;

    argv[argc++] = NULL;

    if (G_vspawn_ex(argv[0], argv) != 0) {
	remove(tempfile);
	G_fatal_error("error running r.stats");
    }
}

int run_reclass(const char *basemap, const char *outputmap, const char *tempfile)
{
    char buf1[6 + GNAME_MAX + 1 + GMAPSET_MAX + 1];
    char buf2[7 + GNAME_MAX + 1];
    const char *argv[8];
    int argc = 0;

    argv[argc++] = "r.reclass";

    sprintf(buf1, "input=%s", basemap);
    argv[argc++] = buf1;

    sprintf(buf2, "output=%s", outputmap);
    argv[argc++] = buf2;

    argv[argc++] = SF_REDIRECT_FILE;
    argv[argc++] = SF_STDIN;
    argv[argc++] = SF_MODE_IN;
    argv[argc++] = tempfile;

    argv[argc++] = NULL;

    return G_vspawn_ex(argv[0], argv);
}

