
/****************************************************************************
 *
 * MODULE:       r.stats
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Brad Douglas <rez touchofmadness.com>,
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      calculates the area present in each of the categories of
 *               user-selected raster map layer(s)
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/glocale.h>
#include "global.h"

char *no_data_str;
int nfiles;
int nrows;
int ncols, no_nulls, no_nulls_all;
int nsteps, cat_ranges, raw_output, as_int, averaged;
int *is_fp;
DCELL *DMAX, *DMIN;

CELL NULL_CELL;
int (*get_row) ();

char fs[2];
struct Categories *labels;

int main(int argc, char *argv[])
{
    int *fd;
    char **names;
    char **ptr;
    char *name;

    /* flags */
    int raw_data;
    int with_coordinates;
    int with_xy;
    int with_percents;
    int with_counts;
    int with_areas;
    int with_labels;

    /* printf format */
    char fmt[20];
    int dp;
    struct Range range;
    struct FPRange fp_range;
    struct Quant q;
    CELL min, max, null_set = 0;
    DCELL dmin, dmax;
    struct GModule *module;
    struct
    {
	struct Flag *A;		/* print averaged values instead of intervals */
	struct Flag *a;		/* area */
	struct Flag *c;		/* cell counts */
	struct Flag *p;		/* percents */
	struct Flag *l;		/* with labels */
	struct Flag *q;		/* quiet */
	struct Flag *n;		/* Suppress reporting of any NULLs */
	struct Flag *N;		/* Suppress reporting of NULLs when 
				   all values are NULL */
	struct Flag *one;	/* one cell per line */
	struct Flag *x;		/*    with row/col */
	struct Flag *g;		/*    with east/north */
	struct Flag *i;		/* use quant rules for fp map, i.e. read it as int */
	struct Flag *r;		/*    raw output: when nsteps option is used,
				   report indexes of ranges instead of ranges
				   themselves; when -C (cats) option is used
				   reports indexes of fp ranges = ind. of labels */
	struct Flag *C;		/* report stats for labeled ranges in cats files */
    } flag;
    struct
    {
	struct Option *cell;
	struct Option *fs;
	struct Option *nv;
	struct Option *output;
	struct Option *nsteps;	/* divide data range into nsteps and report stats
				   for these ranges: only for fp maps
				   NOTE: when -C flag is used, and there are 
				   explicit fp ranges in cats or when the map 
				   is int, nsteps is ignored */
    } option;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Generates area statistics for raster map layers.");

    /* Define the different options */

    option.cell = G_define_standard_option(G_OPT_R_INPUTS);

    option.output = G_define_standard_option(G_OPT_F_OUTPUT);
    option.output->required = NO;
    option.output->description =
	_("Name for output file (if omitted or \"-\" output to stdout)");

    option.fs = G_define_standard_option(G_OPT_F_SEP);
    option.fs->answer = "space";

    option.nv = G_define_option();
    option.nv->key = "nv";
    option.nv->type = TYPE_STRING;
    option.nv->required = NO;
    option.nv->multiple = NO;
    option.nv->answer = "*";
    option.nv->description = _("String representing no data cell value");

    option.nsteps = G_define_option();
    option.nsteps->key = "nsteps";
    option.nsteps->type = TYPE_INTEGER;
    option.nsteps->required = NO;
    option.nsteps->multiple = NO;
    option.nsteps->answer = "255";
    option.nsteps->description =
	_("Number of fp subranges to collect stats from");

    /* Define the different flags */

    flag.one = G_define_flag();
    flag.one->key = '1';
    flag.one->description = _("One cell (range) per line");

    flag.A = G_define_flag();
    flag.A->key = 'A';
    flag.A->description = _("Print averaged values instead of intervals");
    flag.A->guisection = _("Print");

    flag.a = G_define_flag();
    flag.a->key = 'a';
    flag.a->description = _("Print area totals in square meters");
    flag.a->guisection = _("Print");

    flag.c = G_define_flag();
    flag.c->key = 'c';
    flag.c->description = _("Print cell counts");
    flag.c->guisection = _("Print");

    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description =
	_("Print APPROXIMATE percents (total percent may not be 100%)");
    flag.p->guisection = _("Print");

    flag.l = G_define_flag();
    flag.l->key = 'l';
    flag.l->description = _("Print category labels");
    flag.l->guisection = _("Print");

    flag.g = G_define_flag();
    flag.g->key = 'g';
    flag.g->description = _("Print grid coordinates (east and north)");
    flag.g->guisection = _("Print");

    flag.x = G_define_flag();
    flag.x->key = 'x';
    flag.x->description = _("Print x and y (column and row)");
    flag.x->guisection = _("Print");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Print raw indexes of fp ranges (fp maps only)");
    flag.r->guisection = _("Print");

    flag.n = G_define_flag();
    flag.n->key = 'n';
    flag.n->description = _("Suppress reporting of any NULLs");

    flag.N = G_define_flag();
    flag.N->key = 'N';
    flag.N->description =
	_("Suppress reporting of NULLs when all values are NULL");

    flag.C = G_define_flag();
    flag.C->key = 'C';
    flag.C->description = _("Report for cats fp ranges (fp maps only)");

    flag.i = G_define_flag();
    flag.i->key = 'i';
    flag.i->description = _("Read fp map as integer (use map's quant rules)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = option.output->answer;
    if (name != NULL && strcmp(name, "-") != 0) {
	if (NULL == freopen(name, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), name);
	}
    }

    sscanf(option.nsteps->answer, "%d", &nsteps);
    if (nsteps <= 0) {
	G_warning(_("'%s' must be greater than zero; using %s=255"),
		  option.nsteps->key, option.nsteps->key);
	nsteps = 255;
    }
    cat_ranges = flag.C->answer;

    averaged = flag.A->answer;
    raw_output = flag.r->answer;
    as_int = flag.i->answer;
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    fd = NULL;
    nfiles = 0;
    dp = -1;

    with_percents = flag.p->answer;
    with_counts = flag.c->answer;
    with_areas = flag.a->answer;
    with_labels = flag.l->answer;

    no_nulls = flag.n->answer;
    no_nulls_all = flag.N->answer;
    no_data_str = option.nv->answer;

    raw_data = flag.one->answer;
    with_coordinates = flag.g->answer;
    with_xy = flag.x->answer;
    if (with_coordinates || with_xy)
	raw_data = 1;

    /* get field separator */
    strcpy(fs, " ");
    if (option.fs->answer) {
	if (strcmp(option.fs->answer, "space") == 0)
	    *fs = ' ';
	else if (strcmp(option.fs->answer, "tab") == 0)
	    *fs = '\t';
	else if (strcmp(option.fs->answer, "\\t") == 0)
	    *fs = '\t';
	else
	    *fs = *option.fs->answer;
    }


    /* open all raster maps */
    if (option.cell->answers[0] == NULL)
	G_fatal_error(_("Raster map not found"));

    names = option.cell->answers;
    ptr = option.cell->answers;

    for (; *ptr != NULL; ptr++) {
	name = *ptr;
	fd = (int *)G_realloc(fd, (nfiles + 1) * sizeof(int));
	is_fp = (int *)G_realloc(is_fp, (nfiles + 1) * sizeof(int));
	DMAX = (DCELL *) G_realloc(DMAX, (nfiles + 1) * sizeof(DCELL));
	DMIN = (DCELL *) G_realloc(DMIN, (nfiles + 1) * sizeof(DCELL));

	fd[nfiles] = Rast_open_old(name, "");
	if (!as_int)
	    is_fp[nfiles] = Rast_map_is_fp(name, "");
	else {
	    is_fp[nfiles] = 0;
	    if (cat_ranges || nsteps != 255)
		G_warning(_("Raster map <%s> is reading as integer map! "
			    "Flag '-%c' and/or '%s' option will be ignored."),
			  name, flag.C->key, option.nsteps->key);
	}
	if (with_labels || (cat_ranges && is_fp[nfiles])) {
	    labels = (struct Categories *)
		G_realloc(labels, (nfiles + 1) * sizeof(struct Categories));
	    if (Rast_read_cats(name, "", &labels[nfiles]) < 0)
		Rast_init_cats("", &labels[nfiles]);
	}
	if (is_fp[nfiles])
	    /* floating point map */
	{
	    Rast_quant_init(&q);
	    if (cat_ranges) {
		if (!Rast_quant_nof_rules(&labels[nfiles].q)) {
		    G_warning(_("Cats for raster map <%s> are either missing or have no explicit labels. "
			       "Using %s=%d."),
			      name, option.nsteps->key, nsteps);
		    cat_ranges = 0;
		}
		else if (nsteps != 255)
		    G_warning(_("Flag '-%c' was given, using cats fp ranges of raster map <%s>, "
			       "ignoring '%s' option"),
			      flag.C->key, name, option.nsteps->key);
	    }
	    if (!cat_ranges) {	/* DO NOT use else here, cat_ranges can change */
		if (Rast_read_fp_range(name, "", &fp_range) < 0)
		    G_fatal_error(_("Unable to read fp range of raster map <%s>"),
				  name);
		Rast_get_fp_range_min_max(&fp_range, &DMIN[nfiles],
				       &DMAX[nfiles]);
		G_debug(3, "file %2d: dmin=%f  dmax=%f", nfiles, DMIN[nfiles], 
			DMAX[nfiles]);

		Rast_quant_add_rule(&q, DMIN[nfiles], DMAX[nfiles], 1, nsteps+1);

		/* set the quant rules for reading the map */
		Rast_set_quant_rules(fd[nfiles], &q);
		Rast_quant_get_limits(&q, &dmin, &dmax, &min, &max);
		G_debug(2, "overall: dmin=%f  dmax=%f,  qmin=%d  qmax=%d",
			dmin, dmax, min, max);

		Rast_quant_free(&q);
	    }
	    else {		/* cats ranges */

		/* set the quant rules for reading the map */
		Rast_set_quant_rules(fd[nfiles], &labels[nfiles].q);
		Rast_quant_get_limits(&labels[nfiles].q, &dmin, &dmax, &min,
				   &max);
	    }
	}
	else {
	    if (Rast_read_range(name, "", &range) < 0)
		G_fatal_error(_("Unable to read range for map <%s>"), name);
	    Rast_get_range_min_max(&range, &min, &max);
	}
	if (!null_set) {
	    null_set = 1;
	    NULL_CELL = max + 1;
	}
	else if (NULL_CELL < max + 1)
	    NULL_CELL = max + 1;

	nfiles++;
    }

    if (dp < 0)
	strcpy(fmt, "%lf");
    else
	sprintf(fmt, "%%.%dlf", dp);

    if (raw_data)
	raw_stats(fd, with_coordinates, with_xy, with_labels);
    else
	cell_stats(fd, with_percents, with_counts, with_areas, with_labels,
		   fmt);

    exit(EXIT_SUCCESS);
}
