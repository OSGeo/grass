
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

int main(int argc, char *argv[])
{
    char *p;
    int method;
    int in_fd;
    int selection_fd;
    int out_fd;
    DCELL *result;
    char *selection;
    RASTER_MAP_TYPE map_type;
    int row, col;
    int readrow;
    int nrows, ncols;
    int n;
    int copycolr;
    int half;
    stat_func *newvalue;
    stat_func_w *newvalue_w;
    ifunc cat_names;
    double quantile;
    const void *closure;
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

    DCELL(*values_w)[2];	/* list of neighborhood values and weights */

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("aggregation"));
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
    ncb.newcell = parm.output->answer;

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

    /* get the method */
    for (method = 0; (p = menu[method].name); method++)
	if ((strcmp(p, parm.method->answer) == 0))
	    break;
    if (!p) {
	G_warning(_("<%s=%s> unknown %s"),
		  parm.method->key, parm.method->answer, parm.method->key);
	G_usage();
	exit(EXIT_FAILURE);
    }

    if (menu[method].method == c_quant) {
	quantile = atoi(parm.quantile->answer);
	closure = &quantile;
    }

    half = (map_type == CELL_TYPE) ? menu[method].half : 0;

    /* establish the newvalue routine */
    newvalue = menu[method].method;
    newvalue_w = menu[method].method_w;

    /* copy color table? */
    copycolr = menu[method].copycolr;
    if (copycolr) {
	G_suppress_warnings(1);
	copycolr =
	    (Rast_read_colors(ncb.oldcell, "", &colr) > 0);
	G_suppress_warnings(0);
    }

    /* read the weights */
    if (parm.weight->answer) {
	read_weights(parm.weight->answer);
	if (!newvalue_w)
	    weights_mask();
    }
    else if (parm.gauss->answer) {
	if (!newvalue_w)
	    G_fatal_error(_("Method %s not compatible with Gaussian filter"), parm.method->answer);
	gaussian_weights(atof(parm.gauss->answer));
    }
    else
	newvalue_w = NULL;

    /* allocate the cell buffers */
    allocate_bufs();
    result = Rast_allocate_d_buf();

    /* get title, initialize the category and stat info */
    if (parm.title->answer)
	strcpy(ncb.title, parm.title->answer);
    else
	sprintf(ncb.title, "%dx%d neighborhood: %s of %s",
		ncb.nsize, ncb.nsize, menu[method].name, ncb.oldcell);


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

    /*open the new raster map */
    out_fd = Rast_open_new(ncb.newcell, map_type);

    if (flag.circle->answer)
	circle_mask();

    if (newvalue_w)
	values_w =
	    (DCELL(*)[2]) G_malloc(ncb.nsize * ncb.nsize * 2 * sizeof(DCELL));
    else
	values = (DCELL *) G_malloc(ncb.nsize * ncb.nsize * sizeof(DCELL));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	readcell(in_fd, readrow++, nrows, ncols);

	if (selection)
            Rast_get_null_value_row(selection_fd, selection, row);

	for (col = 0; col < ncols; col++) {
	    DCELL *rp = &result[col];

            if (selection && selection[col]) {
                /* ncb.buf length is region row length + 2 * ncb.dist (eq. floor(neighborhood/2))
                 * Thus original data start is shifted by ncb.dist! */
		*rp = ncb.buf[ncb.dist][col+ncb.dist];
		continue;
	    }

	    if (newvalue_w)
		n = gather_w(values_w, col);
	    else
		n = gather(values, col);

	    if (n < 0)
		Rast_set_d_null_value(rp, 1);
	    else {
		if (newvalue_w)
		    newvalue_w(rp, values_w, n, closure);
		else
		    newvalue(rp, values, n, closure);

		if (half && !Rast_is_d_null_value(rp))
		    *rp += 0.5;
	    }
	}

	Rast_put_d_row(out_fd, result);
    }
    G_percent(row, nrows, 2);

    Rast_close(out_fd);
    Rast_close(in_fd);

    if (selection)
        Rast_close(selection_fd);

    /* put out category info */
    null_cats();
    if ((cat_names = menu[method].cat_names))
	cat_names();

    Rast_write_cats(ncb.newcell, &ncb.cats);

    if (copycolr)
	Rast_write_colors(ncb.newcell, G_mapset(), &colr);

    Rast_short_history(ncb.newcell, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(ncb.newcell, &history);


    exit(EXIT_SUCCESS);
}

