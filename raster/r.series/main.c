
/****************************************************************************
 *
 * MODULE:       r.series
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Hamish Bowman <hamish_b yahoo.com>, Jachym Cepicky <jachym les-ejk.cz>,
 *               Martin Wegmann <wegmann biozentrum.uni-wuerzburg.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2002-2008 by the GRASS Development Team
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

struct menu
{
    stat_func *method;		/* routine to compute new value */
    stat_func_w *method_w;	/* routine to compute new value (weighted) */
    RASTER_MAP_TYPE outtype;	/* type of result */
    char *name;			/* method name */
    char *text;			/* menu display - full description */
} menu[] = {
    {c_ave,    w_ave,    DCELL_TYPE, "average",    "average value"},
    {c_count,  w_count,  CELL_TYPE,  "count",      "count of non-NULL cells"},
    {c_median, w_median, DCELL_TYPE, "median",     "median value"},
    {c_mode,   w_mode,   -1,         "mode",       "most frequently occurring value"},
    {c_min,    NULL,     -1,         "minimum",    "lowest value"},
    {c_minx,   NULL,     CELL_TYPE,  "min_raster", "raster with lowest value"},
    {c_max,    NULL,     -1,         "maximum",    "highest value"},
    {c_maxx,   NULL,     CELL_TYPE,  "max_raster", "raster with highest value"},
    {c_stddev, w_stddev, DCELL_TYPE, "stddev",     "standard deviation"},
    {c_range,  NULL,     -1,         "range",      "range of values"},
    {c_sum,    w_sum,    DCELL_TYPE, "sum",        "sum of values"},
    {c_var,    w_var,    DCELL_TYPE, "variance",   "statistical variance"},
    {c_divr,   NULL,     CELL_TYPE,  "diversity",  "number of different values"},
    {c_reg_m,  w_reg_m,  DCELL_TYPE, "slope",      "linear regression slope"},
    {c_reg_c,  w_reg_c,  DCELL_TYPE, "offset",     "linear regression offset"},
    {c_reg_r2, w_reg_r2, DCELL_TYPE, "detcoeff",   "linear regression coefficient of determination"},
    {c_reg_t,  w_reg_t,  DCELL_TYPE, "tvalue",     "linear regression t-value"},
    {c_quart1, w_quart1, DCELL_TYPE, "quart1",     "first quartile"},
    {c_quart3, w_quart3, DCELL_TYPE, "quart3",     "third quartile"},
    {c_perc90, w_perc90, DCELL_TYPE, "perc90",     "ninetieth percentile"},
    {c_quant,  w_quant,  DCELL_TYPE, "quantile",   "arbitrary quantile"},
    {c_skew,   w_skew,   DCELL_TYPE, "skewness",   "skewness"},
    {c_kurt,   w_kurt,   DCELL_TYPE, "kurtosis",   "kurtosis"},
    {NULL,     NULL,     0, NULL,         NULL}
};

struct input
{
    const char *name;
    int fd;
    DCELL *buf;
    DCELL weight;
};

struct output
{
    const char *name;
    int fd;
    DCELL *buf;
    stat_func *method_fn;
    stat_func_w *method_fn_w;
    double quantile;
};

static char *build_method_list(void)
{
    char *buf = G_malloc(1024);
    char *p = buf;
    int i;

    for (i = 0; menu[i].name; i++) {
	char *q;

	if (i)
	    *p++ = ',';
	for (q = menu[i].name; *q; p++, q++)
	    *p = *q;
    }
    *p = '\0';

    return buf;
}

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
    struct GModule *module;
    struct
    {
	struct Option *input, *file, *output, *method, *weights, *quantile, *range;
    } parm;
    struct
    {
	struct Flag *nulls, *lazy;
    } flag;
    int i;
    int num_inputs;
    struct input *inputs = NULL;
    int num_outputs;
    struct output *outputs = NULL;
    struct History history;
    DCELL *values = NULL, *values_tmp = NULL;
    DCELL(*values_w)[2];	/* list of values and weights */
    DCELL(*values_w_tmp)[2];	/* list of values and weights */
    int have_weights;
    int nrows, ncols;
    int row, col;
    double lo, hi;
    RASTER_MAP_TYPE intype, maptype;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("series"));
    module->description =
	_("Makes each output cell value a "
	  "function of the values assigned to the corresponding cells "
	  "in the input raster map layers.");

    parm.input = G_define_standard_option(G_OPT_R_INPUTS);
    parm.input->required = NO;

    parm.file = G_define_standard_option(G_OPT_F_INPUT);
    parm.file->key = "file";
    parm.file->description = _("Input file with one raster map name and optional one weight per line, field separator between name and weight is |");
    parm.file->required = NO;

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->multiple = YES;

    parm.method = G_define_option();
    parm.method->key = "method";
    parm.method->type = TYPE_STRING;
    parm.method->required = YES;
    parm.method->options = build_method_list();
    parm.method->description = _("Aggregate operation");
    parm.method->multiple = YES;

    parm.quantile = G_define_option();
    parm.quantile->key = "quantile";
    parm.quantile->type = TYPE_DOUBLE;
    parm.quantile->required = NO;
    parm.quantile->description = _("Quantile to calculate for method=quantile");
    parm.quantile->options = "0.0-1.0";
    parm.quantile->multiple = YES;

    parm.weights = G_define_option();
    parm.weights->key = "weights";
    parm.weights->type = TYPE_DOUBLE;
    parm.weights->required = NO;
    parm.weights->description = _("Weighting factor for each input map, default value is 1.0 for each input map");
    parm.weights->multiple = YES;
    
    parm.range = G_define_option();
    parm.range->key = "range";
    parm.range->type = TYPE_DOUBLE;
    parm.range->key_desc = "lo,hi";
    parm.range->description = _("Ignore values outside this range");

    flag.nulls = G_define_flag();
    flag.nulls->key = 'n';
    flag.nulls->description = _("Propagate NULLs");

    flag.lazy = G_define_flag();
    flag.lazy->key = 'z';
    flag.lazy->description = _("Do not keep files open");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    lo = -1.0 / 0.0; /* -inf */
    hi = 1.0 / 0.0; /* inf */
    if (parm.range->answer) {
	lo = atof(parm.range->answers[0]);
	hi = atof(parm.range->answers[1]);
    }
    
    if (parm.input->answer && parm.file->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.input->key, parm.file->key);
 
    if (!parm.input->answer && !parm.file->answer)
        G_fatal_error(_("Please specify %s= or %s="),
			parm.input->key, parm.file->key);

    have_weights = 0;

    intype = -1;

    /* process the input maps from the file */
    if (parm.file->answer) {
	FILE *in;
	int max_inputs;

	if (strcmp(parm.file->answer, "-") == 0)
	    in = stdin;
	else {
	    in = fopen(parm.file->answer, "r");
	    if (!in)
		G_fatal_error(_("Unable to open input file <%s>"), parm.file->answer);
	}
    
	num_inputs = 0;
	max_inputs = 0;

	for (;;) {
	    char buf[GNAME_MAX + 50]; /* Name and weight*/
            char tok_buf[GNAME_MAX + 50];
	    char *name;
            int ntokens;
            char **tokens;
	    struct input *p;
            double weight = 1.0;

	    if (!G_getl2(buf, sizeof(buf), in))
		break;

            strcpy(tok_buf, buf);
            tokens = G_tokenize(tok_buf, "|");
            ntokens = G_number_of_tokens(tokens);

	    name = G_chop(tokens[0]);
            if (ntokens > 1) {
	        weight = atof(G_chop(tokens[1]));

		if (weight < 0)
		    G_fatal_error(_("Weights must be positive"));

		if (weight != 1)
		    have_weights = 1;
            }

	    /* Ignore empty lines */
	    if (!*name)
		continue;

	    if (num_inputs >= max_inputs) {
		max_inputs += 100;
		inputs = G_realloc(inputs, max_inputs * sizeof(struct input));
	    }
	    p = &inputs[num_inputs++];

	    p->name = G_store(name);
            p->weight = weight;
	    G_verbose_message(_("Reading raster map <%s> using weight %f..."), p->name, p->weight);
	    p->fd = Rast_open_old(p->name, "");
	    if (p->fd < 0)
		G_fatal_error(_("Unable to open input raster <%s>"), p->name);
	    maptype = Rast_get_map_type(p->fd);
	    if (intype == -1)
		intype = maptype;
	    else {
		if (intype != maptype)
		    intype = DCELL_TYPE;
	    }
	    if (flag.lazy->answer)
		Rast_close(p->fd);
	    p->buf = Rast_allocate_d_buf();
	}

	if (num_inputs < 1)
	    G_fatal_error(_("No raster map name found in input file"));
        
	fclose(in);
    }
    else {
	int num_weights;

    	for (i = 0; parm.input->answers[i]; i++)
	    ;
    	num_inputs = i;

    	if (num_inputs < 1)
	    G_fatal_error(_("Raster map not found"));

        /* count weights */
	num_weights = 0;
        if (parm.weights->answers) {
            for (i = 0; parm.weights->answers[i]; i++)
                    ;
            num_weights = i;
        }
    
        if (num_weights && num_weights != num_inputs)
                G_fatal_error(_("input= and weights= must have the same number of values"));

    	inputs = G_malloc(num_inputs * sizeof(struct input));

    	for (i = 0; i < num_inputs; i++) {
	    struct input *p = &inputs[i];

	    p->name = parm.input->answers[i];
	    p->weight = 1.0;

            if (num_weights) {
                p->weight = (DCELL)atof(parm.weights->answers[i]);

		if (p->weight < 0)
		    G_fatal_error(_("Weights must be positive"));

		if (p->weight != 1)
		    have_weights = 1;
            }

	    G_verbose_message(_("Reading raster map <%s> using weight %f..."), p->name, p->weight);
	    p->fd = Rast_open_old(p->name, "");
	    if (p->fd < 0)
		G_fatal_error(_("Unable to open input raster <%s>"), p->name);
	    maptype = Rast_get_map_type(p->fd);
	    if (intype == -1)
		intype = maptype;
	    else {
		if (intype != maptype)
		    intype = DCELL_TYPE;
	    }
	    if (flag.lazy->answer)
		Rast_close(p->fd);
	    p->buf = Rast_allocate_d_buf();
    	}
    }

    /* process the output maps */
    for (i = 0; parm.output->answers[i]; i++)
	;
    num_outputs = i;

    for (i = 0; parm.method->answers[i]; i++)
	;
    if (num_outputs != i)
	G_fatal_error(_("output= and method= must have the same number of values"));

    outputs = G_calloc(num_outputs, sizeof(struct output));

    for (i = 0; i < num_outputs; i++) {
	struct output *out = &outputs[i];
	const char *output_name = parm.output->answers[i];
	const char *method_name = parm.method->answers[i];
	int method = find_method(method_name);

	out->name = output_name;

	if (have_weights) {
	    if (menu[method].method_w) {
		out->method_fn = NULL;
		out->method_fn_w = menu[method].method_w;
		/* special case mode: the result of a weighed mode 
		 * can be stored as type of input
		 * all other weighed versions: result as DCELL_TYPE */
		if (menu[method].outtype == CELL_TYPE)
		    menu[method].outtype = DCELL_TYPE;
	    }
	    else {
		G_warning(_("Method %s not compatible with weights, using unweighed version instead"),
			  method_name);

		out->method_fn = menu[method].method;
		out->method_fn_w = NULL;
	    }
	}
	else {
	    out->method_fn = menu[method].method;
	    out->method_fn_w = NULL;
	}

	out->quantile = (parm.quantile->answer && parm.quantile->answers[i])
	    ? atof(parm.quantile->answers[i])
	    : 0;
	out->buf = Rast_allocate_d_buf();
	if (menu[method].outtype == -1)
	    out->fd = Rast_open_new(output_name, intype);
	else
	    out->fd = Rast_open_new(output_name, menu[method].outtype);
    }

    /* initialise variables */
    values = G_malloc(num_inputs * sizeof(DCELL));
    values_tmp = G_malloc(num_inputs * sizeof(DCELL));
    values_w = NULL;
    values_w_tmp = NULL;
    if (have_weights) {
	values_w = (DCELL(*)[2]) G_malloc(num_inputs * 2 * sizeof(DCELL));
	values_w_tmp = (DCELL(*)[2]) G_malloc(num_inputs * 2 * sizeof(DCELL));
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* process the data */
    G_verbose_message(_("Percent complete..."));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	if (flag.lazy->answer) {
	    /* Open the files only on run time */
	    for (i = 0; i < num_inputs; i++) {
		inputs[i].fd = Rast_open_old(inputs[i].name, "");
		Rast_get_d_row(inputs[i].fd, inputs[i].buf, row);
		Rast_close(inputs[i].fd);
	    }
	}
	else {
	    for (i = 0; i < num_inputs; i++)
	        Rast_get_d_row(inputs[i].fd, inputs[i].buf, row);
	}

	for (col = 0; col < ncols; col++) {
	    int null = 0;

	    for (i = 0; i < num_inputs; i++) {
		DCELL v = inputs[i].buf[col];

		if (Rast_is_d_null_value(&v))
		    null = 1;
		else if (parm.range->answer && (v < lo || v > hi)) {
		    Rast_set_d_null_value(&v, 1);
		    null = 1;
		}
		values[i] = v;
		if (have_weights) {
		    values_w[i][0] = v;
		    values_w[i][1] = inputs[i].weight;
		}
	    }

	    for (i = 0; i < num_outputs; i++) {
		struct output *out = &outputs[i];

		if (null && flag.nulls->answer)
		    Rast_set_d_null_value(&out->buf[col], 1);
		else {
		    if (out->method_fn_w) {
			memcpy(values_w_tmp, values_w, num_inputs * 2 * sizeof(DCELL));
			(*out->method_fn_w)(&out->buf[col], values_w_tmp, num_inputs, &out->quantile);
		    }
		    else {
			memcpy(values_tmp, values, num_inputs * sizeof(DCELL));
			(*out->method_fn)(&out->buf[col], values_tmp, num_inputs, &out->quantile);
		    }
		}
	    }
	}

	for (i = 0; i < num_outputs; i++)
	    Rast_put_d_row(outputs[i].fd, outputs[i].buf);
    }

    G_percent(row, nrows, 2);

    /* close output maps */
    for (i = 0; i < num_outputs; i++) {
	struct output *out = &outputs[i];

	Rast_close(out->fd);

	Rast_short_history(out->name, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out->name, &history);
    }

    /* Close input maps */
    if (!flag.lazy->answer) {
    	for (i = 0; i < num_inputs; i++)
	    Rast_close(inputs[i].fd);
    }

    exit(EXIT_SUCCESS);
}
