
/****************************************************************************
 *
 * MODULE:       r.neighbors
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Bob Covill <bcovill tekmap.ns.ca>,
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Jan-Oliver Wagner <jan intevation.de>,
 *               Radim Blazek <radim.blazek gmail.com>
 *
 * PURPOSE:      Makes each cell category value a function of the category values 
 *               assigned to the cells around it, and stores new cell values in an
 *               output raster map layer
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/stats.h>
#include "ncb.h"
#include "local_proto.h"

typedef int (*ifunc) (void);

struct menu
{
    stat_func *method;		/* routine to compute new value */
    stat_func_w *method_w;	/* routine to compute new value (weighted) */
    ifunc cat_names;		/* routine to make category names */
    int copycolr;		/* flag if color table can be copied */
    int half;			/* whether to add 0.5 to result */
    char *name;			/* method name */
    char *text;			/* menu display - full description */
};

#define NO_CATS 0

/* modify this table to add new methods */
static struct menu menu[] = {
    {c_ave, w_ave, NO_CATS, 1, 1, "average", "average value"},
    {c_median, w_median, NO_CATS, 1, 0, "median", "median value"},
    {c_mode, w_mode, NO_CATS, 1, 0, "mode", "most frequently occuring value"},
    {c_min, NULL, NO_CATS, 1, 0, "minimum", "lowest value"},
    {c_max, NULL, NO_CATS, 1, 0, "maximum", "highest value"},
    {c_range, NULL, NO_CATS, 1, 0, "range", "range value"},
    {c_stddev, w_stddev, NO_CATS, 0, 1, "stddev", "standard deviation"},
    {c_sum, w_sum, NO_CATS, 1, 0, "sum", "sum of values"},
    {c_count, w_count, NO_CATS, 0, 0, "count", "count of non-NULL values"},
    {c_var, w_var, NO_CATS, 0, 1, "variance", "statistical variance"},
    {c_divr, NULL, divr_cats, 0, 0, "diversity",
     "number of different values"},
    {c_intr, NULL, intr_cats, 0, 0, "interspersion",
     "number of values different than center value"},
    {c_quart1, w_quart1, NO_CATS, 1, 0, "quart1", "first quartile"},
    {c_quart3, w_quart3, NO_CATS, 1, 0, "quart3", "third quartile"},
    {c_perc90, w_perc90, NO_CATS, 1, 0, "perc90", "ninetieth percentile"},
    {c_quant, w_quant, NO_CATS, 1, 0, "quantile", "arbitrary quantile"},
    {0, 0, 0, 0, 0, 0, 0}
};

struct ncb ncb;

struct output
{
    const char *name;
    char title[1024];
    int fd;
    DCELL *buf;
    stat_func *method_fn;
    stat_func_w *method_fn_w;
    int copycolr;
    int half;
    ifunc cat_names;
    int map_type;
    double quantile;
};

static int find_method(const char *method_name)
{
    int i;

    for (i = 0; menu[i].name; i++)
	if (strcmp(menu[i].name, method_name) == 0)
	    return i;

    G_fatal_error(_("Unknown method <%s>"), method_name);

    return -1;
}

int main(int argc, char *argv[])
{
    char *p;
    int in_fd;
    int selection_fd;
    int num_outputs;
    struct output *outputs = NULL;
    int copycolr, weights, have_weights_mask;
    char *selection;
    RASTER_MAP_TYPE map_type;
    int row, col;
    int readrow;
    int nrows, ncols;
    int i, n;
    struct Colors colr;
    struct Cell_head cellhd;
    struct Cell_head window;
    struct History history;
    struct GModule *module;
    struct
    {
	struct Option *input, *output, *selection;
	struct Option *method, *size;
	struct Option *title;
	struct Option *weight;
	struct Option *gauss;
	struct Option *quantile;
    } parm;
    struct
    {
	struct Flag *align, *circle;
    } flag;

    DCELL *values;		/* list of neighborhood values */
    DCELL *values_tmp;		/* list of neighborhood values */
    DCELL(*values_w)[2];	/* list of neighborhood values and weights */
    DCELL(*values_w_tmp)[2];	/* list of neighborhood values and weights */

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("filter"));
    module->description =
	_("Makes each cell category value a "
	  "function of the category values assigned to the cells "
	  "around it, and stores new cell values in an output raster "
	  "map layer.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);

    parm.selection = G_define_standard_option(G_OPT_R_INPUT);
    parm.selection->key = "selection";
    parm.selection->required = NO;
    parm.selection->description = _("Name of an input raster map to select the cells which should be processed");

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->multiple = YES;

    parm.method = G_define_option();
    parm.method->key = "method";
    parm.method->type = TYPE_STRING;
    parm.method->required = NO;
    parm.method->answer = "average";
    p = G_malloc(1024);
    for (n = 0; menu[n].name; n++) {
	if (n)
	    strcat(p, ",");
	else
	    *p = 0;
	strcat(p, menu[n].name);
    }
    parm.method->options = p;
    parm.method->description = _("Neighborhood operation");
    parm.method->multiple = YES;
    parm.method->guisection = _("Neighborhood");

    parm.size = G_define_option();
    parm.size->key = "size";
    parm.size->type = TYPE_INTEGER;
    parm.size->required = NO;
    parm.size->description = _("Neighborhood size");
    parm.size->answer = "3";
    parm.size->guisection = _("Neighborhood");

    parm.title = G_define_option();
    parm.title->key = "title";
    parm.title->key_desc = "phrase";
    parm.title->type = TYPE_STRING;
    parm.title->required = NO;
    parm.title->description = _("Title of the output raster map");

    parm.weight = G_define_standard_option(G_OPT_F_INPUT);
    parm.weight->key = "weight";
    parm.weight->required = NO;
    parm.weight->description = _("Text file containing weights");

    parm.gauss = G_define_option();
    parm.gauss->key = "gauss";
    parm.gauss->type = TYPE_DOUBLE;
    parm.gauss->required = NO;
    parm.gauss->description = _("Sigma (in cells) for Gaussian filter");

    parm.quantile = G_define_option();
    parm.quantile->key = "quantile";
    parm.quantile->type = TYPE_DOUBLE;
    parm.quantile->required = NO;
    parm.quantile->multiple = YES;
    parm.quantile->description = _("Quantile to calculate for method=quantile");
    parm.quantile->options = "0.0-1.0";
    parm.quantile->answer = "0.5";

    flag.align = G_define_flag();
    flag.align->key = 'a';
    flag.align->description = _("Do not align output with the input");

    flag.circle = G_define_flag();
    flag.circle->key = 'c';
    flag.circle->description = _("Use circular neighborhood");
    flag.circle->guisection = _("Neighborhood");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sscanf(parm.size->answer, "%d", &ncb.nsize);
    if (ncb.nsize <= 0)
	G_fatal_error(_("Neighborhood size must be positive"));
    if (ncb.nsize % 2 == 0)
	G_fatal_error(_("Neighborhood size must be odd"));
    ncb.dist = ncb.nsize / 2;

    if (parm.weight->answer && flag.circle->answer)
	G_fatal_error(_("weight= and -c are mutually exclusive"));

    if (parm.weight->answer && parm.gauss->answer)
	G_fatal_error(_("weight= and gauss= are mutually exclusive"));

    ncb.oldcell = parm.input->answer;

    if (!flag.align->answer) {
	Rast_get_cellhd(ncb.oldcell, "", &cellhd);
	G_get_window(&window);
	Rast_align_window(&window, &cellhd);
	Rast_set_window(&window);
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* open raster maps */
    in_fd = Rast_open_old(ncb.oldcell, "");
    map_type = Rast_get_map_type(in_fd);

    /* process the output maps */
    for (i = 0; parm.output->answers[i]; i++)
	;
    num_outputs = i;

    for (i = 0; parm.method->answers[i]; i++)
	;
    if (num_outputs != i)
	G_fatal_error(_("output= and method= must have the same number of values"));

    outputs = G_calloc(num_outputs, sizeof(struct output));

    /* read the weights */
    weights = 0;
    ncb.weights = NULL;
    ncb.mask = NULL;
    if (parm.weight->answer) {
	read_weights(parm.weight->answer);
	weights = 1;
    }
    else if (parm.gauss->answer) {
	gaussian_weights(atof(parm.gauss->answer));
	weights = 1;
    }
    
    copycolr = 0;
    have_weights_mask = 0;

    for (i = 0; i < num_outputs; i++) {
	struct output *out = &outputs[i];
	const char *output_name = parm.output->answers[i];
	const char *method_name = parm.method->answers[i];
	int method = find_method(method_name);

	out->name = output_name;
	if (weights) {
	    if (menu[method].method_w) {
		out->method_fn = NULL;
		out->method_fn_w = menu[method].method_w;
	    }
	    else {
		if (parm.weight->answer) {
		    G_warning(_("Method %s not compatible with weighing window, using weight mask instead"),
			      method_name);
		    if (!have_weights_mask) {
			weights_mask();
			have_weights_mask = 1;
		    }
		}
		else if (parm.gauss->answer) {
		    G_warning(_("Method %s not compatible with Gaussian filter, using unweighed version instead"),
			      method_name);
		}
		
		out->method_fn = menu[method].method;
		out->method_fn_w = NULL;
	    }
	}
	else {
	    out->method_fn = menu[method].method;
	    out->method_fn_w = NULL;
	}
	out->half = menu[method].half;
	out->copycolr = menu[method].copycolr;
	out->cat_names = menu[method].cat_names;
	if (out->copycolr)
	    copycolr = 1;
	out->quantile = (parm.quantile->answer && parm.quantile->answers[i])
	    ? atof(parm.quantile->answers[i])
	    : 0;
	out->buf = Rast_allocate_d_buf();
	out->fd = Rast_open_new(output_name, DCELL_TYPE);

	/* get title, initialize the category and stat info */
	if (parm.title->answer)
	    strcpy(out->title, parm.title->answer);
	else
	    sprintf(out->title, "%dx%d neighborhood: %s of %s",
		    ncb.nsize, ncb.nsize, menu[method].name, ncb.oldcell);
    }

    /* copy color table? */
    if (copycolr) {
	G_suppress_warnings(1);
	copycolr =
	    (Rast_read_colors(ncb.oldcell, "", &colr) > 0);
	G_suppress_warnings(0);
    }

    /* allocate the cell buffers */
    allocate_bufs();

    /* initialize the cell bufs with 'dist' rows of the old cellfile */
    readrow = 0;
    for (row = 0; row < ncb.dist; row++)
	readcell(in_fd, readrow++, nrows, ncols);

    /* open the selection raster map */
    if (parm.selection->answer) {
	G_message(_("Opening selection map <%s>"), parm.selection->answer);
	selection_fd = Rast_open_old(parm.selection->answer, "");
        selection = Rast_allocate_null_buf();
    } else {
        selection_fd = -1;
        selection = NULL;
    }

    if (flag.circle->answer)
	circle_mask();

    values_w = NULL;
    values_w_tmp = NULL;
    if (weights) {
	values_w =
	    (DCELL(*)[2]) G_malloc(ncb.nsize * ncb.nsize * 2 * sizeof(DCELL));
	values_w_tmp =
	    (DCELL(*)[2]) G_malloc(ncb.nsize * ncb.nsize * 2 * sizeof(DCELL));
    }
    values = (DCELL *) G_malloc(ncb.nsize * ncb.nsize * sizeof(DCELL));
    values_tmp = (DCELL *) G_malloc(ncb.nsize * ncb.nsize * sizeof(DCELL));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	readcell(in_fd, readrow++, nrows, ncols);

	if (selection)
            Rast_get_null_value_row(selection_fd, selection, row);

	for (col = 0; col < ncols; col++) {

            if (selection && selection[col]) {
                /* ncb.buf length is region row length + 2 * ncb.dist (eq. floor(neighborhood/2))
                 * Thus original data start is shifted by ncb.dist! */
		for (i = 0; i < num_outputs; i++)
		    outputs[i].buf[col] = ncb.buf[ncb.dist][col + ncb.dist];
		continue;
	    }

	    if (weights)
		n = gather_w(values, values_w, col);
	    else
		n = gather(values, col);

	    for (i = 0; i < num_outputs; i++) {
		struct output *out = &outputs[i];
		DCELL *rp = &out->buf[col];

		if (n == 0) {
		    Rast_set_d_null_value(rp, 1);
		}
		else {
		    if (out->method_fn_w) {
			memcpy(values_w_tmp, values_w, n * 2 * sizeof(DCELL));
			(*out->method_fn_w)(rp, values_w_tmp, n, &out->quantile);
		    }
		    else {
			memcpy(values_tmp, values, n * sizeof(DCELL));
			(*out->method_fn)(rp, values_tmp, n, &out->quantile);
		    }

		    if (out->half && !Rast_is_d_null_value(rp))
			*rp += 0.5;
		}
	    }
	}

	for (i = 0; i < num_outputs; i++) {
	    struct output *out = &outputs[i];

	    Rast_put_d_row(out->fd, out->buf);
	}
    }
    G_percent(row, nrows, 2);

    Rast_close(in_fd);

    if (selection)
        Rast_close(selection_fd);

    for (i = 0; i < num_outputs; i++) {
	Rast_close(outputs[i].fd);

	/* put out category info */
	null_cats(outputs[i].title);
	if (outputs[i].cat_names)
	    outputs[i].cat_names();

	Rast_write_cats(outputs[i].name, &ncb.cats);

	if (copycolr && outputs[i].copycolr)
	    Rast_write_colors(outputs[i].name, G_mapset(), &colr);

	Rast_short_history(outputs[i].name, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(outputs[i].name, &history);
    }

    exit(EXIT_SUCCESS);
}

