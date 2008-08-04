
#include <stdlib.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "mapcalc.h"
#include "globals.h"

/****************************************************************************/

int current_depth, current_row;
int depths, rows, columns;

/****************************************************************************/

static void initialize(expression * e);
static void evaluate(expression * e);

/****************************************************************************/

static void allocate_buf(expression * e)
{
    e->buf = G_malloc(columns * G_raster_size(e->res_type));
}

static void set_buf(expression * e, void *buf)
{
    e->buf = buf;
}

/****************************************************************************/

static void initialize_constant(expression * e)
{
    allocate_buf(e);
}

static void initialize_variable(expression * e)
{
    set_buf(e, e->data.var.bind->data.bind.val->buf);
}

static void initialize_map(expression * e)
{
    allocate_buf(e);
    e->data.map.idx = open_map(e->data.map.name, e->data.map.mod,
			       e->data.map.row, e->data.map.col);
}

static void initialize_function(expression * e)
{
    int i;

    allocate_buf(e);

    e->data.func.argv[0] = e->buf;

    for (i = 1; i <= e->data.func.argc; i++) {
	initialize(e->data.func.args[i]);
	e->data.func.argv[i] = e->data.func.args[i]->buf;
    }
}

static void initialize_binding(expression * e)
{
    initialize(e->data.bind.val);
    set_buf(e, e->data.bind.val->buf);
}

static void initialize(expression * e)
{
    switch (e->type) {
    case expr_type_constant:
	initialize_constant(e);
	break;
    case expr_type_variable:
	initialize_variable(e);
	break;
    case expr_type_map:
	initialize_map(e);
	break;
    case expr_type_function:
	initialize_function(e);
	break;
    case expr_type_binding:
	initialize_binding(e);
	break;
    default:
	G_fatal_error(_("Unknown type: %d"), e->type);
    }
}

/****************************************************************************/

static void evaluate_constant(expression * e)
{
    int *ibuf = e->buf;
    float *fbuf = e->buf;
    double *dbuf = e->buf;
    int i;

    switch (e->res_type) {
    case CELL_TYPE:
	for (i = 0; i < columns; i++)
	    ibuf[i] = e->data.con.ival;
	break;

    case FCELL_TYPE:
	for (i = 0; i < columns; i++)
	    fbuf[i] = e->data.con.fval;
	break;

    case DCELL_TYPE:
	for (i = 0; i < columns; i++)
	    dbuf[i] = e->data.con.fval;
	break;
    default:
	G_fatal_error(_("Invalid type: %d"), e->res_type);
    }
}

static void evaluate_variable(expression * e)
{
    /* this is a no-op */
}

static void evaluate_map(expression * e)
{
    get_map_row(e->data.map.idx,
		e->data.map.mod,
		current_depth + e->data.map.depth,
		current_row + e->data.map.row,
		e->data.map.col, e->buf, e->res_type);
}

static void evaluate_function(expression * e)
{
    int i;
    int res;

    for (i = 1; i <= e->data.func.argc; i++)
	evaluate(e->data.func.args[i]);

    res = (*e->data.func.func) (e->data.func.argc,
				e->data.func.argt, e->data.func.argv);

    switch (res) {
    case E_ARG_LO:
	G_fatal_error(_("Too few arguments for function '%s'"),
		      e->data.func.name);
	break;
    case E_ARG_HI:
	G_fatal_error(_("Too many arguments for function '%s'"),
		      e->data.func.name);
	break;
    case E_ARG_TYPE:
	G_fatal_error(_("Invalid argument type for function '%s'"),
		      e->data.func.name);
	break;
    case E_RES_TYPE:
	G_fatal_error(_("Invalid return type for function '%s'"),
		      e->data.func.name);
	break;
    case E_INV_TYPE:
	G_fatal_error(_("Unknown type for function '%s'"), e->data.func.name);
	break;
    case E_ARG_NUM:
	G_fatal_error(_("Number of arguments for function '%s'"),
		      e->data.func.name);
	break;
    case E_WTF:
	G_fatal_error(_("Unknown error for function '%s'"),
		      e->data.func.name);
	break;
    }
}

static void evaluate_binding(expression * e)
{
    evaluate(e->data.bind.val);
}

/****************************************************************************/

static void evaluate(expression * e)
{
    switch (e->type) {
    case expr_type_constant:
	evaluate_constant(e);
	break;
    case expr_type_variable:
	evaluate_variable(e);
	break;
    case expr_type_map:
	evaluate_map(e);
	break;
    case expr_type_function:
	evaluate_function(e);
	break;
    case expr_type_binding:
	evaluate_binding(e);
	break;
    default:
	G_fatal_error(_("Unknown type: %d"), e->type);
    }
}

/****************************************************************************/

static expr_list *exprs;

/****************************************************************************/

static int error_handler(const char *msg, int fatal)
{
    expr_list *l;

    if (!fatal)
	return 0;

    for (l = exprs; l; l = l->next) {
	expression *e = l->exp;
	int fd = e->data.bind.fd;

	if (fd >= 0)
	    unopen_output_map(fd);
    }

    G_unset_error_routine();
    G_fatal_error("%s", msg);
    return 0;
}

static void setup_rand(void)
{
    /* Read PRNG seed from environment variable if available */
    /* GRASS_RND_SEED */
    const char *random_seed = getenv("GRASS_RND_SEED");
    long seed_value;

    if (!random_seed)
	return;

    seed_value = atol(random_seed);
    G_debug(3, "Read random seed from environment: %ld", seed_value);

#if defined(HAVE_DRAND48)
    srand48(seed_value);
#else
    srand((unsigned int)seed_value);
#endif
}

void execute(expr_list * ee)
{
    int verbose = isatty(2);
    expr_list *l;
    int count, n;

    setup_region();
    setup_maps();
    setup_rand();

    exprs = ee;
    G_set_error_routine(error_handler);

    for (l = ee; l; l = l->next) {
	expression *e = l->exp;
	const char *var;
	expression *val;

	if (e->type != expr_type_binding)
	    G_fatal_error("internal error: execute: invalid type: %d",
			  e->type);

	initialize(e);

	var = e->data.bind.var;
	val = e->data.bind.val;

	e->data.bind.fd = open_output_map(var, val->res_type);
    }

    count = rows * depths;
    n = 0;

    for (current_depth = 0; current_depth < depths; current_depth++)
	for (current_row = 0; current_row < rows; current_row++) {
	    if (verbose)
		G_percent(n, count, 2);

	    for (l = ee; l; l = l->next) {
		expression *e = l->exp;
		int fd = e->data.bind.fd;

		evaluate(e);
		put_map_row(fd, e->buf, e->res_type);
	    }

	    n++;
	}

    if (verbose)
	G_percent(n, count, 2);

    for (l = ee; l; l = l->next) {
	expression *e = l->exp;
	const char *var = e->data.bind.var;
	expression *val = e->data.bind.val;
	int fd = e->data.bind.fd;

	close_output_map(fd);
	e->data.bind.fd = -1;

	if (val->type == expr_type_map) {
	    if (val->data.map.mod == 'M') {
		copy_cats(var, val->data.map.idx);
		copy_colors(var, val->data.map.idx);
	    }

	    copy_history(var, val->data.map.idx);
	}
	else
	    create_history(var, val);
    }

    G_unset_error_routine();
}

/****************************************************************************/
