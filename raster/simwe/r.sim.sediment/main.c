
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
#define DIFFC	"0.8"
#define NITER   "10"
#define ITEROUT "2"
#define DENSITY "200"
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
#include <grass/linkm.h>
#include <grass/bitmap.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

/********************************/
/* Specific stuff               */

/********************************/

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
    int ii;
    int ret_val;
    double x_orig, y_orig;
    static int rand1 = 12345;
    static int rand2 = 67891;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("sediment flow"));
    G_add_keyword(_("erosion"));
    G_add_keyword(_("deposition"));
    module->description =
	_("Sediment transport and erosion/deposition simulation "
	  "using path sampling method (SIMWE).");

    parm.elevin = G_define_standard_option(G_OPT_R_ELEV);
    
    parm.wdepth = G_define_standard_option(G_OPT_R_INPUT);
    parm.wdepth->key = "wdepth";
    parm.wdepth->description = _("Name of water depth raster map [m]");

    parm.dxin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dxin->key = "dx";
    parm.dxin->description = _("Name of x-derivatives raster map [m/m]");

    parm.dyin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dyin->key = "dy";
    parm.dyin->description = _("Name of y-derivatives raster map [m/m]");
    
    parm.detin = G_define_standard_option(G_OPT_R_INPUT);
    parm.detin->key = "det";
    parm.detin->description =
	_("Name of detachment capacity coefficient raster map [s/m]");

    parm.tranin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tranin->key = "tran";
    parm.tranin->description =
	_("Name of transport capacity coefficient raster map [s]");
    
    parm.tauin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tauin->key = "tau";
    parm.tauin->description =
	_("Name of critical shear stress raster map [Pa]");

    parm.manin = G_define_standard_option(G_OPT_R_INPUT);
    parm.manin->key = "man";
    parm.manin->required = NO;
    parm.manin->description = _("Name of mannings n raster map");
    parm.manin->guisection = _("Input");

    parm.maninval = G_define_option();
    parm.maninval->key = "man_value";
    parm.maninval->type = TYPE_DOUBLE;
    parm.maninval->answer = MANINVAL;
    parm.maninval->required = NO;
    parm.maninval->description = _("Name of mannings n value");
    parm.maninval->guisection = _("Input");

    parm.outwalk = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.outwalk->key = "outwalk";
    parm.outwalk->required = NO;
    parm.outwalk->description =
	_("Base name of the output walkers vector points map");
    parm.outwalk->guisection = _("Output options");
    
    parm.observation = G_define_standard_option(G_OPT_V_INPUT);
    parm.observation->key = "observation";
    parm.observation->required = NO;
    parm.observation->description =
	_("Name of the sampling locations vector points map");
    parm.observation->guisection = _("Input options");

    parm.logfile = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.logfile->key = "logfile";
    parm.logfile->required = NO;
    parm.logfile->description =
	_("Name of the sampling points output text file. For each observation vector point the time series of sediment transport is stored.");
    parm.logfile->guisection = _("Output");

    parm.tc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tc->key = "tc";
    parm.tc->required = NO;
    parm.tc->description = _("Name for output transport capacity raster map [kg/ms]");
    parm.tc->guisection = _("Output");

    parm.et = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.et->key = "et";
    parm.et->required = NO;
    parm.et->description =
	_("Name for output transp.limited erosion-deposition raster map [kg/m2s]");
    parm.et->guisection = _("Output");

    parm.conc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.conc->key = "conc";
    parm.conc->required = NO;
    parm.conc->description =
	_("Name for output sediment concentration raster map [particle/m3]");
    parm.conc->guisection = _("Output");

    parm.flux = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.flux->key = "flux";
    parm.flux->required = NO;
    parm.flux->description = _("Name for output sediment flux raster map [kg/ms]");
    parm.flux->guisection = _("Output");

    parm.erdep = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.erdep->key = "erdep";
    parm.erdep->required = NO;
    parm.erdep->description =
	_("Name for output erosion-deposition raster map [kg/m2s]");
    parm.erdep->guisection = _("Output");

    parm.nwalk = G_define_option();
    parm.nwalk->key = "nwalk";
    parm.nwalk->type = TYPE_INTEGER;
    parm.nwalk->required = NO;
    parm.nwalk->description = _("Number of walkers");
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

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&cellhd);

    conv = G_database_units_to_meters_factor();

    mixx = cellhd.west * conv;
    maxx = cellhd.east * conv;
    miyy = cellhd.south * conv;
    mayy = cellhd.north * conv;

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
    hhc = hhmax = 0.;

#if 0
    bxmi = 2093113. * conv;
    bymi = 731331. * conv;
    bxma = 2093461. * conv;
    byma = 731529. * conv;
    bresx = 2. * conv;
    bresy = 2. * conv;
    maxwab = 100000;

    mx2o = (int)((bxma - bxmi) / bresx);
    my2o = (int)((byma - bymi) / bresy);

    /* relative small box coordinates: leave 1 grid layer for overlap */

    bxmi = bxmi - mixx + stepx;
    bymi = bymi - miyy + stepy;
    bxma = bxma - mixx - stepx;
    byma = byma - miyy - stepy;
    mx2 = mx2o - 2 * ((int)(stepx / bresx));
    my2 = my2o - 2 * ((int)(stepy / bresy));
#endif

    elevin = parm.elevin->answer;
    wdepth = parm.wdepth->answer;
    dxin = parm.dxin->answer;
    dyin = parm.dyin->answer;
    detin = parm.detin->answer;
    tranin = parm.tranin->answer;
    tauin = parm.tauin->answer;
    manin = parm.manin->answer;
    tc = parm.tc->answer;
    et = parm.et->answer;
    conc = parm.conc->answer;
    flux = parm.flux->answer;
    erdep = parm.erdep->answer;
    outwalk = parm.outwalk->answer; 

    /*      sscanf(parm.nwalk->answer, "%d", &maxwa); */
    sscanf(parm.niter->answer, "%d", &timesec);
    sscanf(parm.outiter->answer, "%d", &iterout);
/*    sscanf(parm.density->answer, "%d", &ldemo); */
    sscanf(parm.diffc->answer, "%lf", &frac);
    sscanf(parm.maninval->answer, "%lf", &manin_val);

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
    /*rwalk = (double) maxwa; */

    if (conv != 1.0)
	G_message(_("Using metric conversion factor %f, step=%f"), conv,
		  step);


    if ((tc == NULL) && (et == NULL) && (conc == NULL) && (flux == NULL) &&
	(erdep == NULL))
	G_warning(_("You are not outputting any raster or site files"));
    ret_val = input_data();
    if (ret_val != 1)
	G_fatal_error(_("Input failed"));

    /* mandatory for si,sigma */

    si = G_alloc_matrix(my, mx);
    sigma = G_alloc_matrix(my, mx);

    /* memory allocation for output grids */

    dif = G_alloc_fmatrix(my, mx);
    if (erdep != NULL || et != NULL)
	er = G_alloc_fmatrix(my, mx);

    seeds(rand1, rand2);
    grad_check();

    if (et != NULL)
	erod(si);
    /* treba dat output pre topoerdep */
    main_loop();

    if (tserie == NULL) {
	ii = output_data(0, 1.);
	if (ii != 1)
	    G_fatal_error(_("Cannot write raster maps"));
    }

    /* Exit with Success */
    exit(EXIT_SUCCESS);
}
