
/****************************************************************************
 *
 * MODULE:       r.sim.water: main program for hydrologic and sediment transport
 *               simulation (SIMWE)
 *
 * AUTHOR(S):    L. Mitas,  H. Mitasova, J. Hofierka
 * PURPOSE:      Hydrologic and sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*-
 * r.sim.water: main program for hydrologic and sediment transport
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
 * v. 1.0 May 2002
 * modified by Y. Chemin in February 2008 (reporting, optional inputs)
 * sites-related input/output commented out Nov. 2008
 */

/********************************/
/* DEFINE GLOB VAR              */

/********************************/
/* #define NWALK        "1000000" */
#define DIFFC	"0.8"
#define HMAX	"0.3"
#define HALPHA	"4.0"
#define	HBETA	"0.5"
#define NITER   "10"
#define ITEROUT "2"
#define DENSITY "200"
#define RAINVAL "50"
#define MANINVAL "0.1"
#define INFILVAL "0.0"

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
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

/********************************/
/* Specific stuff               */

/********************************/

#include <grass/simlib.h>

/****************************************/
/* MAIN                                 */

/****************************************/
int main(int argc, char *argv[])
{
    int ii;
    int threads;
    int ret_val;
    double x_orig, y_orig;
    struct GModule *module;
    struct Cell_head cellhd;
    struct WaterParams wp;
    struct options parm;
    struct flags flag;
    long seed_value;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("soil"));
    G_add_keyword(_("flow"));
    G_add_keyword(_("overland flow"));
    G_add_keyword(_("model"));
    module->description =
	_("Overland flow hydrologic simulation using "
	  "path sampling method (SIMWE).");

    parm.elevin = G_define_standard_option(G_OPT_R_ELEV);
    
    parm.dxin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dxin->key = "dx";
    parm.dxin->description = _("Name of x-derivatives raster map [m/m]");

    parm.dyin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dyin->key = "dy";
    parm.dyin->description = _("Name of y-derivatives raster map [m/m]");

    parm.rain = G_define_standard_option(G_OPT_R_INPUT);
    parm.rain->key = "rain";
    parm.rain->required = NO;
    parm.rain->description =
	_("Name of rainfall excess rate (rain-infilt) raster map [mm/hr]");
    parm.rain->guisection = _("Input");
    
    parm.rainval = G_define_option();
    parm.rainval->key = "rain_value";
    parm.rainval->type = TYPE_DOUBLE;
    parm.rainval->answer = RAINVAL;
    parm.rainval->required = NO;
    parm.rainval->description =
	_("Rainfall excess rate unique value [mm/hr]");
    parm.rainval->guisection = _("Input");

    parm.infil = G_define_standard_option(G_OPT_R_INPUT);
    parm.infil->key = "infil";
    parm.infil->required = NO;
    parm.infil->description =
	_("Name of runoff infiltration rate raster map [mm/hr]");
    parm.infil->guisection = _("Input");

    parm.infilval = G_define_option();
    parm.infilval->key = "infil_value";
    parm.infilval->type = TYPE_DOUBLE;
    parm.infilval->answer = INFILVAL;
    parm.infilval->required = NO;
    parm.infilval->description =
	_("Runoff infiltration rate unique value [mm/hr]");
    parm.infilval->guisection = _("Input");

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

    parm.traps = G_define_standard_option(G_OPT_R_INPUT);
    parm.traps->key = "flow_control";
    parm.traps->required = NO;
    parm.traps->description =
	_("Name of flow controls raster map (permeability ratio 0-1)");
    parm.traps->guisection = _("Input");

    parm.observation = G_define_standard_option(G_OPT_V_INPUT);
    parm.observation->key = "observation";
    parm.observation->required = NO;
    parm.observation->label =
	_("Name of sampling locations vector points map");
    parm.observation->guisection = _("Input");

    parm.depth = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.depth->key = "depth";
    parm.depth->required = NO;
    parm.depth->description = _("Name for output water depth raster map [m]");
    parm.depth->guisection = _("Output");

    parm.disch = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.disch->key = "discharge";
    parm.disch->required = NO;
    parm.disch->description = _("Name for output water discharge raster map [m3/s]");
    parm.disch->guisection = _("Output");

    parm.err = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.err->key = "error";
    parm.err->required = NO;
    parm.err->description = _("Name for output simulation error raster map [m]");
    parm.err->guisection = _("Output");

    parm.outwalk = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.outwalk->key = "walkers_output";
    parm.outwalk->required = NO;
    parm.outwalk->label =
	_("Base name of the output walkers vector points map");
    parm.outwalk->guisection = _("Output");

    parm.logfile = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.logfile->key = "logfile";
    parm.logfile->required = NO;
    parm.logfile->description =
	_("Name for sampling points output text file. For each observation vector point the time series of sediment transport is stored.");
    parm.logfile->guisection = _("Output");

    parm.nwalk = G_define_option();
    parm.nwalk->key = "nwalkers";
    parm.nwalk->type = TYPE_INTEGER;
    parm.nwalk->required = NO;
    parm.nwalk->description =
	_("Number of walkers, default is twice the number of cells");
    parm.nwalk->guisection = _("Parameters");

    parm.niter = G_define_option();
    parm.niter->key = "niterations";
    parm.niter->type = TYPE_INTEGER;
    parm.niter->answer = NITER;
    parm.niter->required = NO;
    parm.niter->description = _("Time used for iterations [minutes]");
    parm.niter->guisection = _("Parameters");

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

    parm.hmax = G_define_option();
    parm.hmax->key = "hmax";
    parm.hmax->type = TYPE_DOUBLE;
    parm.hmax->answer = HMAX;
    parm.hmax->required = NO;
    parm.hmax->label =
	_("Threshold water depth [m]");
    parm.hmax->description = _("Diffusion increases after this water depth is reached");
    parm.hmax->guisection = _("Parameters");

    parm.halpha = G_define_option();
    parm.halpha->key = "halpha";
    parm.halpha->type = TYPE_DOUBLE;
    parm.halpha->answer = HALPHA;
    parm.halpha->required = NO;
    parm.halpha->description = _("Diffusion increase constant");
    parm.halpha->guisection = _("Parameters");

    parm.hbeta = G_define_option();
    parm.hbeta->key = "hbeta";
    parm.hbeta->type = TYPE_DOUBLE;
    parm.hbeta->answer = HBETA;
    parm.hbeta->required = NO;
    parm.hbeta->description =
	_("Weighting factor for water flow velocity vector");
    parm.hbeta->guisection = _("Parameters");

    flag.tserie = G_define_flag();
    flag.tserie->key = 't';
    flag.tserie->description = _("Time-series output");
    flag.tserie->guisection = _("Output");

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
    flag.generateSeed->label =
        _("Generate random seed");
    flag.generateSeed->description =
        _("Automatically generates random seed for random number"
          " generator (use when you don't want to provide the seed option)");

     parm.threads = G_define_option();
     parm.threads->key = "nprocs";
     parm.threads->type = TYPE_INTEGER;
     parm.threads->answer = NUM_THREADS;
     parm.threads->required = NO;
     parm.threads->description =
     _("Number of threads which will be used for parallel compute");
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

    WaterParams_init(&wp);

    wp.conv = G_database_units_to_meters_factor();

    G_debug(3, "Conversion factor is set to: %f", wp.conv);

    wp.mixx = wp.conv * cellhd.west;
    wp.maxx = wp.conv * cellhd.east;
    wp.miyy = wp.conv * cellhd.south;
    wp.mayy = wp.conv * cellhd.north;

    wp.stepx = cellhd.ew_res * wp.conv;
    wp.stepy = cellhd.ns_res * wp.conv;
    /*  step = amin1(stepx,stepy); */
    wp.step = (wp.stepx + wp.stepy) / 2.;
    wp.mx = cellhd.cols;
    wp.my = cellhd.rows;
    x_orig = cellhd.west * wp.conv;
    y_orig = cellhd.south * wp.conv;	/* do we need this? */
    wp.xmin = 0.;
    wp.ymin = 0.;
    wp.xp0 = wp.xmin + wp.stepx / 2.;
    wp.yp0 = wp.ymin + wp.stepy / 2.;
    wp.xmax = wp.xmin + wp.stepx * (float)wp.mx;
    wp.ymax = wp.ymin + wp.stepy * (float)wp.my;

    G_debug(3, "xmax: %f, ymax: %f", wp.xmax, wp.ymax);

    wp.ts = flag.tserie->answer;

    wp.elevin = parm.elevin->answer;
    wp.dxin = parm.dxin->answer;
    wp.dyin = parm.dyin->answer;
    wp.rain = parm.rain->answer;
    wp.infil = parm.infil->answer;
    wp.traps = parm.traps->answer;
    wp.manin = parm.manin->answer;
    wp.depth = parm.depth->answer;
    wp.disch = parm.disch->answer;
    wp.err = parm.err->answer;
    wp.outwalk = parm.outwalk->answer; 

    G_debug(3, "Parsing numeric parameters");

    sscanf(parm.niter->answer, "%d", &wp.timesec);
    sscanf(parm.outiter->answer, "%d", &wp.iterout);
    sscanf(parm.diffc->answer, "%lf", &wp.frac);
    sscanf(parm.hmax->answer, "%lf", &wp.hhmax);
    sscanf(parm.halpha->answer, "%lf", &wp.halpha);
    sscanf(parm.hbeta->answer, "%lf", &wp.hbeta);

    G_debug(3, "Parsing rain parameters");

    sscanf(parm.threads->answer, "%d", &threads);
    if (threads < 1)
    {
      G_warning(_("<%d> is not valid number of threads. Number of threads will be set on <%d>"),
      threads, abs(threads));
      threads = abs(threads);
    }
#if defined(_OPENMP)
    omp_set_num_threads(threads);
#else
    threads = 1;
#endif
    G_message(_("Number of threads: %d"), threads);

    /* if no rain map input, then: */
    if (parm.rain->answer == NULL) {
	/*Check for Rain Unique Value Input */
	/* if no rain unique value input */
	if (parm.rainval->answer == NULL) {
	    /*No rain input so use default */
	    sscanf(RAINVAL, "%lf", &wp.rain_val);
	    /* if rain unique input exist, load it */
	}
	else {
	    /*Unique value input only */
	    sscanf(parm.rainval->answer, "%lf", &wp.rain_val);
	}
	/* if Rain map exists */
    }
    else {
	/*Map input, so set rain_val to -999.99 */
	if (parm.rainval->answer == NULL) {
	    wp.rain_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose the map, discard the unique value */
	    wp.rain_val = -999.99;
	}
    }
    /* Report the final value of rain_val */
    G_debug(3, "rain_val is set to: %f\n", wp.rain_val);

    /* if no Mannings map, then: */
    if (parm.manin->answer == NULL) {
	/*Check for Manin Unique Value Input */
	/* if no Mannings unique value input */
	if (parm.maninval->answer == NULL) {
	    /*No Mannings input so use default */
	    sscanf(MANINVAL, "%lf", &wp.manin_val);
	    /* if Mannings unique input value exists, load it */
	}
	else {
	    /*Unique value input only */
	    sscanf(parm.maninval->answer, "%lf", &wp.manin_val);
	}
	/* if Mannings map exists */
    }
    else {
	/* Map input, set manin_val to -999.99 */
	if (parm.maninval->answer == NULL) {
	    wp.manin_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose map, discard the unique value */
	    wp.manin_val = -999.99;
	}
    }
    /* Report the final value of manin_val */
    G_debug(1, "manin_val is set to: %f\n", wp.manin_val);

    /* if no infiltration map, then: */
    if (parm.infil->answer == NULL) {
	/*Check for Infil Unique Value Input */
	/*if no infiltration unique value input */
	if (parm.infilval->answer == NULL) {
	    /*No infiltration unique value so use default */
	    sscanf(INFILVAL, "%lf", &wp.infil_val);
	    /* if infiltration unique value exists, load it */
	}
	else {
	    /*unique value input only */
	    sscanf(parm.infilval->answer, "%lf", &wp.infil_val);
	}
	/* if infiltration map exists */
    }
    else {
	/* Map input, set infil_val to -999.99 */
	if (parm.infilval->answer == NULL) {
	    wp.infil_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose map, discard the unique value */
	    wp.infil_val = -999.99;
	}
    }
    /* Report the final value of infil_val */
    G_debug(1, "infil_val is set to: %f\n", wp.infil_val);

    /* Recompute timesec from user input in minutes
     * to real timesec in seconds */
    wp.timesec = wp.timesec * 60.0;
    wp.iterout = wp.iterout * 60.0;
    if ((wp.timesec / wp.iterout) > 100.0 && wp.ts == 1)
	G_message(_("More than 100 files are going to be created !!!!!"));

    /* compute how big the raster is and set this to appr 2 walkers per cell */
    if (parm.nwalk->answer == NULL) {
	wp.maxwa = wp.mx * wp.my * 2;
	wp.rwalk = (double)(wp.mx * wp.my * 2.);
	G_message(_("default nwalk=%d, rwalk=%f"), wp.maxwa, wp.rwalk);
    }
    else {
	sscanf(parm.nwalk->answer, "%d", &wp.maxwa);
	wp.rwalk = (double)wp.maxwa;
    }

    /*      rwalk = (double) maxwa; */

    if (wp.conv != 1.0)
	G_message(_("Using metric conversion factor %f, step=%f"), wp.conv,
		  wp.step);

    init_library_globals(&wp);

 if ((wp.depth == NULL) && (wp.disch == NULL) && (wp.err == NULL))
        G_warning(_("You are not outputting any raster maps"));
    ret_val = input_data();
    if (ret_val != 1)
        G_fatal_error(_("Input failed"));

    alloc_grids_water();

    grad_check();
    main_loop();

    /* Exit with Success */
    exit(EXIT_SUCCESS);
}
