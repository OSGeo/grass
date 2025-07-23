#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "mapcalc.h"
#include "globals.h"
#include "func_proto.h"

/****************************************************************************/

int current_depth, current_row;
int depths, rows, columns;

/* Local variables for map management */
static expression **map_list = NULL;
static int num_maps = 0;
static int max_maps = 0;

/****************************************************************************/

static void extract_maps(expression *e);
static void initialize(expression *e);
static void evaluate(expression *e);
static int append_map(expression *e);

/****************************************************************************/

int append_map(expression *e)
{
    /* \brief Append a map to the global map list and reallocate if necessary
     */
    if (num_maps >= max_maps) {
        max_maps += 10;
        map_list = G_realloc(map_list, max_maps * sizeof(struct expression *));
    }

    map_list[num_maps] = e;

    return num_maps++;
}

void extract_maps(expression *e)
{
    /* \brief Search for map names in the expression and add them to the global
     * map list */
    int i;

    switch (e->type) {
    case expr_type_map:
        G_debug(1, "Found map %s", e->data.map.name);
        append_map(e);
        break;
    case expr_type_function:
        for (i = 1; i <= e->data.func.argc; i++) {
            extract_maps(e->data.func.args[i]);
        }
        break;
    case expr_type_binding:
        extract_maps(e->data.bind.val);
        break;
    }
}

/****************************************************************************/

static void allocate_buf(expression *e)
{
    e->buf = G_malloc(columns * Rast_cell_size(e->res_type));
}

static void set_buf(expression *e, void *buf)
{
    e->buf = buf;
}

/****************************************************************************/

static void initialize_constant(expression *e)
{
    allocate_buf(e);
}

static void initialize_variable(expression *e)
{
    set_buf(e, e->data.var.bind->data.bind.val->buf);
}

static void initialize_map(expression *e)
{
    allocate_buf(e);

    e->data.map.idx = open_map(e->data.map.name, e->data.map.mod,
                               e->data.map.row, e->data.map.col);
}

static void initialize_function(expression *e)
{
    int i;

    allocate_buf(e);

    e->data.func.argv = G_malloc((e->data.func.argc + 1) * sizeof(void *));
    e->data.func.argv[0] = e->buf;

    for (i = 1; i <= e->data.func.argc; i++) {
        initialize(e->data.func.args[i]);
        e->data.func.argv[i] = e->data.func.args[i]->buf;
    }
}

static void initialize_binding(expression *e)
{
    initialize(e->data.bind.val);
    set_buf(e, e->data.bind.val->buf);
}

static void initialize(expression *e)
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

static void do_evaluate(void *p)
{
    evaluate((struct expression *)p);
}

static void begin_evaluate(struct expression *e)
{
    G_begin_execute(do_evaluate, e, &e->worker, 0);
}

static void end_evaluate(struct expression *e)
{
    G_end_execute(&e->worker);
}

/****************************************************************************/

static void evaluate_constant(expression *e)
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

static void evaluate_variable(expression *e UNUSED)
{
    /* this is a no-op */
}

static void evaluate_map(expression *e)
{
    get_map_row(
        e->data.map.idx, e->data.map.mod, current_depth + e->data.map.depth,
        current_row + e->data.map.row, e->data.map.col, e->buf, e->res_type);
}

static void evaluate_function(expression *e)
{
    int i;
    int res;

    if (e->data.func.argc > 1 && e->data.func.func != f_eval) {
        for (i = 1; i <= e->data.func.argc; i++)
            begin_evaluate(e->data.func.args[i]);

        for (i = 1; i <= e->data.func.argc; i++)
            end_evaluate(e->data.func.args[i]);
    }
    else
        for (i = 1; i <= e->data.func.argc; i++)
            evaluate(e->data.func.args[i]);

    res = (*e->data.func.func)(e->data.func.argc, e->data.func.argt,
                               e->data.func.argv);

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
        G_fatal_error(_("Unknown error for function '%s'"), e->data.func.name);
        break;
    }
}

static void evaluate_binding(expression *e)
{
    evaluate(e->data.bind.val);
}

/****************************************************************************/

static void evaluate(expression *e)
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

static void error_handler(void *p UNUSED)
{
    expr_list *l;

    for (l = exprs; l; l = l->next) {
        expression *e = l->exp;
        int fd = e->data.bind.fd;

        if (fd >= 0)
            unopen_output_map(fd);
    }
}

void execute(expr_list *ee)
{
    int verbose = isatty(2);
    expr_list *l;
    int count, n;

    exprs = ee;
    G_add_error_handler(error_handler, NULL);

    for (l = ee; l; l = l->next) {
        expression *e = l->exp;
        const char *var;

        if (e->type != expr_type_binding && e->type != expr_type_function)
            G_fatal_error("internal error: execute: invalid type: %d", e->type);

        if (e->type != expr_type_binding)
            continue;

        var = e->data.bind.var;

        if (!overwrite_flag && check_output_map(var))
            G_fatal_error(_("output map <%s> exists. To overwrite, "
                            "use the --overwrite flag"),
                          var);
    }

    /* Parse each expression and extract all raster maps */
    for (l = ee; l; l = l->next) {
        expression *e = l->exp;

        extract_maps(e);
    }

    /* Set the region from the input maps */
    if (region_approach == 2)
        prepare_region_from_maps_union(map_list, num_maps);
    if (region_approach == 3)
        prepare_region_from_maps_intersect(map_list, num_maps);

    setup_region();

    /* Parse each expression and initialize the maps, buffers and variables */
    for (l = ee; l; l = l->next) {
        expression *e = l->exp;
        const char *var;
        expression *val;

        initialize(e);

        if (e->type != expr_type_binding)
            continue;

        var = e->data.bind.var;
        val = e->data.bind.val;
        e->data.bind.fd = open_output_map(var, val->res_type);
    }

    setup_maps();

    count = rows * depths;
    n = 0;

    G_init_workers();

    for (current_depth = 0; current_depth < depths; current_depth++) {
        for (current_row = 0; current_row < rows; current_row++) {
            if (verbose)
                G_percent(n, count, 2);

            for (l = ee; l; l = l->next) {
                expression *e = l->exp;
                int fd;

                evaluate(e);

                if (e->type != expr_type_binding)
                    continue;

                fd = e->data.bind.fd;
                put_map_row(fd, e->buf, e->res_type);
            }

            n++;
        }
    }

    G_finish_workers();

    if (verbose)
        G_percent(n, count, 2);

    close_maps();

    for (l = ee; l; l = l->next) {
        expression *e = l->exp;
        const char *var;
        expression *val;
        int fd;

        if (e->type != expr_type_binding)
            continue;

        var = e->data.bind.var;
        val = e->data.bind.val;
        fd = e->data.bind.fd;

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

void describe_maps(FILE *fp, expr_list *ee)
{
    expr_list *l;

    fprintf(fp, "output=");

    for (l = ee; l; l = l->next) {
        expression *e = l->exp;
        const char *var;

        if (e->type != expr_type_binding && e->type != expr_type_function)
            G_fatal_error("internal error: execute: invalid type: %d", e->type);

        initialize(e);

        if (e->type != expr_type_binding)
            continue;

        var = e->data.bind.var;
        fprintf(fp, "%s%s", l != ee ? "," : "", var);
    }

    fprintf(fp, "\n");

    fprintf(fp, "input=");
    list_maps(fp, ",");
    fprintf(fp, "\n");
}

/****************************************************************************/
