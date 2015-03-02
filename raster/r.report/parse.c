#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/glocale.h>
#include "global.h"

int parse_command_line(int argc, char *argv[])
{
    int i;
    char *desc;
    struct
    {
	struct Option *cell;
	struct Option *units;
	struct Option *pl;	/* page length */
	struct Option *pw;	/* page width */
	struct Option *outfile;
	struct Option *nv;
	struct Option *nsteps;
        struct Option *sort;
    } parms;
    struct
    {
	struct Flag *f;
	struct Flag *m;
	struct Flag *h;
	struct Flag *q;
	struct Flag *e;
	struct Flag *n;
	struct Flag *N;
	struct Flag *i;		/* use quant rules for fp map, 
				   i.e. read it as int */
	struct Flag *C;		/*  report for fp ranges in Cats file 
				   (fp maps only) */
    } flags;

    parms.cell = G_define_standard_option(G_OPT_R_MAPS);
    parms.cell->description = _("Name of raster map(s) to report on");

    parms.units = G_define_option();
    parms.units->key = "units";
    parms.units->type = TYPE_STRING;
    parms.units->required = NO;
    parms.units->multiple = YES;
    parms.units->description = _("Units to report");
    desc = NULL;
    G_asprintf(&desc,
	       "mi;%s;me;%s;k;%s;a;%s;h;%s;c;%s;p;%s",
	       _("area in square miles"),
	       _("area in square meters"),
	       _("area in square kilometers"),
	       _("area in acres"),
	       _("area in hectares"),
	       _("number of cells"),
	       _("percent cover"));
    parms.units->descriptions = desc;
    parms.units->options = "mi,me,k,a,h,c,p";
    parms.units->guisection = _("Statistics");

    parms.outfile = G_define_standard_option(G_OPT_F_OUTPUT);
    parms.outfile->required = NO;
    parms.outfile->label =
	_("Name for output file to hold the report");
    parms.outfile->description =
	_("If no output file given report is printed to standard output");

    parms.nv = G_define_standard_option(G_OPT_M_NULL_VALUE);
    parms.nv->answer = "*";
    parms.nv->guisection = _("Formatting");

    parms.pl = G_define_option();
    parms.pl->key = "page_length";
    parms.pl->type = TYPE_INTEGER;
    parms.pl->required = NO;
    parms.pl->description = _("Page length");
    parms.pl->answer = DEFAULT_PAGE_LENGTH;
    parms.pl->guisection = _("Formatting");

    parms.pw = G_define_option();
    parms.pw->key = "page_width";
    parms.pw->type = TYPE_INTEGER;
    parms.pw->required = NO;
    parms.pw->description = _("Page width");
    parms.pw->answer = DEFAULT_PAGE_WIDTH;
    parms.pw->guisection = _("Formatting");

		    parms.nsteps = G_define_option();
    parms.nsteps->key = "nsteps";
    parms.nsteps->type = TYPE_INTEGER;
    parms.nsteps->required = NO;
    parms.nsteps->multiple = NO;
    parms.nsteps->answer = "255";
    parms.nsteps->description =
	_("Number of floating-point subranges to collect stats from");
    parms.nsteps->guisection = _("Floating point");

    parms.sort = G_define_option();
    parms.sort->key = "sort";
    parms.sort->type = TYPE_STRING;
    parms.sort->required = NO;
    parms.sort->multiple = NO;
    parms.sort->label = _("Sort output statistics by cell counts");
    parms.sort->description = _("Default: sorted by categories or intervals");
    parms.sort->options = "asc,desc";
    G_asprintf((char **)&(parms.sort->descriptions),
               "asc;%s;desc;%s",
               _("Sort by cell counts in ascending order"),
               _("Sort by cell counts in descending order"));
    parms.sort->guisection = _("Formatting");

    flags.h = G_define_flag();
    flags.h->key = 'h';
    flags.h->description = _("Suppress page headers");
    flags.h->guisection = _("Formatting");

    flags.f = G_define_flag();
    flags.f->key = 'f';
    flags.f->description = _("Use formfeeds between pages");
    flags.f->guisection = _("Formatting");

    flags.e = G_define_flag();
    flags.e->key = 'e';
    flags.e->description = _("Scientific format");
    flags.e->guisection = _("Formatting");

    flags.n = G_define_flag();
    flags.n->key = 'n';
    flags.n->description = _("Do not report no data value");
    flags.n->guisection = _("No data");

    flags.N = G_define_flag();
    flags.N->key = 'a';
    flags.N->description = _("Do not report cells where all maps have no data");
    flags.N->guisection = _("No data");

    flags.C = G_define_flag();
    flags.C->key = 'c';
    flags.C->description = _("Report for cats floating-point ranges (floating-point maps only)");
    flags.C->guisection = _("Floating point");

    flags.i = G_define_flag();
    flags.i->key = 'i';
    flags.i->description =
	_("Read floating-point map as integer (use map's quant rules)");
    flags.i->guisection = _("Floating point");

    /* hidden feature.
     * if first arg is >file just run r.stats into this file and quit
     * if first arg is <file, run report from stats in file
     * (this feature is for the interactive version of this program -
     *  to get more than one report without re-running r.stats)
     */
    stats_flag = EVERYTHING;
    if (argc > 1) {
	if (argv[1][0] == '<' || argv[1][0] == '>') {
	    stats_file = argv[1] + 1;
	    if (argv[1][0] == '<')
		stats_flag = REPORT_ONLY;
	    else {
		unlink(stats_file);
		stats_flag = STATS_ONLY;
	    }
	    argc--;
	    argv++;
	}
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    use_formfeed = flags.f->answer;
    with_headers = !flags.h->answer;
    e_format = flags.e->answer;
    no_nulls = flags.n->answer;
    no_nulls_all = flags.N->answer;
    cat_ranges = flags.C->answer;
    as_int = flags.i->answer;

    for (i = 0; parms.cell->answers[i]; i++)
	parse_layer(parms.cell->answers[i]);
    if (parms.units->answers)
	for (i = 0; parms.units->answers[i]; i++)
	    parse_units(parms.units->answers[i]);

    sscanf(parms.nsteps->answer, "%d", &nsteps);
    if (nsteps <= 0) {
	G_warning(_("nsteps has to be > 0; using nsteps=255"));
	nsteps = 255;
    }

    if (sscanf(parms.pl->answer, "%d", &page_length) != 1 ||
        page_length < 0) {
      G_fatal_error(_("Illegal page length"));
    }

    if (sscanf(parms.pw->answer, "%d", &page_width) != 1 ||
        page_width < 1) {
      G_fatal_error(_("Illegal page width"));
    }

    if (parms.outfile->answer) {
	if (freopen(parms.outfile->answer, "w", stdout) == NULL) {
	    perror(parms.outfile->answer);
	    exit(EXIT_FAILURE);
	}
    }
    no_data_str = parms.nv->answer;

    /* determine sorting method */
    do_sort = SORT_DEFAULT; /* sort by cats by default */
    if (parms.sort->answer) {
        switch(parms.sort->answer[0]) {
        case 'a':
            do_sort = SORT_ASC;
            break;
        case 'd':
            do_sort = SORT_DESC;
            break;
        default:
            G_debug(1, "Sorting by '%s' not supported", parms.sort->answer);
            break;
        }
    }

    return 0;
}

int parse_units(char *s)
{
    int x;

    if (match(s, "miles", 2))
	x = SQ_MILES;
    else if (match(s, "meters", 2))
	x = SQ_METERS;
    else if (match(s, "kilometers", 1))
	x = SQ_KILOMETERS;
    else if (match(s, "acres", 1))
	x = ACRES;
    else if (match(s, "hectares", 1))
	x = HECTARES;
    else if (match(s, "cell_counts", 1))
	x = CELL_COUNTS;
    else if (match(s, "counts", 1))
	x = CELL_COUNTS;
    else if (match(s, "percent_cover", 1))
	x = PERCENT_COVER;
    else {
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (nunits >= MAX_UNITS) {
	G_fatal_error(_("Only %d unit%s allowed"),
		      MAX_UNITS, MAX_UNITS == 1 ? "" : "s");
    }
    unit[nunits].type = x;
    nunits++;

    return 0;
}

int parse_layer(char *s)
{
    char name[GNAME_MAX];
    const char *mapset;
    struct FPRange fp_range;
    int n;

    strcpy(name, s);
    mapset = G_find_raster2(name, "");

    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), s);

    n = nlayers++;
    layers = (LAYER *) G_realloc(layers, nlayers * sizeof(LAYER));
    is_fp = (int *)G_realloc(is_fp, (nlayers + 1) * sizeof(int));
    DMAX = (DCELL *) G_realloc(DMAX, (nlayers + 1) * sizeof(DCELL));
    DMIN = (DCELL *) G_realloc(DMIN, (nlayers + 1) * sizeof(DCELL));
    if (!as_int)
	is_fp[n] = Rast_map_is_fp(name, mapset);
    else
	is_fp[n] = 0;
    if (is_fp[n]) {
	if (Rast_read_fp_range(name, mapset, &fp_range) < 0)
	    G_fatal_error(_("Unable to read fp range for raster map <%s>"),
			  name);
	Rast_get_fp_range_min_max(&fp_range, &DMIN[n], &DMAX[n]);
    }

    layers[n].name = G_store(name);
    layers[n].mapset = mapset;
    if (Rast_read_cats(name, mapset, &layers[n].labels) < 0)
	G_fatal_error(_("Unable to read category file of raster map <%s@%s>"),
		      name, mapset);

    return 0;
}

int match(char *s, char *key, int min)
{
    size_t len;

    len = strlen(s);
    if ((int) len < min)
	return 0;
    return strncmp(s, key, len) == 0;
}
