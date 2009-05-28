
/****************************************************************************
 *
 * MODULE:       r.sim.water: main program for hydrologic and sediment transport
 *               simulation (SIMWE)
 *
 * AUTHOR(S):    L. Mitas,  H. Mitasova, J. Hofierka
 * PURPOSE:      Hydrologic and sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
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
 *Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
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
#include <grass/Vect.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
/* #include <grass/site.h> */
#include <grass/glocale.h>

/********************************/
/* Specific stuff               */

/********************************/
#define MAIN
#include <grass/waterglobs.h>

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
    int i, ii, j, l;
    int ret_val;
    double x_orig, y_orig;
    static int rand1 = 12345;
    static int rand2 = 67891;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster, flow, hydrology");
    module->description =
	_("Overland flow hydrologic simulation using "
	  "path sampling method (SIMWE)");

    parm.elevin = G_define_standard_option(G_OPT_R_INPUT);
    parm.elevin->key = "elevin";
    parm.elevin->description = _("Name of the elevation raster map [m]");
    parm.elevin->guisection = _("Input_options");

    parm.dxin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dxin->key = "dxin";
    parm.dxin->description = _("Name of the x-derivatives raster map [m/m]");
    parm.dxin->guisection = _("Input_options");

    parm.dyin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dyin->key = "dyin";
    parm.dyin->description = _("Name of the y-derivatives raster map [m/m]");
    parm.dyin->guisection = _("Input_options");

    parm.rain = G_define_standard_option(G_OPT_R_INPUT);
    parm.rain->key = "rain";
    parm.rain->required = NO;
    parm.rain->description =
	_("Name of the rainfall excess rate (rain-infilt) raster map [mm/hr]");
    parm.rain->guisection = _("Input_options");

    parm.rainval = G_define_option();
    parm.rainval->key = "rain_val";
    parm.rainval->type = TYPE_DOUBLE;
    parm.rainval->answer = RAINVAL;
    parm.rainval->required = NO;
    parm.rainval->description =
	_("Rainfall excess rate unique value [mm/hr]");
    parm.rainval->guisection = _("Input_options");

    parm.infil = G_define_standard_option(G_OPT_R_INPUT);
    parm.infil->key = "infil";
    parm.infil->required = NO;
    parm.infil->description =
	_("Name of the runoff infiltration rate raster map [mm/hr]");
    parm.infil->guisection = _("Input_options");

    parm.infilval = G_define_option();
    parm.infilval->key = "infil_val";
    parm.infilval->type = TYPE_DOUBLE;
    parm.infilval->answer = INFILVAL;
    parm.infilval->required = NO;
    parm.infilval->description =
	_("Runoff infiltration rate unique value [mm/hr]");
    parm.infilval->guisection = _("Input_options");

    parm.manin = G_define_standard_option(G_OPT_R_INPUT);
    parm.manin->key = "manin";
    parm.manin->required = NO;
    parm.manin->description = _("Name of the Mannings n raster map");
    parm.manin->guisection = _("Input_options");

    parm.maninval = G_define_option();
    parm.maninval->key = "manin_val";
    parm.maninval->type = TYPE_DOUBLE;
    parm.maninval->answer = MANINVAL;
    parm.maninval->required = NO;
    parm.maninval->description = _("Mannings n unique value");
    parm.maninval->guisection = _("Input_options");

    parm.traps = G_define_standard_option(G_OPT_R_INPUT);
    parm.traps->key = "traps";
    parm.traps->required = NO;
    parm.traps->description =
	_("Name of the flow controls raster map (permeability ratio 0-1)");
    parm.traps->guisection = _("Input_options");

/*
    parm.sfile = G_define_standard_option(G_OPT_V_INPUT);
    parm.sfile->key = "vector";
    parm.sfile->required = NO;
    parm.sfile->description =
	_("Name of the sampling locations vector points map");
    parm.sfile->guisection = _("Input_options");
*/

    parm.depth = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.depth->key = "depth";
    parm.depth->required = NO;
    parm.depth->description = _("Output water depth raster map [m]");
    parm.depth->guisection = _("Output_options");

    parm.disch = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.disch->key = "disch";
    parm.disch->required = NO;
    parm.disch->description = _("Output water discharge raster map [m3/s]");
    parm.disch->guisection = _("Output_options");

    parm.err = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.err->key = "err";
    parm.err->required = NO;
    parm.err->description = _("Output simulation error raster map [m]");
    parm.err->guisection = _("Output_options");

/*
    parm.outwalk = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.outwalk->key = "outwalk";
    parm.outwalk->required = NO;
    parm.outwalk->description =
	_("Name of the output walkers vector points map");
    parm.outwalk->guisection = _("Output_options");
*/

    parm.nwalk = G_define_option();
    parm.nwalk->key = "nwalk";
    parm.nwalk->type = TYPE_INTEGER;
    parm.nwalk->required = NO;
    parm.nwalk->description =
	_("Number of walkers, default is twice the no. of cells");
    parm.nwalk->guisection = _("Parameters");

    parm.niter = G_define_option();
    parm.niter->key = "niter";
    parm.niter->type = TYPE_INTEGER;
    parm.niter->answer = NITER;
    parm.niter->required = NO;
    parm.niter->description = _("Time used for iterations [minutes]");
    parm.niter->guisection = _("Parameters");

    parm.outiter = G_define_option();
    parm.outiter->key = "outiter";
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
    parm.diffc->key = "diffc";
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
    parm.hmax->description =
	_("Threshold water depth [m] (diffusion increases after this water depth is reached)");
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


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_get_set_window(&cellhd) == -1)
	exit(EXIT_FAILURE);

    conv = G_database_units_to_meters_factor();

    mixx = conv * cellhd.west;
    maxx = conv * cellhd.east;
    miyy = conv * cellhd.south;
    mayy = conv * cellhd.north;

    stepx = cellhd.ew_res * conv;
    stepy = cellhd.ns_res * conv;
    /*  step = amin1(stepx,stepy); */
    step = (stepx + stepy) / 2.;
    mx = cellhd.cols;
    my = cellhd.rows;
    x_orig = cellhd.west * conv;
    y_orig = cellhd.south * conv;	/* do we need this? */
    xmin = 0.;
    ymin = 0.;
    xp0 = xmin + stepx / 2.;
    yp0 = ymin + stepy / 2.;
    xmax = xmin + stepx * (float)mx;
    ymax = ymin + stepy * (float)my;

    ts = flag.tserie->answer;

    elevin = parm.elevin->answer;
    dxin = parm.dxin->answer;
    dyin = parm.dyin->answer;
    rain = parm.rain->answer;
    infil = parm.infil->answer;
    traps = parm.traps->answer;
    manin = parm.manin->answer;
    depth = parm.depth->answer;
    disch = parm.disch->answer;
    err = parm.err->answer;
/*    outwalk = parm.outwalk->answer; */
/*    sfile = parm.sfile->answer; */

    sscanf(parm.niter->answer, "%d", &timesec);
    sscanf(parm.outiter->answer, "%d", &iterout);
/*    sscanf(parm.density->answer, "%d", &ldemo); */
    sscanf(parm.diffc->answer, "%lf", &frac);
    sscanf(parm.hmax->answer, "%lf", &hhmax);
    sscanf(parm.halpha->answer, "%lf", &halpha);
    sscanf(parm.hbeta->answer, "%lf", &hbeta);

    /* if no rain map input, then: */
    if (parm.rain->answer == NULL) {
	/*Check for Rain Unique Value Input */
	/* if no rain unique value input */
	if (parm.rainval->answer == NULL) {
	    /*No rain input so use default */
	    sscanf(RAINVAL, "%lf", &rain_val);
	    /* if rain unique input exist, load it */
	}
	else {
	    /*Unique value input only */
	    sscanf(parm.rainval->answer, "%lf", &rain_val);
	}
	/* if Rain map exists */
    }
    else {
	/*Map input, so set rain_val to -999.99 */
	if (parm.rainval->answer == NULL) {
	    rain_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose the map, discard the unique value */
	    rain_val = -999.99;
	}
    }
    /* Report the final value of rain_val */
    G_debug(3, "rain_val is set to: %f\n", rain_val);

    /* if no Mannings map, then: */
    if (parm.manin->answer == NULL) {
	/*Check for Manin Unique Value Input */
	/* if no Mannings unique value input */
	if (parm.maninval->answer == NULL) {
	    /*No Mannings input so use default */
	    sscanf(MANINVAL, "%lf", &manin_val);
	    /* if mannings unique input value exists, load it */
	}
	else {
	    /*Unique value input only */
	    sscanf(parm.maninval->answer, "%lf", &manin_val);
	}
	/* if Mannings map exists */
    }
    else {
	/* Map input, set manin_val to -999.99 */
	if (parm.maninval->answer == NULL) {
	    manin_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose map, discard the unique value */
	    manin_val = -999.99;
	}
    }
    /* Report the final value of manin_val */
    G_debug(3, "manin_val is set to: %f\n", manin_val);

    /* if no infiltration map, then: */
    if (parm.infil->answer == NULL) {
	/*Check for Infil Unique Value Input */
	/*if no infiltration unique value input */
	if (parm.infilval->answer == NULL) {
	    /*No infiltration unique value so use default */
	    sscanf(INFILVAL, "%lf", &infil_val);
	    /* if infiltration unique value exists, load it */
	}
	else {
	    /*unique value input only */
	    sscanf(parm.infilval->answer, "%lf", &infil_val);
	}
	/* if infiltration map exists */
    }
    else {
	/* Map input, set infil_val to -999.99 */
	if (parm.infilval->answer == NULL) {
	    infil_val = -999.99;
	}
	else {
	    /*both map and unique value exist */
	    /*Choose map, discard the unique value */
	    infil_val = -999.99;
	}
    }
    /* Report the final value of infil_val */
    G_debug(3, "infil_val is set to: %f\n", infil_val);

    /* Recompute timesec from user input in minutes
     * to real timesec in seconds */
    timesec = timesec * 60.0;
    iterout = iterout * 60.0;
    if ((timesec / iterout) > 100.0)
	G_message(_("More than 100 files are going to be created !!!!!"));

    /* compute how big the raster is and set this to appr 2 walkers per cell */
    if (parm.nwalk->answer == NULL) {
	maxwa = mx * my * 2;
	rwalk = (double)(mx * my * 2.);
	G_message(_("default nwalk=%d, rwalk=%f"), maxwa, rwalk);
    }
    else {
	sscanf(parm.nwalk->answer, "%d", &maxwa);
	rwalk = (double)maxwa;
    }

    /*      rwalk = (double) maxwa; */

    if (conv != 1.0)
	G_message(_("Using metric conversion factor %f, step=%f"), conv,
		  step);

    /*
     * G_set_embedded_null_value_mode(1);
     */

/* replaced with condition that skips outwalk */
/*    if ((depth == NULL) && (disch == NULL) && (err == NULL) &&
	(outwalk == NULL))
	G_warning(_("You are not outputting any raster or vector points maps"));
    ret_val = input_data();
    if (ret_val != 1)
	G_fatal_error(_("Input failed"));
*/

 if ((depth == NULL) && (disch == NULL) && (err == NULL))
        G_warning(_("You are not outputting any raster maps"));
    ret_val = input_data();
    if (ret_val != 1)
        G_fatal_error(_("Input failed"));


    /* memory allocation for output grids */
    G_debug(2, "beginning memory allocation for output grids");

    gama = (double **)G_malloc(sizeof(double *) * (my));
    for (l = 0; l < my; l++) {
	gama[l] = (double *)G_malloc(sizeof(double) * (mx));
    }
    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    gama[j][i] = 0.;
    }

    if (err != NULL) {
	gammas = (double **)G_malloc(sizeof(double *) * (my));
	for (l = 0; l < my; l++) {
	    gammas[l] = (double *)G_malloc(sizeof(double) * (mx));
	}
	for (j = 0; j < my; j++) {
	    for (i = 0; i < mx; i++)
		gammas[j][i] = 0.;
	}
    }

    dif = (float **)G_malloc(sizeof(float *) * (my));
    for (l = 0; l < my; l++) {
	dif[l] = (float *)G_malloc(sizeof(float) * (mx));
    }
    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    dif[j][i] = 0.;
    }

    /*  if (maskmap != NULL)
       bitmask = BM_create (cols, rows);
       IL_create_bitmask (&params, bitmask);
     */
    G_debug(2, "seeding randoms");
    seeds(rand1, rand2);
    grad_check();
    main_loop();

    if (ts == 0) {
	ii = output_data(0, 1.);
	if (ii != 1)
	    G_fatal_error(_("Cannot write raster maps"));
    }

/*
    if (fdwalkers != NULL)
	fclose(fdwalkers);

    if (sfile != NULL)
	fclose(fw);
*/
    /* Exit with Success */
    exit(EXIT_SUCCESS);
}
