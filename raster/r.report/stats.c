#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "global.h"

static int die(void);

int get_stats(void)
{
    char buf[1024];
    char tmp[50];
    int i, nl, ns;
    FILE *fd;
    char **tokens;

    if (stats_flag == EVERYTHING)
	stats_file = G_tempfile();

    if (stats_flag != REPORT_ONLY) {
	strcpy(buf, "r.stats -acr");
	/* if (!masking) strcat (buf, "m"); */
	if (!verbose)
	    strcat(buf, "q");
	if (no_nulls)
	    strcat(buf, "n");
	if (no_nulls_all)
	    strcat(buf, "N");
	if (as_int)
	    strcat(buf, "i");
	if (cat_ranges)
	    strcat(buf, "C");
	else if (nsteps != 255) {
	    sprintf(tmp, " nsteps=%d", nsteps);
	    strcat(buf, tmp);
	}

	strcat(buf, " fs=: \"input=");

	for (i = 0; i < nlayers; i++) {
	    if (i)
		strcat(buf, ",");
	    strcat(buf,
		   G_fully_qualified_name(layers[i].name, layers[i].mapset));
	}
	strcat(buf, "\"");

	strcat(buf, " > \"");
	strcat(buf, stats_file);
	strcat(buf, "\"");
	/*      G_fatal_error(buf); */
	if (system(buf)) {
	    if (stats_flag == EVERYTHING)
		unlink(stats_file);
	    exit(1);
	}
    }
    if (stats_flag == STATS_ONLY)
	return 0;

    fd = fopen(stats_file, "r");

    if (fd == NULL) {
	if (stats_flag == EVERYTHING)
	    unlink(stats_file);
	G_fatal_error(_("Unable to open result file <%s>"), stats_file);
    }
    while (G_getl(buf, sizeof buf, fd)) {
	tokens = G_tokenize(buf, ":");
	i = 0;
	ns = nstats++;
	Gstats = (GSTATS *) G_realloc(Gstats, nstats * sizeof(GSTATS));
	Gstats[ns].cats = (CELL *) G_calloc(nlayers, sizeof(long));
	for (nl = 0; nl < nlayers; nl++) {
	    if (sscanf(tokens[i], "%d", &Gstats[ns].cats[nl]) != 1) {
		if (tokens[i][0] == '*')
		    G_set_c_null_value(&Gstats[ns].cats[nl], 1);
		else
		    die();
	    }
	    i++;
	}
	if (sscanf(tokens[i++], "%lf", &Gstats[ns].area) != 1)
	    die();
	if (sscanf(tokens[i++], "%ld", &Gstats[ns].count) != 1)
	    die();
	G_free_tokens(tokens);
    }
    fclose(fd);
    if (stats_flag == EVERYTHING)
	unlink(stats_file);

    return 0;
}

static int die(void)
{
    if (stats_flag == EVERYTHING)
	unlink(stats_file);
    G_fatal_error(_("Problem reading r.stats output"));
}
