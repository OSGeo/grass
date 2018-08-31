
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/****************************************************************************/

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *expr, *file, *seed, *region;
    struct Flag *random, *describe;
    int all_ok;

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
    region->description = _("The computational region that should be used.\n"
                            "               - current uses the current region of the mapset.\n"
                            "               - intersect computes the intersection region between\n"
                            "                 all input maps and uses the smallest resolution\n"
                            "               - union computes the union extent of all map regions\n"
                            "                 and uses the smallest resolution");

    file = G_define_standard_option(G_OPT_F_INPUT);
    file->key = "file";
    file->required = NO;
    file->description = _("File containing expression(s) to evaluate");
    file->guisection = _("Expression");

    seed = G_define_option();
    seed->key = "seed";
    seed->type = TYPE_INTEGER;
    seed->required = NO;
    seed->description = _("Seed for rand() function");

    random = G_define_flag();
    random->key = 's';
    random->description = _("Generate random seed (result is non-deterministic)");

    describe = G_define_flag();
    describe->key = 'l';
    describe->description = _("List input and output maps");

    if (argc == 1)
    {
        char **p = G_malloc(3 * sizeof(char *));
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
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
                        expr->key, file->key);

    if (seed->answer && random->answer)
        G_fatal_error(_("%s= and -%c are mutually exclusive"),
                        seed->key, random->key);

    if (expr->answer)
        result = parse_string(expr->answer);
    else if (file->answer)
        result = parse_file(file->answer);
    else
        result = parse_stream(stdin);

    if (!result)
        G_fatal_error(_("parse error"));

    if (seed->answer) {
        seed_value = atol(seed->answer);
        G_srand48(seed_value);
        seeded = 1;
        G_debug(3, "Read random seed from seed=: %ld", seed_value);
    }

    if (random->answer) {
        seed_value = G_srand48_auto();
        seeded = 1;
        G_debug(3, "Generated random seed (-s): %ld", seed_value);
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
    execute(result);
    post_exec();

    all_ok = 1;

    if (floating_point_exception_occurred) {
        G_warning(_("Floating point error(s) occurred in the calculation"));
        all_ok = 0;
    }

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/****************************************************************************/
