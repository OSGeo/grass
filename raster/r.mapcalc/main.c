/****************************************************************************
 *
 * MODULE:       r.mapcalc
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               rewritten 2002: Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:
 * SPDX-FileCopyrightText: 1999-2007 GRASS Development Team
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/glocale.h>

#include "mapcalc.h"

/****************************************************************************/

int overwrite_flag;

int64_t seed_value;
long seeded;
int rand_calls;
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

/* Count the rand() calls, each of which is evaluated once per row. A
 * variable is not followed, since its binding is evaluated where it
 * is defined, not where it is used. */
static int count_rand_calls(const expression *e)
{
    int count = 0;

    if (!e)
        return 0;

    switch (e->type) {
    case expr_type_function:
        if (strcmp(e->data.func.name, "rand") == 0)
            count++;
        // args is 1-indexed (likely from yacc parser conventions)
        for (int i = 1; i <= e->data.func.argc; i++)
            count += count_rand_calls(e->data.func.args[i]);
        return count;

    case expr_type_binding:
        return count_rand_calls(e->data.bind.val);

    default:
        return 0;
    }
}

static int expr_list_count_rand_calls(const expr_list *list)
{
    int count = 0;

    for (; list; list = list->next)
        count += count_rand_calls(list->exp);
    return count;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *expr, *file, *seed, *region, *nprocs;
    struct Flag *random, *describe;
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

    rand_calls = expr_list_count_rand_calls(result);
    if (seed->answer) {
        struct G_random_state check;
        char *end;

        errno = 0;
        seed_value = strtoll(seed->answer, &end, 10);
        if (errno || end == seed->answer || *end)
            G_fatal_error(_("Invalid random seed <%s>"), seed->answer);
        /* Refuse a seed out of range now, before the output maps are
         * created. */
        G_random_state_from_seed(&check, seed_value);
        seeded = 1;
        G_debug(3, "Read random seed from seed=: %" PRId64, seed_value);
    }
    else {
        if (rand_calls > 0) {
            seed_value = G_random_generate_seed();
            seeded = 1;
            G_debug(3, "Automatically generated random seed: %" PRId64,
                    seed_value);
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

    /* Ensure the proper number of threads is assigned */
    threads = G_set_omp_num_threads(nprocs);
    if (threads > 1)
        threads = Rast_disable_omp_on_mask(threads);
    if (threads < 1)
        G_fatal_error(_("<%d> is not valid number of nprocs."), threads);

    /* Execute calculations */
    execute(result);

    G_free(p);
    p = NULL;

    return EXIT_SUCCESS;
}

/****************************************************************************/
