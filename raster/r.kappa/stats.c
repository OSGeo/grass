#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/spawn.h>
#include "kappa.h"
#include <grass/glocale.h>
#include "local_proto.h"


static void die(void)
{
    unlink(stats_file);
    G_fatal_error(_("Problem reading r.stats output"));
}


int stats(void)
{
    char buf[1024];
    char mname[GNAME_MAX], rname[GNAME_MAX];
    const char *mmapset, *rmapset;
    int i, nl;
    size_t ns;
    FILE *fd;
    char **tokens;
    const char *argv[9];
    int argc = 0;

    strcpy(mname, maps[1]);
    mmapset = G_find_raster2(mname, "");
    if (mmapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), maps[0]);

    strcpy(rname, maps[0]);
    rmapset = G_find_raster2(rname, "");
    if (rmapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), maps[1]);

    stats_file = G_tempfile();

    argv[argc++] = "r.stats";

    argv[argc++] = "-cin";

    argv[argc++] = "separator=:";

    sprintf(buf, "input=%s,%s",
	    G_fully_qualified_name(mname, mmapset),
	    G_fully_qualified_name(rname, rmapset));
    argv[argc++] = buf;

    argv[argc++] = SF_REDIRECT_FILE;
    argv[argc++] = SF_STDOUT;
    argv[argc++] = SF_MODE_OUT;
    argv[argc++] = stats_file;

    argv[argc++] = NULL;

    if (G_vspawn_ex(argv[0], argv) != 0) {
	remove(stats_file);
	G_fatal_error("error running r.stats");
    }

    fd = fopen(stats_file, "r");
    if (fd == NULL) {
	unlink(stats_file);
	sprintf(buf, "Unable to open result file <%s>\n", stats_file);
    }

    while (G_getl(buf, sizeof buf, fd)) {
	tokens = G_tokenize(buf, ":");
	i = 0;
	ns = nstats++;
	Gstats = (GSTATS *) G_realloc(Gstats, nstats * sizeof(GSTATS));
	Gstats[ns].cats = (long *)G_calloc(nlayers, sizeof(long));
	for (nl = 0; nl < nlayers; nl++) {
	    if (sscanf(tokens[i++], "%ld", &Gstats[ns].cats[nl]) != 1)
		die();
	}
	if (sscanf(tokens[i++], "%ld", &Gstats[ns].count) != 1)
	    die();
	G_free_tokens(tokens);
    }
    fclose(fd);
    unlink(stats_file);

    return 0;
}
