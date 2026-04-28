/****************************************************************************
 *
 * MODULE:       r.mapcalc
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               rewritten 2002: Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <grass/glocale.h>

#include "mapcalc.h"

/****************************************************************************/

int overwrite_flag;

long seed_value;
long seeded;
int region_approach;

/****************************************************************************/

static expr_list *result;

/****************************************************************************/

static expr_list *parse_file(const char *filename)
{
    expr_list *res;
    FILE *fp;

    if (strcmp(filename, "-") == 0)
        return parse_stream(stdin);

    fp = fopen(filename, "r");
    if (!fp)
        G_fatal_error(_("Unable to open input file <%s>"), filename);

    res = parse_stream(fp);

    fclose(fp);

    return res;
}

static bool has_rand_expr(const expression *e)
{
    if (!e)
        return 0;

    switch (e->type) {
    case expr_type_function:
        if (strcmp(e->data.func.name, "rand") == 0)
            return 1;
        // args is 1-indexed (likely from yacc parser conventions)
        for (int i = 1; i <= e->data.func.argc; i++) {
            if (has_rand_expr(e->data.func.args[i]))
                return 1;
        }
        return 0;

    case expr_type_binding:
        return has_rand_expr(e->data.bind.val);

    case expr_type_variable:
        return has_rand_expr(e->data.var.bind);

    default:
        return 0;
    }
}

static bool expr_list_has_rand(const expr_list *list)
{
    for (; list; list = list->next) {
        if (has_rand_expr(list->exp))
            return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *expr, *file, *seed, *region, *nprocs;
    struct Flag *random, *describe;
    int all_ok;
    char *desc;
    int threads = 1;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    module->description = _("Raster map calculator.");
    module->overwrite = 1;

    expr = G_define_option();
    expr->key = "expression";
    expr->type = TYPE_STRING;
    expr->required = NO;
    expr->description = _("Expression to evaluate");
    expr->guisection = _("Expression");

    region = G_define_option();
    region->key = "region";
    region->type = TYPE_STRING;
    region->required = NO;
    region->answer = "current";
    region->options = "current,intersect,union";
    region->description = _("The computational region that should be used.");
    desc = NULL;
    G_asprintf(&desc,
               "current;%s;"
               "intersect;%s;"
               "union;%s;",
               _("current uses the current region of the mapset"),
               _("intersect computes the intersection region between "
                 "all input maps and uses the smallest resolution"),
               _("union computes the union extent of all map regions "
                 "and uses the smallest resolution"));
    region->descriptions = desc;

    file = G_define_standard_option(G_OPT_F_INPUT);
    file->key = "file";
    file->required = NO;
    file->description = _("File containing expression(s) to evaluate");
    file->guisection = _("Expression");

    seed = G_define_standard_option(G_OPT_M_SEED);

    random = G_define_flag();
    random->key = 's';
    random->label =
        _("Generate random seed (result is non-deterministic) [deprecated]");
    random->description =
        _("This flag is deprecated and will be removed in a future release. "
          "Seeding is automatic or use parameter seed.");

    describe = G_define_flag();
    describe->key = 'l';
    describe->description = _("List input and output maps");

    nprocs = G_define_standard_option(G_OPT_M_NPROCS);

    char **p = G_malloc(3 * sizeof(char *));
    if (argc == 1) {
        p[0] = argv[0];
        p[1] = G_store("file=-");
        p[2] = NULL;
        argv = p;
        argc = 2;
    }

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    overwrite_flag = module->overwrite;

    if (expr->answer && file->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"), expr->key,
                      file->key);

    if (seed->answer && random->answer)
        G_fatal_error(_("%s= and -%c are mutually exclusive"), seed->key,
                      random->key);

    if (expr->answer)
        result = parse_string(expr->answer);
    else if (file->answer)
        result = parse_file(file->answer);
    else
        result = parse_stream(stdin);

    if (!result)
        G_fatal_error(_("parse error"));

    bool has_rand = expr_list_has_rand(result);
    if (seed->answer) {
        seed_value = atol(seed->answer);
        G_srand48(seed_value);
        seeded = 1;
        G_debug(3, "Read random seed from seed=: %ld", seed_value);
    }
    else {
        if (has_rand) {
            seed_value = G_srand48_auto();
            seeded = 1;
            G_debug(3, "Automatically generated random seed: %ld", seed_value);
        }
        if (random->answer) {
            G_verbose_message(_("Flag 's' is deprecated and will be removed in "
                                "a future release. "
                                "Seeding is automatic or use parameter seed."));
        }
    }

    /* Set the global variable of the region setup approach */
    region_approach = 1;

    if (G_strncasecmp(region->answer, "union", 5) == 0)
        region_approach = 2;

    if (G_strncasecmp(region->answer, "intersect", 9) == 0)
        region_approach = 3;

    G_debug(1, "Region answer %s region approach %i", region->answer,
            region_approach);

    if (describe->answer) {
        describe_maps(stdout, result);
        return EXIT_SUCCESS;
    }

    pre_exec();

    /* Determine the number of threads */
    threads = atoi(nprocs->answer);

    /* Check if the program name is r3.mapcalc */
    /* Handle both Unix and Windows path separators */
    const char *progname = strrchr(argv[0], '/');
    if (!progname)
        progname = strrchr(argv[0], '\\');
    progname = progname ? progname + 1 : argv[0];

    if ((strncmp(progname, "r3.mapcalc", 10) == 0) && (threads != 1)) {
        threads = 1;
        nprocs->answer = "1";
        G_verbose_message(_("r3.mapcalc does not support parallel execution."));
    }
    else if ((threads != 1) && (has_rand)) {
        threads = 1;
        nprocs->answer = "1";
        G_verbose_message(
            _("Parallel execution is not supported with rand() function"));
    }

    /* Ensure the proper number of threads is assigned */
    threads = G_set_omp_num_threads(nprocs);
    if (threads > 1)
        threads = Rast_disable_omp_on_mask(threads);
    if (threads < 1)
        G_fatal_error(_("<%d> is not valid number of nprocs."), threads);

    /* Execute calculations */
    execute(result);
    post_exec();

    all_ok = 1;

    G_free(p);
    p = NULL;

    if (floating_point_exception_occurred) {
        G_warning(_("Floating point error(s) occurred in the calculation"));
        all_ok = 0;
    }

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/****************************************************************************/
