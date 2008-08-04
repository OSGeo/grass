#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
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
    char mname[GNAME_MAX], rname[GMAPSET_MAX];
    char *mmapset, *rmapset;
    int i, nl;
    size_t ns;
    FILE *fd;
    char **tokens;

    strcpy(mname, maps[0]);
    mmapset = G_find_cell2(mname, "");
    if (mmapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), maps[0]);

    strcpy(rname, maps[1]);
    rmapset = G_find_cell2(rname, "");
    if (rmapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), maps[1]);

    stats_file = G_tempfile();
    strcpy(buf, "r.stats -cin");
    strcat(buf, " fs=:");
    strcat(buf, " input=");
    strcat(buf, G_fully_qualified_name(maps[0], mmapset));
    strcat(buf, ",");
    strcat(buf, G_fully_qualified_name(maps[1], rmapset));
    strcat(buf, " > ");
    strcat(buf, stats_file);

    if (system(buf)) {
	unlink(stats_file);
	exit(1);
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
