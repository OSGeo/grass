
/****************************************************************************
 *
 * MODULE:       r.sim.sediment: main program for sediment transport
 *               simulation (SIMWE)
 *
 * AUTHOR(S):    L. Mitas,  H. Mitasova, J. Hofierka
 * PURPOSE:      Sediment transport simulation (SIMWE)
 *
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
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
 *Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
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
#include <grass/Vect.h>
#include <grass/linkm.h>
#include <grass/bitmap.h>
/*#include <grass/site.h> */
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
    module->keywords = _("raster, sediment flow, erosion, deposition");
    module->description =
	_("Sediment transport and erosion/deposition simulation "
	  "using path sampling method (SIMWE)");

    parm.elevin = G_define_standard_option(G_OPT_R_INPUT);
    parm.elevin->key = "elevin";
    parm.elevin->description = _("Name of the elevation raster map [m]");
    parm.elevin->guisection = _("Input_options");

    parm.wdepth = G_define_standard_option(G_OPT_R_INPUT);
    parm.wdepth->key = "wdepth";
    parm.wdepth->description = _("Name of the water depth raster map [m]");
    parm.wdepth->guisection = _("Input_options");

    parm.dxin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dxin->key = "dxin";
    parm.dxin->description = _("Name of the x-derivatives raster map [m/m]");
    parm.dxin->guisection = _("Input_options");

    parm.dyin = G_define_standard_option(G_OPT_R_INPUT);
    parm.dyin->key = "dyin";
    parm.dyin->description = _("Name of the y-derivatives raster map [m/m]");
    parm.dyin->guisection = _("Input_options");

    parm.detin = G_define_standard_option(G_OPT_R_INPUT);
    parm.detin->key = "detin";
    parm.detin->description =
	_("Name of the detachment capacity coefficient raster map [s/m]");
    parm.detin->guisection = _("Input_options");

    parm.tranin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tranin->key = "tranin";
    parm.tranin->description =
	_("Name of the transport capacity coefficient raster map [s]");
    parm.tranin->guisection = _("Input_options");

    parm.tauin = G_define_standard_option(G_OPT_R_INPUT);
    parm.tauin->key = "tauin";
    parm.tauin->description =
	_("Name of the critical shear stress raster map [Pa]");
    parm.tauin->guisection = _("Input_options");

    parm.manin = G_define_standard_option(G_OPT_R_INPUT);
    parm.manin->key = "manin";
    parm.manin->required = NO;
    parm.manin->description = _("Name of the Mannings n raster map");
    parm.manin->guisection = _("Input_options");

    parm.maninval = G_define_option();
    parm.maninval->key = "maninval";
    parm.maninval->type = TYPE_DOUBLE;
    parm.maninval->answer = MANINVAL;
    parm.maninval->required = NO;
    parm.maninval->description = _("Name of the Mannings n value");
    parm.maninval->guisection = _("Input_options");

    /* needs to be updated to GRASS 6 vector format !! 
    parm.sfile = G_define_standard_option(G_OPT_V_INPUT);
    parm.sfile->key = "vector";
    parm.sfile->required = NO;
    parm.sfile->description =
	_("Name of the sampling locations vector points map");
    parm.sfile->guisection = _("Input_options");
*/

    parm.tc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.tc->key = "tc";
    parm.tc->required = NO;
    parm.tc->description = _("Output transport capacity raster map [kg/ms]");
    parm.tc->guisection = _("Output_options");

    parm.et = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.et->key = "et";
    parm.et->required = NO;
    parm.et->description =
	_("Output transp.limited erosion-deposition raster map [kg/m2s]");
    parm.et->guisection = _("Output_options");

    parm.conc = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.conc->key = "conc";
    parm.conc->required = NO;
    parm.conc->description =
	_("Output sediment concentration raster map [particle/m3]");
    parm.conc->guisection = _("Output_options");

    parm.flux = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.flux->key = "flux";
    parm.flux->required = NO;
    parm.flux->description = _("Output sediment flux raster map [kg/ms]");
    parm.flux->guisection = _("Output_options");

    parm.erdep = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.erdep->key = "erdep";
    parm.erdep->required = NO;
    parm.erdep->description =
	_("Output erosion-deposition raster map [kg/m2s]");
    parm.erdep->guisection = _("Output_options");

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

    if (G_get_set_window(&cellhd) == -1)
	exit(EXIT_FAILURE);

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
    /*  maskmap = parm.maskmap->answer; */
    detin = parm.detin->answer;
    tranin = parm.tranin->answer;
    tauin = parm.tauin->answer;
    manin = parm.manin->answer;
    tc = parm.tc->answer;
    et = parm.et->answer;
    conc = parm.conc->answer;
    flux = parm.flux->answer;
    erdep = parm.erdep->answer;
/*    sfile = parm.sfile->answer; */

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

    /*
     * G_set_embedded_null_value_mode(1);
     */

    if ((tc == NULL) && (et == NULL) && (conc == NULL) && (flux == NULL) &&
	(erdep == NULL))
	G_warning(_("You are not outputting any raster or site files"));
    ret_val = input_data();
    if (ret_val != 1)
	G_fatal_error(_("Input failed"));

    /* mandatory for si,sigma */

    si = (double **)G_malloc(sizeof(double *) * (my));
    for (l = 0; l < my; l++) {
	si[l] = (double *)G_malloc(sizeof(double) * (mx));
    }
    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    si[j][i] = 0.;
    }

    sigma = (double **)G_malloc(sizeof(double *) * (my));
    for (l = 0; l < my; l++) {
	sigma[l] = (double *)G_malloc(sizeof(double) * (mx));
    }
    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    sigma[j][i] = 0.;
    }

    /* memory allocation for output grids */

    dif = (float **)G_malloc(sizeof(float *) * (my));
    for (l = 0; l < my; l++) {
	dif[l] = (float *)G_malloc(sizeof(float) * (mx));
    }
    for (j = 0; j < my; j++) {
	for (i = 0; i < mx; i++)
	    dif[j][i] = 0.;
    }

    if (erdep != NULL || et != NULL) {
	er = (float **)G_malloc(sizeof(float *) * (my));
	for (l = 0; l < my; l++) {
	    er[l] = (float *)G_malloc(sizeof(float) * (mx));
	}
	for (j = 0; j < my; j++) {
	    for (i = 0; i < mx; i++)
		er[j][i] = 0.;
	}
    }

    /*  if (maskmap != NULL)
       bitmask = BM_create (cols, rows);
       IL_create_bitmask (&params, bitmask);
     */
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

/*
    if (fdwalkers != NULL)
	fclose(fdwalkers);

    if (sfile != NULL)
	G_sites_close(fw);
*/
    /* Exit with Success */
    exit(EXIT_SUCCESS);
}
