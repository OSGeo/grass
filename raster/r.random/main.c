/****************************************************************************
 *
 * MODULE:       r.random
 *
 * AUTHOR(S):    Dr. James Hinthorne - Central Wash. Uni. GIS Lab
 *               Modified for GRASS 5.x
 *                 Eric G. Miller
 *               Modified for GRASS 6.x and updates
 *                 Brad Douglas
 *               Cover map support
 *                 Markus Neteler
 *               Modified interface for GRASS 8.x
 *                 Vaclav Petras
 *
 * PURPOSE:      Generate a vector or raster map of random points
 *               selected from an input map
 *
 * COPYRIGHT:    (C) 2003-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

static int has_percent(char *);

int main(int argc, char *argv[])
{
    short percent;
    double percentage;
    gcell_count targets;
    gcell_count count;
    struct rr_state myState;
    long seed_value;

    struct GModule *module;
    struct {
        struct Option *input, *cover, *raster, *sites, *npoints, *seed;
    } parm;
    struct {
        struct Flag *gen_seed, *zero, *z_geometry, *notopol_flag;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("random"));
    G_add_keyword(_("level1"));

    module->label = _("Creates randomly placed raster cells or vector points");
    module->description = _("Creates a raster map and vector point map "
                            "containing randomly located cells and points.");

    parm.input = G_define_standard_option(G_OPT_R_INPUT);
    parm.input->description = _("Name of input raster map");
    parm.input->guisection = _("Input");

    parm.cover = G_define_standard_option(G_OPT_R_INPUT);
    parm.cover->key = "cover";
    parm.cover->required = NO;
    parm.cover->description = _("Name of cover raster map");
    parm.cover->guisection = _("Input");

    parm.npoints = G_define_option();
    parm.npoints->key = "npoints";
    parm.npoints->key_desc = "number[%]";
    parm.npoints->type = TYPE_STRING;
    parm.npoints->required = YES;
    parm.npoints->label = _("The number of points (or cells) to generate");
    parm.npoints->description =
        _("The number of vector points or raster cells to generate,"
          " possibly as a percentage of number of cells");
    parm.npoints->guisection = _("Output");

    parm.raster = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.raster->required = NO;
    parm.raster->key = "raster";
    parm.raster->guisection = _("Output");

    parm.sites = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.sites->required = NO;
    parm.sites->key = "vector";
    parm.sites->guisection = _("Output");

    parm.seed = G_define_standard_option(G_OPT_M_SEED);
    parm.seed->guisection = _("Input");

    flag.gen_seed = G_define_flag();
    flag.gen_seed->key = 's';
    flag.gen_seed->description =
        _("Generate random seed (result is non-deterministic)");
    flag.gen_seed->guisection = _("Input");

    flag.zero = G_define_flag();
    flag.zero->key = 'n';
    flag.zero->description = _("Generate points also for NULL cells");
    flag.zero->guisection = _("Output");

    flag.z_geometry = G_define_flag();
    flag.z_geometry->key = 'z';
    flag.z_geometry->label = _("Generate vector points as 3D points");
    flag.z_geometry->description =
        _("Input raster values will be used for Z coordinates");
    flag.z_geometry->guisection = _("Output");

    flag.notopol_flag = G_define_standard_flag(G_FLG_V_TOPO);
    flag.notopol_flag->description =
        _("Do not build topology for vector points");
    flag.notopol_flag->guisection = _("Output");

    /* Either explicit seed or explicitly generate seed, but not both. */
    G_option_exclusive(parm.seed, flag.gen_seed, NULL);
    G_option_required(parm.seed, flag.gen_seed, NULL);
    /* At least one of outputs is required. */
    G_option_required(parm.raster, parm.sites, NULL);

    if (G_parser(argc, argv) != 0)
        exit(EXIT_FAILURE);

    /* Set some state variables */
    myState.use_nulls = flag.zero->answer;
    myState.inraster = parm.input->answer;
    if (parm.cover->answer) {
        myState.docover = TRUE;
        myState.inrcover = parm.cover->answer;
    }
    else {
        myState.docover = FALSE;
        myState.inrcover = NULL;
    }
    myState.outraster = parm.raster->answer;
    myState.outvector = parm.sites->answer;
    myState.z_geometry = flag.z_geometry->answer;
    myState.notopol = flag.notopol_flag->answer;

    /* look for n[%] */
    percent = has_percent(parm.npoints->answer);
    if (percent) {
        if (sscanf(parm.npoints->answer, "%lf", &percentage) != 1 ||
            percentage <= 0.0 || percentage > 100.0) {
            G_fatal_error(_("<%s=%s> invalid percentage"), parm.npoints->key,
                          parm.npoints->answer);
        }
    }
    else {
#ifdef HAVE_LONG_LONG_INT
        if (sscanf(parm.npoints->answer, "%llu", &targets) != 1
#else
        if (sscanf(parm.npoints->answer, "%lu", &targets) != 1
#endif
            || targets <= 0) {
            G_fatal_error(_("<%s=%s> invalid number of points"),
                          parm.npoints->key, parm.npoints->answer);
        }
    }

    /* Compute stats only after we know parameters are OK, but before
       we need to check the provided number of points. */
    get_stats(&myState);

    count =
        (myState.use_nulls) ? myState.nCells : myState.nCells - myState.nNulls;

    if (percent)
        myState.nRand = (gcell_count)(count * percentage / 100.0 + .5);
    else {
        if (targets > count) {
#ifdef HAVE_LONG_LONG_INT
            if (myState.use_nulls)
                G_fatal_error(
                    _("There aren't [%llu] cells in the current region"),
                    targets);
            else
                G_fatal_error(_("There aren't [%llu] non-NULL cells in the "
                                "current region"),
                              targets);
#else
            if (myState.use_nulls)
                G_fatal_error(
                    _("There aren't [%lu] cells in the current region"),
                    targets);
            else
                G_fatal_error(_("There aren't [%lu] non-NULL cells in the "
                                "current region"),
                              targets);
#endif
        }

        if (targets <= 0)
            G_fatal_error(
                _("There are no valid locations in the current region"));

        myState.nRand = targets;
    }

    if (parm.seed->answer) {
        seed_value = atol(parm.seed->answer);
        G_srand48(seed_value);
        G_debug(3, "Read random seed from seed=: %ld", seed_value);
    }
    else {
        seed_value = G_srand48_auto();
        G_debug(3, "Generated random seed (-s): %ld", seed_value);
    }

    execute_random(&myState);

    if (myState.outraster)
        make_support(&myState, percent, percentage, seed_value);

    return EXIT_SUCCESS;
}

static int has_percent(char *s)
{
    while (*s)
        if (*s++ == '%')
            return 1;

    return 0;
}
