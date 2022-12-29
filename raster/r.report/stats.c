#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>
#include "global.h"

static int die(void);

int get_stats(void)
{
    char buf[1024];
    int i, nl, ns;
    FILE *fd;
    char **tokens;

    if (stats_flag == EVERYTHING)
	stats_file = G_tempfile();

    if (stats_flag != REPORT_ONLY) {
	char tmp[50];
	int n_argv = 50;
	const char **argv = G_calloc(n_argv, sizeof(*argv));
	int argc = 0;

	argv[argc++] = "r.stats";
	argv[argc++] = "-acr";

	/* if (!masking) argv[argc++] = "-m"; */
	if (G_verbose() == G_verbose_min())
	    argv[argc++] = "--quiet";

	if (no_nulls)
	    argv[argc++] = "-n";

	if (no_nulls_all)
	    argv[argc++] = "-N";

	if (as_int)
	    argv[argc++] = "-i";

	if (cat_ranges)
	    argv[argc++] = "-C";
	else if (nsteps != 255) {
	    sprintf(tmp, "nsteps=%d", nsteps);
	    argv[argc++] = tmp;
	}

	argv[argc++] = "separator=:";

	argv[argc++] = SF_REDIRECT_FILE;
	argv[argc++] = SF_STDOUT;
	argv[argc++] = SF_MODE_OUT;
	argv[argc++] = stats_file;

        if (do_sort == SORT_ASC)
            argv[argc++] = "sort=asc";
        else if (do_sort == SORT_DESC)
            argv[argc++] = "sort=desc";
        
	for (i = 0; i < nlayers; i++) {
	    char *name = G_fully_qualified_name(layers[i].name, layers[i].mapset);
	    char *buf = G_malloc(6 + strlen(name) + 1);

	    sprintf(buf, "input=%s", name);
	    G_free(name);

	    if (argc + 1 >= n_argv) {
		n_argv += 50;
		argv = G_realloc(argv, n_argv * sizeof(*argv));
	    }

	    argv[argc++] = buf;
	}

	argv[argc++] = NULL;

	if (G_vspawn_ex(argv[0], argv) != 0) {
	    remove(stats_file);
	    G_fatal_error("error running r.stats");
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
		    Rast_set_c_null_value(&Gstats[ns].cats[nl], 1);
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
