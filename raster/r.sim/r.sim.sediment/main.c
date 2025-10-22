/****************************************************************************
 *
 * MODULE:       r.sim.sediment: main program for sediment transport
 *               simulation (SIMWE)
 *
 * AUTHOR(S):    L. Mitas,  H. Mitasova, J. Hofierka
 * PURPOSE:      Sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*-
 * r.sim.sediment: main program for hydrologic and sediment transport
 * simulation (SIMWE)
 *
 * Original program (2002) and various modifications:
 * Lubos Mitas, Helena Mitasova
 *
 * GRASS5.0 version of the program:
 * J. Hofierka
 *
 * Copyright (C) 2002 L. Mitas,  H. Mitasova, J. Hofierka
 *
 *This program is free software; you can redistribute it and/or
 *modify it under the terms of the GNU General Public License
 *as published by the Free Software Foundation; either version 2
 *of the License, or (at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *
 * Notes on modifications:
 * v. 1.0 June 2002
 *
 */

/********************************/
/* DEFINE GLOB VAR              */

/********************************/
#define DIFFC    "0.8"
#define NITER    "10"
#define ITEROUT  "2"
#define DENSITY  "200"
#define MANINVAL "0.1"

/********************************/
/* INCLUDES                     */

/********************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <grass/config.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/raster.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

/********************************/
/* Specific stuff               */

/********************************/

#include <grass/simlib.h>

char fncdsm[32];
char filnam[10];

/*struct BM *bitmask; */
/*struct Cell_head cellhd; */
struct GModule *module;
struct Map_info Map;

char msg[1024];

/****************************************/
/* MAIN                                 */

/****************************************/
int main(int argc, char *argv[])
{
    int threads;
    int ret_val;
    struct Cell_head cellhd;
    struct options parm;
    struct flags flag;
    long seed_value;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("soil"));
    G_add_keyword(_("sediment flow"));
    G_add_keyword(_("erosion"));
    G_add_keyword(_("deposition"));
    G_add_keyword(_("model"));
    G_add_keyword(_("parallel"));
    module->description =
        _("Sediment transport and erosion/deposition simulation "
          "using path sampling method (SIMWE).");

    parm.elevin = G_define_standard_option(G_OPT_R_ELEV);

    parm.wdepth = G_define_standard_option(G_OPT_R_INPUT);
    parm.wdepth->key = "water_depth";
    parm.wdepth->description = _("Name of water depth raster map [m]");

    parm.dxin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dxin->key = "dx";
    parm.dxin->description = _("Name of x-derivatives raster map [m/m]");

    parm.dyin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dyin->key = "dy";
    parm.dyin->description = _("Name of y-derivatives raster map [m/m]");

    parm.detin = G_define_standard_option(G_OPT_R_INPUT);
    parm.detin->key = "detachment_coeff";
    parm.detin->description =
        _("Name of detachment capacity coefficient raster map [s/m]");

    parm.tranin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tranin->key = "transport_coeff";
    parm.tranin->description =
        _("Name of transport capacity coefficient raster map [s]");

    parm.tauin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tauin->key = "shear_stress";
    parm.tauin->description =
        _("Name of critical shear stress raster map [Pa]");

    parm.manin = G_define_standard_option(G_OPT_R_INPUT);
    parm.manin->key = "man";
    parm.manin->required = NO;
    parm.manin->description = _("Name of Manning's n raster map");
    parm.manin->guisection = _("Input");

    parm.maninval = G_define_option();
    parm.maninval->key = "man_value";
    parm.maninval->type = TYPE_DOUBLE;
    parm.maninval->answer = MANINVAL;
    parm.maninval->required = NO;
    parm.maninval->description = _("Manning's n unique value");
    parm.maninval->guisection = _("Input");

    parm.observation = G_define_standard_option(G_OPT_V_INPUT);
    parm.observation->key = "observation";
    parm.observation->required = NO;
    parm.observation->label = _("Name of sampling locations vector points map");
    parm.observation->guisection = _("Input");

    parm.tc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tc->key = "transport_capacity";
    parm.tc->required = NO;
    parm.tc->description =
        _("Name for output transport capacity raster map [kg/ms]");
    parm.tc->guisection = _("Output");

    parm.et = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.et->key = "tlimit_erosion_deposition";
    parm.et->required = NO;
    parm.et->description = _("Name for output transport limited "
                             "erosion-deposition raster map [kg/m2s]");
    parm.et->guisection = _("Output");

    parm.conc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.conc->key = "sediment_concentration";
    parm.conc->required = NO;
    parm.conc->description =
        _("Name for output sediment concentration raster map [particle/m3]");
    parm.conc->guisection = _("Output");

    parm.flux = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.flux->key = "sediment_flux";
    parm.flux->required = NO;
    parm.flux->description =
        _("Name for output sediment flux raster map [kg/ms]");
    parm.flux->guisection = _("Output");

    parm.erdep = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.erdep->key = "erosion_deposition";
    parm.erdep->required = NO;
    parm.erdep->description =
        _("Name for output erosion-deposition raster map [kg/m2s]");
    parm.erdep->guisection = _("Output");

    parm.logfile = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.logfile->key = "logfile";
    parm.logfile->required = NO;
    parm.logfile->description =
        _("Name for sampling points output text file. For each observation "
          "vector point the time series of sediment transport is stored.");
    parm.logfile->guisection = _("Output");

    parm.outwalk = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.outwalk->key = "walkers_output";
    parm.outwalk->required = NO;
    parm.outwalk->description =
        _("Base name of the output walkers vector points map");
    parm.outwalk->guisection = _("Output");

    parm.nwalk = G_define_option();
    parm.nwalk->key = "nwalkers";
    parm.nwalk->type = TYPE_INTEGER;
    parm.nwalk->required = NO;
    parm.nwalk->description = _("Number of walkers");
    parm.nwalk->guisection = _("Parameters");

    parm.niter = G_define_option();
    parm.niter->key = "niterations";
    parm.niter->type = TYPE_INTEGER;
    parm.niter->answer = NITER;
    parm.niter->required = NO;
    parm.niter->description = _("Time used for iterations [minutes]");
    parm.niter->guisection = _("Parameters");

    parm.mintimestep = G_define_option();
    parm.mintimestep->key = "mintimestep";
    parm.mintimestep->type = TYPE_DOUBLE;
    parm.mintimestep->answer = "0.0";
    parm.mintimestep->required = NO;
    parm.mintimestep->label =
        _("Minimum time step for the simulation [seconds]");
    parm.mintimestep->description =
        _("A larger minimum time step substantially reduces processing time, "
          "but at the cost of accuracy");
    parm.mintimestep->guisection = _("Parameters");

    parm.outiter = G_define_option();
    parm.outiter->key = "output_step";
    parm.outiter->type = TYPE_INTEGER;
    parm.outiter->answer = ITEROUT;
    parm.outiter->required = NO;
    parm.outiter->description =
        _("Time interval for creating output maps [minutes]");
    parm.outiter->guisection = _("Parameters");

    /*
       parm.density = G_define_option();
       parm.density->key = "density";
       parm.density->type = TYPE_INTEGER;
       parm.density->answer = DENSITY;
       parm.density->required = NO;
       parm.density->description = _("Density of output walkers");
       parm.density->guisection = _("Parameters");
     */

    parm.diffc = G_define_option();
    parm.diffc->key = "diffusion_coeff";
    parm.diffc->type = TYPE_DOUBLE;
    parm.diffc->answer = DIFFC;
    parm.diffc->required = NO;
    parm.diffc->description = _("Water diffusion constant");
    parm.diffc->guisection = _("Parameters");

    parm.seed = G_define_option();
    parm.seed->key = "random_seed";
    parm.seed->type = TYPE_INTEGER;
    parm.seed->required = NO;
    parm.seed->label = _("Seed for random number generator");
    parm.seed->description =
        _("The same seed can be used to obtain same results"
          " or random seed can be generated by other means.");

    flag.generateSeed = G_define_flag();
    flag.generateSeed->key = 's';
    flag.generateSeed->label = _("Generate random seed");
    flag.generateSeed->description =
        _("Automatically generates random seed for random number"
          " generator (use when you don't want to provide the seed option)");

    parm.threads = G_define_option();
    parm.threads->key = "nprocs";
    parm.threads->type = TYPE_INTEGER;
    parm.threads->answer = NUM_THREADS;
    parm.threads->required = NO;
    parm.threads->description =
        _("Number of threads which will be used for parallel computation.");
    parm.threads->guisection = _("Parameters");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (flag.generateSeed->answer) {
        seed_value = G_srand48_auto();
        G_verbose_message(_("Generated random seed (-s): %ld"), seed_value);
    }
    else if (parm.seed->answer) {
        seed_value = atol(parm.seed->answer);
        G_srand48(seed_value);
        G_verbose_message(_("Read random seed from %s option: %ld"),
                          parm.seed->key, seed_value);
    }
    else {
        /* default as it used to be */
        G_srand48(12345);
    }

    G_get_set_window(&cellhd);

    Geometry geometry = {0};
    Settings settings = {0};
    Setup setup = {0};
    Simulation sim = {0};
    ObservationPoints points = {0};
    settings.hhmax = settings.halpha = settings.hbeta = 0;
    settings.ts = false;
    Inputs inputs = {0};
    Outputs outputs = {0};
    Grids grids = {0};

    geometry.conv = G_database_units_to_meters_factor();

    geometry.mixx = cellhd.west * geometry.conv;
    geometry.miyy = cellhd.south * geometry.conv;

    geometry.stepx = cellhd.ew_res * geometry.conv;
    geometry.stepy = cellhd.ns_res * geometry.conv;
    /*  geometry.step = amin1(geometry.stepx,geometry.stepy); */
    geometry.step = (geometry.stepx + geometry.stepy) / 2.;
    geometry.mx = cellhd.cols;
    geometry.my = cellhd.rows;
    geometry.xmin = 0.;
    geometry.ymin = 0.;
    geometry.xp0 = geometry.xmin + geometry.stepx / 2.;
    geometry.yp0 = geometry.ymin + geometry.stepy / 2.;
    geometry.xmax = geometry.xmin + geometry.stepx * (float)geometry.mx;
    geometry.ymax = geometry.ymin + geometry.stepy * (float)geometry.my;

    inputs.elevin = parm.elevin->answer;
    inputs.wdepth = parm.wdepth->answer;
    inputs.dxin = parm.dxin->answer;
    inputs.dyin = parm.dyin->answer;
    inputs.detin = parm.detin->answer;
    inputs.tranin = parm.tranin->answer;
    inputs.tauin = parm.tauin->answer;
    inputs.manin = parm.manin->answer;
    outputs.tc = parm.tc->answer;
    outputs.et = parm.et->answer;
    outputs.conc = parm.conc->answer;
    outputs.flux = parm.flux->answer;
    outputs.erdep = parm.erdep->answer;
    outputs.outwalk = parm.outwalk->answer;

    sscanf(parm.threads->answer, "%d", &threads);
    if (threads < 1) {
        G_warning(_("<%d> is not valid number of threads. Number of threads "
                    "will be set on <%d>"),
                  threads, abs(threads));
        threads = abs(threads);
    }
    if (threads > 1 && Rast_mask_is_present()) {
        G_warning(_("Parallel processing disabled due to active mask."));
        threads = 1;
    }
#if defined(_OPENMP)
    omp_set_num_threads(threads);
#else
    threads = 1;
#endif
    G_message(_("Number of threads: %d"), threads);

    /*      sscanf(parm.nwalk->answer, "%d", &wp.maxwa); */
    sscanf(parm.niter->answer, "%d", &settings.timesec);
    sscanf(parm.outiter->answer, "%d", &settings.iterout);
    sscanf(parm.mintimestep->answer, "%lf", &settings.mintimestep);
    /*    sscanf(parm.density->answer, "%d", &wp.ldemo); */
    sscanf(parm.diffc->answer, "%lf", &settings.frac);
    sscanf(parm.maninval->answer, "%lf", &inputs.manin_val);

    /* Recompute timesec from user input in minutes
     * to real timesec in seconds */
    settings.timesec = settings.timesec * 60;
    settings.iterout = settings.iterout * 60;
    if ((settings.timesec / settings.iterout) > 100)
        G_message(_("More than 100 files are going to be created !!!!!"));

    /* compute how big the raster is and set this to appr 2 walkers per cell */
    if (parm.nwalk->answer == NULL) {
        sim.maxwa = geometry.mx * geometry.my * 2;
        sim.rwalk = (double)(geometry.mx * geometry.my * 2.);
        G_message(_("default nwalk=%d, rwalk=%f"), sim.maxwa, sim.rwalk);
    }
    else {
        sscanf(parm.nwalk->answer, "%d", &sim.maxwa);
        sim.rwalk = (double)sim.maxwa;
    }
    /*rwalk = (double) maxwa; */

    if (geometry.conv != 1.0)
        G_message(_("Using metric conversion factor %f, step=%f"),
                  geometry.conv, geometry.step);

    points.observation = parm.observation->answer;
    points.logfile = parm.logfile->answer;
    create_observation_points(&points);

    if ((outputs.tc == NULL) && (outputs.et == NULL) &&
        (outputs.conc == NULL) && (outputs.flux == NULL) &&
        (outputs.erdep == NULL))
        G_warning(_("You are not outputting any raster or site files"));
    ret_val =
        input_data(geometry.my, geometry.mx, &sim, &inputs, &outputs, &grids);
    if (ret_val != 1)
        G_fatal_error(_("Input failed"));

    alloc_grids_sediment(&geometry, &outputs, &grids);

    grad_check(&setup, &geometry, &settings, &inputs, &outputs, &grids);
    init_grids_sediment(&setup, &geometry, &outputs, &grids);
    /* treba dat output pre topoerdep */
    main_loop(&setup, &geometry, &settings, &sim, &points, &inputs, &outputs,
              &grids);
    free_walkers(&sim, outputs.outwalk);

    /* Exit with Success */
    exit(EXIT_SUCCESS);
}
