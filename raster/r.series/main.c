/****************************************************************************
 *
 * MODULE:       r.series
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Hamish Bowman <hamish_nospam yahoo.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Martin Wegmann <wegmann biozentrum.uni-wuerzburg.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
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
#include <grass/glocale.h>
#include <grass/stats.h>

struct menu
{
	stat_func *method;	/* routine to compute new value */
	int is_int;		/* result is an integer */
	char *name;		/* method name */
	char *text;		/* menu display - full description */
} menu[] = {
	{c_ave,    0, "average",    "average value"},
	{c_count,  1, "count",      "count of non-NULL cells"},
	{c_median, 0, "median",     "median value"},
	{c_mode,   0, "mode",       "most frequently occuring value"},
	{c_min,    0, "minimum",    "lowest value"},
	{c_minx,   1, "min_raster", "raster with lowest value"}, 
	{c_max,    0, "maximum",    "highest value"},
	{c_maxx,   1, "max_raster", "raster with highest value"}, 
	{c_stddev, 0, "stddev",     "standard deviation"},
	{c_range,  0, "range" ,     "range of values"},
	{c_sum,    0, "sum",        "sum of values"},
	{c_var,    0, "variance",   "statistical variance"},
	{c_divr,   1, "diversity",  "number of different values"},
	{c_reg_m,  0, "slope",      "linear regression slope"},
	{c_reg_c,  0, "offset",     "linear regression offset"},
	{c_reg_r2, 0, "detcoeff",   "linear regression coefficient of determination"},
	{c_quart1, 0, "quart1",     "first quartile"},
	{c_quart3, 0, "quart3",     "third quartile"},
	{c_perc90, 0, "perc90",     "ninetieth percentile"},
	{NULL,     0, NULL,         NULL}
};

struct input
{
	char *name, *mapset;
	int fd;
	DCELL *buf;
};

static char *build_method_list(void)
{
	char *buf = G_malloc(1024);
	char *p = buf;
	int i;

	for (i = 0; menu[i].name; i++)
	{
		char *q;
		if (i)
			*p++ = ',';
		for (q = menu[i].name; *q; p++, q++)
			*p = *q;
	}
	*p = '\0';

	return buf;
}

int main (int argc, char *argv[])
{
	struct GModule *module;
	struct {
		struct Option *input, *output, *method;
	} parm;
	struct {
    /* please, remove before GRASS 7 released */
		struct Flag *quiet;
		struct Flag *nulls;
	} flag;
	int method;
	stat_func *method_fn;
	int i;
	int num_inputs;
	struct input *inputs;
	char *out_name;
	int out_fd;
	struct History history;
	DCELL *out_buf;
	DCELL *values;
	int nrows, ncols;
	int row, col;

	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("raster, series");
	module->description =
	    _("Makes each output cell value a "
	      "function of the values assigned to the corresponding cells "
	      "in the input raster map layers.");

	parm.input = G_define_standard_option(G_OPT_R_INPUTS);

	parm.output = G_define_standard_option(G_OPT_R_OUTPUT);

	parm.method = G_define_option() ;
	parm.method->key        = "method" ;
	parm.method->type       = TYPE_STRING ;
	parm.method->required   = YES ;
	parm.method->options    = build_method_list();
	parm.method->description= _("Aggregate operation") ;

	/* please, remove before GRASS 7 released */
	flag.quiet = G_define_flag();
	flag.quiet->key = 'q';
	flag.quiet->description = _("Run quietly");

	flag.nulls = G_define_flag();
	flag.nulls->key = 'n';
	flag.nulls->description = _("Propagate NULLs");

	if (G_parser(argc,argv))
	    exit(EXIT_FAILURE);

        /* please, remove before GRASS 7 released */
        if(flag.quiet->answer) {
            putenv("GRASS_VERBOSE=0");
            G_warning(_("The '-q' flag is superseded and will be removed "
                "in future. Please use '--quiet' instead."));
        }

	/* get the method */
	method = -1;
	for (i = 0; menu[i].name; i++)
		if (strcmp(menu[i].name, parm.method->answer) == 0)
		{
			method = i;
			break;
		}
	if (method < 0)
		G_fatal_error(_("Unknown method <%s>"), parm.method->answer);

	method_fn = menu[method].method;

	/* process the input maps */
	for (i = 0; parm.input->answers[i]; i++)
	    ;
	num_inputs = i;

	if(num_inputs < 1)
	    G_fatal_error(_("Raster map not found"));

	inputs = G_malloc(num_inputs * sizeof(struct input));

	for (i = 0; i < num_inputs; i++)
	{
		struct input *p = &inputs[i];
		p->name = parm.input->answers[i];
		p->mapset = G_find_cell2(p->name,"");
		if (!p->mapset)
			G_fatal_error(_("Raster map <%s> not found"), p->name);
		else
		        G_message (_("Reading raster map <%s>..."), p->name);
		p->fd = G_open_cell_old(p->name, p->mapset);
		if (p->fd < 0)
			G_fatal_error(_("Unable to open raster map <%s> in mapset <%s>"),
				      p->name, p->mapset);
		p->buf = G_allocate_d_raster_buf();
	}

	/* process the output map */
	out_name = parm.output->answer;

	out_fd = G_open_raster_new(out_name, menu[method].is_int ? CELL_TYPE : DCELL_TYPE);
	if (out_fd < 0)
		G_fatal_error(_("Unable to create raster map <%s>"), out_name);

	out_buf = G_allocate_d_raster_buf();

	/* initialise variables */
	values = G_malloc(num_inputs * sizeof(DCELL));

	nrows = G_window_rows();
	ncols = G_window_cols();

	/* process the data */
        G_verbose_message (_("Percent complete..."));

	for (row = 0; row < nrows; row++)
	{
                G_percent(row, nrows, 2);

		for (i = 0; i < num_inputs; i++)
			G_get_d_raster_row (inputs[i].fd, inputs[i].buf, row);

		for (col = 0; col < ncols; col++)
		{
			int null = 0;

			for (i = 0; i < num_inputs; i++)
			{
				DCELL v = inputs[i].buf[col];

				if (G_is_d_null_value(&v))
					null = 1;

				values[i] = v;
			}

			if (null && flag.nulls->answer)
				G_set_d_null_value(&out_buf[col], 1);
			else
				(*method_fn)(&out_buf[col], values, num_inputs);
		}

		G_put_d_raster_row(out_fd, out_buf);
	}

        G_percent(row, nrows, 2);

	/* close maps */
	G_close_cell(out_fd);

	G_short_history(out_name, "raster", &history);
	G_command_history(&history);
	G_write_history(out_name, &history);

	for (i = 0; i < num_inputs; i++)
		G_close_cell(inputs[i].fd);

	exit(EXIT_SUCCESS);
}
