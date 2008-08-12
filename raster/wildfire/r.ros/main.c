
/****************************************************************************
 *
 * MODULE:       r.ros
 * AUTHOR(S):    Jianping Xu, Rutgers University, 1993
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>, Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>
 * PURPOSE:
 *
 * This raster module creates three raster map layers:
 *	1. Base (perpendicular) rate of spread (ROS);
 *	2. Maximum (forward) ROS; and
 * 	3. Direction of the Maximum ROS.
 * The calculation of the two ROS values for each raster
 * cell is based on the Fortran code by Pat Andrews (1983)
 * of the Northern Forest Fire Laboratory, USDA Forest
 * Service. These three raster map layers are expected to
 * be the inputs for a seperate GRASS raster module 
 * 'r.spread'.
 * 
 * 'r.ros' can be run in two standard GRASS modes:
 * interactive and command line. For an interactive run,
 * type in 
 *	r.ros 
 * and follow the prompts; for a command line mode, 
 * type in 
 * 	r.ros [-v] model=name [moisture_1h=name] 
 *			[moisture_10h=name] 
 *			[moisture_100h=name] 
 *			moisture_live=name [velocity=name]
 * 			[direction=name] [slope=name] 
 *			[aspect=name] output=name
 * where:
 *   Flag:
 *   Raster Maps:
 *      model   	1-13: the standard fuel models,
 *			all other numbers: same as barriers;
 *  	moisture_1h   	100*moisture_content;
 *	moisture_10h  	100*moisture_content;
 *	moisture_100h   100*moisture_content;
 *	moisture_live   100*moisture_content;
 *	velocity   	ft/minute;
 *	direction   	degree;
 *	slope   	degree;
 * 	aspect   	degree starting from East, anti-clockwise;
 *	output 
 *	  for Base ROS	cm/min (technically not ft/min);
 *  	  for Max ROS	cm/min (technically not ft/min);
 *	  for Direction	degree.
 *
 * Note that the name given as output will be used as 
 * PREFIX for several actual raster maps. For example, if 
 * my_ros is given as the output name, 'r.ros' will 
 * actually produce three raster maps named my_ros.base, 
 * my_ros.max, and my_ros.maxdir, or even my_ros.spotdist
 * respectively. 
 *   
 * COPYRIGHT:    (C) 2000-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"


#define DATA(map, r, c)         (map)[(r) * ncols + (c)]
#define DEBUG


/*measurements of the 13 fuel models, input of Rothermel equation (1972) */
float WO[4][14] =
    { {0, 0.034, 0.092, 0.138, 0.230, 0.046, 0.069, 0.052, 0.069, 0.134,
       0.138, 0.069, 0.184, 0.322},
{0, 0, 0.046, 0, 0.184, 0.023, 0.115, 0.086, 0.046, 0.019, 0.092, 0.207,
 0.644, 1.058},
{0, 0, 0.023, 0, 0.092, 0, 0.092, 0.069, 0.115, 0.007, 0.230, 0.253, 0.759,
 1.288},
{0, 0, 0.023, 0, 0.230, 0.092, 0, 0.017, 0, 0, 0.092, 0, 0}
};

					    /*ovendry fuel loading, lb./ft.^2 */
float DELTA[] = { 0, 1.0, 1.0, 2.5, 6.0, 2.0, 2.5, 2.5,
    0.2, 0.2, 1.0, 1.0, 2.3, 3.0
};				/*fuel depth, ft. */
float SIGMA[4][14] =
    { {0, 3500, 3000, 1500, 2000, 2000, 1750, 1750, 2000, 2500, 2000, 1500,
       1500, 1500},
{0, 0, 109, 0, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109},
{0, 0, 30, 0, 30, 0, 30, 30, 30, 30, 30, 30, 30, 30},
{0, 0, 1500, 0, 1500, 1500, 0, 1500, 0, 0, 1500, 0, 0, 0}
};

			 /*fuel particale surface-area-to-volume ratio, 1/ft. */
float MX[] = { 0, 0.12, 0.15, 0.25, 0.20, 0.20, 0.25, 0.40,
    0.30, 0.25, 0.25, 0.15, 0.20, 0.25
};				/*moisture content of extinction */

CELL *map_elev;			/*full array for elevation map layer (for spotting) */
int nrows, ncols;
struct Cell_head window;


int main(int argc, char *argv[])
{

	/***input of Rothermel equation (1972)***/
    float h = 8000.0,		/*heat of combustion, BTU/lb. */
	rhop = 32,		/*ovendry fuel density, lb./ft.^3 */
	ST = 0.0555;		/*fuel total mineral content, lb. minerals/lb. ovendry */

    float sigma[14];

	/***derived parameters of Rothermel equation (1972)***/
    float R,			/*rate of spread, ft./min.
				   R = IR*xi*(1+phiw+phis)/(rhob*epsilon*Qig)  */
      IR,			/*reaction intensity, BTU/ft.^2/min.
				   IR = gamma*wn*h*etaM*etas  */
      gamma,			/*optimum reation velosity, 1/min.
				   gamma = gammamax*(beta/betaop)^A*
				   exp(A(1-beta/betaop))  */
      gammamax,			/*maximum reation velosity, 1/min.
				   gammamax = sigma^1.5/(495+0.0594*sigma^1.5)  */
      betaop,			/*optimum packing ratio 
				   betaop = 3.348/(sigma^0.8189)  */
      A,			/*A = 1/(4.77*sigma^0.1-7.27)  */
      etas = 0.174 / pow(0.01, 0.19),	/*mineral damping coefficient */
	xi,			/*propagating flux ratio,
				   xi = exp((0.792+0.681*sigma^0.5)(beta+0.1))/
				   (192+0.2595*sigma)  */
	phiw,			/*wind coefficient,  phiw = C*U^B/(beta/betaop)^E */
	C,			/*C = 7.47/exp(0.133*sigma^0.55)  */
	B,			/*B = 0.02526*sigma^.054   */
	E,			/*E = 0.715/exp(0.000359*sigma)  */
	phis,			/*slope coefficient,phis = 5.275/beta^0.3*(tan(theta)^2) */
	rhob,			/*ovendry bulk density, lb./ft.^3, rohb = wo/delta */
	epsilon[4][14],		/*effective heating number, epsilon = 1/exp(138/sigma) */
	Qig[14],		/*heat of preignition, BTU/lb.  Qig = 250+1116*Mf */
	beta;			/*packing ratio,  beta = rhob/rhop  */

	/***intermediate variables***/
    float R0,			/*base ROS (w/out wind/slope) */
      Rdir, sin_fac, cos_fac, Ffactor_all[4][14],	/*in all fuel subclasses by sigma/WO */
      Ffactor_in_dead[3][14],	/*in dead fuel subclasses by sigma/WO */
      Ffactor_dead[14],		/*dead fuel weight by sigma/WO */
      Ffactor_live[14],		/*live fuel weight by sigma/WO */
      Gfactor_in_dead[3][14],	/*in dead fuel by the 6 classes */
      G1, G2, G3, G4, G5, wo_dead[14],	/*dead fuel total load */
      wn_dead,			/*net dead fuel total load */
      wn_live,			/*net live fuel (total) load */
      class_sum, moisture[4],	/*moistures of 1-h,10-h,100-h,live fuels */
      Mf_dead,			/*total moisture of dead fuels */
      etaM_dead,		/*dead fuel misture damping coefficent */
      etaM_live,		/*live fuel misture damping coefficent */
      xmext,			/*live fuel moisture of extinction */
      phi_ws,			/*wind and slope conbined coefficient */
      wmfd, fdmois, fined, finel;

    /*other local variables */
    int col, row,
	spotting,
	model, class,
	fuel_fd = 0,
	mois_1h_fd = 0, mois_10h_fd = 0, mois_100h_fd = 0, mois_live_fd = 0,
	vel_fd = 0, dir_fd = 0,
	elev_fd = 0, slope_fd = 0, aspect_fd = 0,
	base_fd = 0, max_fd = 0, maxdir_fd = 0, spotdist_fd = 0;

    char name_base[60], name_max[60], name_maxdir[60], name_spotdist[60];

    CELL *fuel,			/*cell buffer for fuel model map layer */
     *mois_1h,			/*cell buffer for 1-hour fuel moisture map layer */
     *mois_10h,			/*cell buffer for 10-hour fuel moisture map layer */
     *mois_100h,		/*cell buffer for 100-hour fuel moisture map layer */
     *mois_live,		/*cell buffer for live fuel moisture map layer */
     *vel,			/*cell buffer for wind velocity map layer */
     *dir,			/*cell buffer for wind direction map layer */
     *elev = NULL,		/*cell buffer for elevation map layer (for spotting) */
	*slope,			/*cell buffer for slope map layer */
	*aspect,		/*cell buffer for aspect map layer */
	*base,			/*cell buffer for base ROS map layer */
	*max,			/*cell buffer for max ROS map layer */
	*maxdir,		/*cell buffer for max ROS direction map layer */
	*spotdist = NULL;	/*cell buffer for max spotting distance map layer */

    extern struct Cell_head window;

    struct
    {
	struct Option *model,
	    *mois_1h, *mois_10h, *mois_100h, *mois_live,
	    *vel, *dir, *elev, *slope, *aspect, *output;
    } parm;

    /* please, remove before GRASS 7 released */
    struct Flag *flag1, *flag2;
    struct GModule *module;

    /* initialize access to database and create temporary files */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Generates three, or four raster map layers showing 1) the base "
	  "(perpendicular) rate of spread (ROS), 2) the maximum (forward) ROS, "
	  "3) the direction of the maximum ROS, and optionally 4) the "
	  "maximum potential spotting distance.");

    parm.model = G_define_option();
    parm.model->key = "model";
    parm.model->type = TYPE_STRING;
    parm.model->required = YES;
    parm.model->gisprompt = "old,cell,raster";
    parm.model->description = _("Name of raster map containing fuel MODELs");

    parm.mois_1h = G_define_option();
    parm.mois_1h->key = "moisture_1h";
    parm.mois_1h->type = TYPE_STRING;
    parm.mois_1h->gisprompt = "old,cell,raster";
    parm.mois_1h->description =
	_("Name of raster map containing the 1-HOUR fuel MOISTURE (%)");

    parm.mois_10h = G_define_option();
    parm.mois_10h->key = "moisture_10h";
    parm.mois_10h->type = TYPE_STRING;
    parm.mois_10h->gisprompt = "old,cell,raster";
    parm.mois_10h->description =
	_("Name of raster map containing the 10-HOUR fuel MOISTURE (%)");

    parm.mois_100h = G_define_option();
    parm.mois_100h->key = "moisture_100h";
    parm.mois_100h->type = TYPE_STRING;
    parm.mois_100h->gisprompt = "old,cell,raster";
    parm.mois_100h->description =
	_("Name of raster map containing the 100-HOUR fuel MOISTURE (%)");

    parm.mois_live = G_define_option();
    parm.mois_live->key = "moisture_live";
    parm.mois_live->type = TYPE_STRING;
    parm.mois_live->required = YES;
    parm.mois_live->gisprompt = "old,cell,raster";
    parm.mois_live->description =
	_("Name of raster map containing LIVE fuel MOISTURE (%)");

    parm.vel = G_define_option();
    parm.vel->key = "velocity";
    parm.vel->type = TYPE_STRING;
    parm.vel->gisprompt = "old,cell,raster";
    parm.vel->description =
	_("Name of raster map containing midflame wind VELOCITYs (ft/min)");

    parm.dir = G_define_option();
    parm.dir->key = "direction";
    parm.dir->type = TYPE_STRING;
    parm.dir->gisprompt = "old,cell,raster";
    parm.dir->description =
	_("Name of raster map containing wind DIRECTIONs (degree)");

    parm.slope = G_define_option();
    parm.slope->key = "slope";
    parm.slope->type = TYPE_STRING;
    parm.slope->gisprompt = "old,cell,raster";
    parm.slope->description =
	_("Name of raster map containing SLOPE (degree)");

    parm.aspect = G_define_option();
    parm.aspect->key = "aspect";
    parm.aspect->type = TYPE_STRING;
    parm.aspect->gisprompt = "old,cell,raster";
    parm.aspect->description =
	_("Name of raster map containing ASPECT (degree, anti-clockwise from E)");

    parm.elev = G_define_option();
    parm.elev->key = "elevation";
    parm.elev->type = TYPE_STRING;
    parm.elev->gisprompt = "old,cell,raster";
    parm.elev->description =
	_("Name of raster map containing ELEVATION (m) (required w/ -s)");

    parm.output = G_define_option();
    parm.output->key = "output";
    parm.output->type = TYPE_STRING;
    parm.output->required = YES;
    parm.output->gisprompt = "new,cell,raster";
    parm.output->description =
	_("Name of raster map to contain results (several new layers)");

    /* please, remove before GRASS 7 released */
    flag1 = G_define_flag();
    flag1->key = 'v';
    flag1->description = _("Run verbosely");

    flag2 = G_define_flag();
    flag2->key = 's';
    flag2->description = _("Also produce maximum SPOTTING distance");

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if (flag1->answer) {
	putenv("GRASS_VERBOSE=3");
	G_warning(_("The '-v' flag is superseded and will be removed "
		    "in future. Please use '--verbose' instead."));
    }

    spotting = flag2->answer;

    /*  Check if input layers exists in data base  */
    if (G_find_cell2(parm.model->answer, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.model->answer);

    if (!
	(parm.mois_1h->answer || parm.mois_10h->answer ||
	 parm.mois_100h->answer)) {
	G_warning
	    ("no dead fuel moisture is given. At least one of the 1-h, 10-h, 100-h moisture layers is required.");
	G_usage();
	exit(EXIT_FAILURE);
    }

    if (parm.mois_1h->answer) {
	if (G_find_cell2(parm.mois_1h->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"),
			  parm.mois_1h->answer);
    }
    if (parm.mois_10h->answer) {
	if (G_find_cell2(parm.mois_10h->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"),
			  parm.mois_10h->answer);
    }
    if (parm.mois_100h->answer) {
	if (G_find_cell2(parm.mois_100h->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"),
			  parm.mois_100h->answer);
    }

    if (G_find_cell2(parm.mois_live->answer, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.mois_live->answer);

    if (parm.vel->answer && !(parm.dir->answer)) {
	G_warning
	    ("a wind direction layer should be given if the wind velocity layer--%s-- has been given\n",
	     parm.vel->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (!(parm.vel->answer) && parm.dir->answer) {
	G_warning
	    ("a wind velocity layer should be given if the wind direction layer--%s-- has been given\n",
	     parm.dir->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (parm.vel->answer) {
	if (G_find_cell2(parm.vel->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), parm.vel->answer);
    }
    if (parm.dir->answer) {
	if (G_find_cell2(parm.dir->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), parm.dir->answer);
    }

    if (parm.slope->answer && !(parm.aspect->answer)) {
	G_warning
	    ("an aspect layer should be given if the slope layer--%s-- has been given\n",
	     parm.slope->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (!(parm.slope->answer) && parm.aspect->answer) {
	G_warning
	    ("a slope layer should be given if the aspect layer--%s-- has been given\n",
	     parm.aspect->answer);
	G_usage();
	exit(EXIT_FAILURE);
    }
    if (parm.slope->answer) {
	if (G_find_cell2(parm.slope->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), parm.slope->answer);
    }
    if (parm.aspect->answer) {
	if (G_find_cell2(parm.aspect->answer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"),
			  parm.aspect->answer);
    }

    if (spotting) {
	if (!(parm.elev->answer)) {
	    G_warning
		("an elevation layer should be given if considering spotting\n");
	    G_usage();
	    exit(EXIT_FAILURE);
	}
	else {
	    if (G_find_cell2(parm.elev->answer, "") == NULL)
		G_fatal_error(_("Raster map <%s> not found"),
			      parm.elev->answer);
	}
    }

    /*  Check if specified output layer name IS LEGAL  */
    if (G_legal_filename(parm.output->answer) < 0)
	G_fatal_error("%s - illegal name", parm.output->answer);

    /*assign names of the three output ROS layers */
    sprintf(name_base, "%s.base", parm.output->answer);
    sprintf(name_max, "%s.max", parm.output->answer);
    sprintf(name_maxdir, "%s.maxdir", parm.output->answer);

    /*check if the output layer names EXIST */
    if (G_find_cell2(name_base, G_mapset()))
	G_fatal_error(_("Raster map <%s> already exists in mapset <%s>, select another name"),
		      name_base, G_mapset());

    if (G_find_cell2(name_max, G_mapset()))
	G_fatal_error(_("Raster map <%s> already exists in mapset <%s>, select another name"),
		      name_max, G_mapset());

    if (G_find_cell2(name_maxdir, G_mapset()))
	G_fatal_error(_("Raster map <%s> already exists in mapset <%s>, select another name"),
		      name_maxdir, G_mapset());

    /*assign a name to output SPOTTING distance layer */
    if (spotting) {
	sprintf(name_spotdist, "%s.spotdist", parm.output->answer);
	if (G_find_cell2(name_spotdist, G_mapset()))
	    G_fatal_error(_("Raster map <%s> already exists in mapset <%s>, select another name"),
			  name_spotdist, G_mapset());
    }

    /*  Get database window parameters  */
    if (G_get_window(&window) < 0)
	G_fatal_error("can't read current window parameters");

    /*  find number of rows and columns in window    */
    nrows = G_window_rows();
    ncols = G_window_cols();

    fuel = G_allocate_cell_buf();
    mois_1h = G_allocate_cell_buf();
    mois_10h = G_allocate_cell_buf();
    mois_100h = G_allocate_cell_buf();
    mois_live = G_allocate_cell_buf();
    vel = G_allocate_cell_buf();
    dir = G_allocate_cell_buf();
    slope = G_allocate_cell_buf();
    aspect = G_allocate_cell_buf();
    base = G_allocate_cell_buf();
    max = G_allocate_cell_buf();
    maxdir = G_allocate_cell_buf();
    if (spotting) {
	spotdist = G_allocate_cell_buf();
	elev = G_allocate_cell_buf();
	map_elev = (CELL *) G_calloc(nrows * ncols, sizeof(CELL));
    }

    /*  Open input cell layers for reading  */

    fuel_fd =
	G_open_cell_old(parm.model->answer,
			G_find_cell2(parm.model->answer, ""));
    if (fuel_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"),
		      parm.model->answer);

    if (parm.mois_1h->answer) {
	mois_1h_fd =
	    G_open_cell_old(parm.mois_1h->answer,
			    G_find_cell2(parm.mois_1h->answer, ""));
	if (mois_1h_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.mois_1h->answer);
    }
    if (parm.mois_10h->answer) {
	mois_10h_fd =
	    G_open_cell_old(parm.mois_10h->answer,
			    G_find_cell2(parm.mois_10h->answer, ""));
	if (mois_10h_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.mois_10h->answer);
    }
    if (parm.mois_100h->answer) {
	mois_100h_fd =
	    G_open_cell_old(parm.mois_100h->answer,
			    G_find_cell2(parm.mois_100h->answer, ""));
	if (mois_100h_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.mois_100h->answer);
    }

    mois_live_fd =
	G_open_cell_old(parm.mois_live->answer,
			G_find_cell2(parm.mois_live->answer, ""));
    if (mois_live_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"),
		      parm.mois_live->answer);

    if (parm.vel->answer) {
	vel_fd =
	    G_open_cell_old(parm.vel->answer,
			    G_find_cell2(parm.vel->answer, ""));
	if (vel_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.vel->answer);
    }
    if (parm.dir->answer) {
	dir_fd =
	    G_open_cell_old(parm.dir->answer,
			    G_find_cell2(parm.dir->answer, ""));
	if (dir_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.dir->answer);
    }

    if (parm.slope->answer) {
	slope_fd =
	    G_open_cell_old(parm.slope->answer,
			    G_find_cell2(parm.slope->answer, ""));
	if (slope_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.slope->answer);
    }
    if (parm.aspect->answer) {
	aspect_fd =
	    G_open_cell_old(parm.aspect->answer,
			    G_find_cell2(parm.aspect->answer, ""));
	if (aspect_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.aspect->answer);
    }

    if (spotting) {
	elev_fd =
	    G_open_cell_old(parm.elev->answer,
			    G_find_cell2(parm.elev->answer, ""));
	if (elev_fd < 0)
	    G_fatal_error(_("Unable to open raster map <%s>"),
			  parm.elev->answer);
    }

    base_fd = G_open_cell_new(name_base);
    max_fd = G_open_cell_new(name_max);
    maxdir_fd = G_open_cell_new(name_maxdir);
    if (spotting)
	spotdist_fd = G_open_cell_new(name_spotdist);

    /*compute weights, combined wo, and combined sigma */
    /*wo[model] -- simple sum of WO[class][model] by all fuel subCLASS */
    /*sigma[model] -- weighted sum of SIGMA[class][model] by all fuel subCLASS       *epsilon[class][model] */
    for (model = 1; model <= 13; model++) {
	class_sum = 0.0;
	wo_dead[model] = 0.0;
	sigma[model] = 0.0;
	for (class = 0; class <= 3; class++) {
	    class_sum = class_sum + WO[class][model] * SIGMA[class][model];
	    if (SIGMA[class][model] > 0.0) {
		epsilon[class][model] = exp(-138.0 / SIGMA[class][model]);
	    }
	    else {
		epsilon[class][model] = 0.0;
	    }
	}
	for (class = 0; class <= 3; class++) {
	    Ffactor_all[class][model] =
		WO[class][model] * SIGMA[class][model] / class_sum;
	    sigma[model] =
		sigma[model] +
		SIGMA[class][model] * Ffactor_all[class][model];
	}
	class_sum = 0.0;
	for (class = 0; class <= 2; class++) {
	    wo_dead[model] = wo_dead[model] + WO[class][model];
	    class_sum = class_sum + WO[class][model] * SIGMA[class][model];
	}
	for (class = 0; class <= 2; class++) {
	    Ffactor_in_dead[class][model] =
		WO[class][model] * SIGMA[class][model] / class_sum;
	}

	/* compute G factor for each of the 6 subclasses */
	G1 = 0.0;
	G2 = 0.0;
	G3 = 0.0;
	G4 = 0.0;
	G5 = 0.0;
	for (class = 0; class <= 2; class++) {
	    if (SIGMA[class][model] >= 1200)
		G1 = G1 + Ffactor_in_dead[class][model];
	    if (SIGMA[class][model] < 1200 && SIGMA[class][model] >= 192)
		G2 = G2 + Ffactor_in_dead[class][model];
	    if (SIGMA[class][model] < 192 && SIGMA[class][model] >= 96)
		G3 = G3 + Ffactor_in_dead[class][model];
	    if (SIGMA[class][model] < 96 && SIGMA[class][model] >= 48)
		G4 = G4 + Ffactor_in_dead[class][model];
	    if (SIGMA[class][model] < 48 && SIGMA[class][model] >= 16)
		G5 = G5 + Ffactor_in_dead[class][model];
	}
	for (class = 0; class <= 2; class++) {
	    if (SIGMA[class][model] >= 1200)
		Gfactor_in_dead[class][model] = G1;
	    if (SIGMA[class][model] < 1200 && SIGMA[class][model] >= 192)
		Gfactor_in_dead[class][model] = G2;
	    if (SIGMA[class][model] < 192 && SIGMA[class][model] >= 96)
		Gfactor_in_dead[class][model] = G3;
	    if (SIGMA[class][model] < 96 && SIGMA[class][model] >= 48)
		Gfactor_in_dead[class][model] = G4;
	    if (SIGMA[class][model] < 48 && SIGMA[class][model] >= 16)
		Gfactor_in_dead[class][model] = G5;
	    if (SIGMA[class][model] < 16)
		Gfactor_in_dead[class][model] = 0.0;
	}

	Ffactor_dead[model] =
	    class_sum / (class_sum + WO[3][model] * SIGMA[3][model]);
	Ffactor_live[model] = 1 - Ffactor_dead[model];
    }

    /*if considering spotting, read elevation map into an array */
    if (spotting)
	for (row = 0; row < nrows; row++) {
	    if (G_get_map_row(elev_fd, elev, row) < 0)
		G_fatal_error("cannot get map row!");
	    for (col = 0; col < ncols; col++)
		DATA(map_elev, row, col) = elev[col];
	}

    /*major computation: compute ROSs one cell a time */
    G_message(_("Percent Completed ... "));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	if (G_get_map_row(fuel_fd, fuel, row) < 0)
	    G_fatal_error("cannot get map row: %d!", row);
	if (parm.mois_1h->answer)
	    if (G_get_map_row(mois_1h_fd, mois_1h, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (parm.mois_10h->answer)
	    if (G_get_map_row(mois_10h_fd, mois_10h, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (parm.mois_100h->answer)
	    if (G_get_map_row(mois_100h_fd, mois_100h, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (G_get_map_row(mois_live_fd, mois_live, row) < 0)
	    G_fatal_error("cannot get map row: %d!", row);
	if (parm.vel->answer)
	    if (G_get_map_row(vel_fd, vel, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (parm.dir->answer)
	    if (G_get_map_row(dir_fd, dir, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (parm.slope->answer)
	    if (G_get_map_row(slope_fd, slope, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);
	if (parm.aspect->answer)
	    if (G_get_map_row(aspect_fd, aspect, row) < 0)
		G_fatal_error("cannot get map row: %d!", row);

	/*initialize cell buffers for output map layers */
	for (col = 0; col < ncols; col++) {
	    base[col] = max[col] = maxdir[col] = 0;
	    if (spotting)
		spotdist[col] = 0;
	}

	for (col = 0; col < ncols; col++) {
	    /*check if a fuel is within the 13 models,
	     *if not, no processing; useful when no data presents*/
	    if (fuel[col] < 1 || fuel[col] > 13)
		continue;
	    if (parm.mois_1h->answer)
		moisture[0] = 0.01 * mois_1h[col];
	    if (parm.mois_10h->answer)
		moisture[1] = 0.01 * mois_10h[col];
	    if (parm.mois_100h->answer)
		moisture[2] = 0.01 * mois_100h[col];
	    moisture[3] = 0.01 * mois_live[col];
	    if (parm.aspect->answer)
		aspect[col] = (630 - aspect[col]) % 360;

	    /* assign some dead fuel moisture if not completely 
	     *     given based on emperical relationship*/
	    if (!(parm.mois_10h->answer || parm.mois_100h->answer)) {
		moisture[1] = moisture[0] + 0.01;
		moisture[2] = moisture[0] + 0.02;
	    }
	    if (!(parm.mois_1h->answer || parm.mois_100h->answer)) {
		moisture[0] = moisture[1] - 0.01;
		moisture[2] = moisture[1] + 0.01;
	    }
	    if (!(parm.mois_1h->answer || parm.mois_10h->answer)) {
		moisture[0] = moisture[2] - 0.02;
		moisture[1] = moisture[2] - 0.01;
	    }
	    if (!(parm.mois_1h->answer) && parm.mois_10h->answer &&
		parm.mois_100h->answer)
		moisture[0] = moisture[1] - 0.01;
	    if (!(parm.mois_10h->answer) && parm.mois_1h->answer &&
		parm.mois_100h->answer)
		moisture[1] = moisture[0] + 0.01;
	    if (!(parm.mois_100h->answer) && parm.mois_1h->answer &&
		parm.mois_10h->answer)
		moisture[2] = moisture[1] + 0.01;

	    /*compute xmext, moisture of extinction of live fuels */
	    wmfd = 0.0;
	    fined = 0.0;
	    if (SIGMA[3][fuel[col]] > 0.0) {
		for (class = 0; class <= 2; class++) {
		    if (SIGMA[class][fuel[col]] == 0.0)
			continue;
		    fined =
			fined +
			WO[class][fuel[col]] * exp(-138.0 /
						   SIGMA[class][fuel[col]]);
		    wmfd =
			wmfd +
			WO[class][fuel[col]] * exp(-138.0 /
						   SIGMA[class][fuel[col]]) *
			moisture[class];
		}
		fdmois = wmfd / fined;
		finel = WO[3][fuel[col]] * exp(-500.0 / SIGMA[3][fuel[col]]);
		xmext =
		    2.9 * (fined / finel) * (1 - fdmois / MX[fuel[col]]) -
		    0.226;
	    }
	    else
		xmext = MX[fuel[col]];
	    if (xmext < MX[fuel[col]])
		xmext = MX[fuel[col]];

	    /*compute other intermediate values */
	    Mf_dead = 0.0;
	    wn_dead = 0.0;
	    class_sum = 0.0;
	    for (class = 0; class <= 2; class++) {
		Mf_dead =
		    Mf_dead +
		    moisture[class] * Ffactor_in_dead[class][fuel[col]];
		wn_dead =
		    wn_dead +
		    WO[class][fuel[col]] * Gfactor_in_dead[class][fuel[col]] *
		    (1 - ST);
		Qig[class] = 250 + 1116 * moisture[class];
		class_sum =
		    class_sum +
		    Ffactor_all[class][fuel[col]] *
		    epsilon[class][fuel[col]] * Qig[class];
	    }
	    etaM_dead =
		1.0 - 2.59 * (Mf_dead / MX[fuel[col]]) +
		5.11 * (Mf_dead / MX[fuel[col]]) * (Mf_dead / MX[fuel[col]]) -
		3.52 * (Mf_dead / MX[fuel[col]]) * (Mf_dead / MX[fuel[col]]) *
		(Mf_dead / MX[fuel[col]]);
	    if (Mf_dead >= MX[fuel[col]])
		etaM_dead = 0.0;
	    etaM_live =
		1.0 - 2.59 * (moisture[3] / xmext) +
		5.11 * (moisture[3] / xmext) * (moisture[3] / xmext) -
		3.52 * (moisture[3] / xmext) * (moisture[3] / xmext) *
		(moisture[3] / xmext);
	    if (moisture[3] >= xmext)
		etaM_live = 0.0;
	    wn_live = WO[3][fuel[col]] * (1 - ST);
	    Qig[3] = 250 + 1116 * moisture[3];
	    class_sum =
		class_sum +
		Ffactor_all[3][fuel[col]] * epsilon[3][fuel[col]] * Qig[3];

	    /*final computations */
	    rhob = (wo_dead[fuel[col]] + WO[3][fuel[col]]) / DELTA[fuel[col]];
	    beta = rhob / rhop;
	    betaop = 3.348 / pow(sigma[fuel[col]], 0.8189);
	    A = 133 / pow(sigma[fuel[col]], 0.7913);
	    gammamax =
		pow(sigma[fuel[col]],
		    1.5) / (495 + 0.0594 * pow(sigma[fuel[col]], 1.5));
	    gamma =
		gammamax * pow(beta / betaop,
			       A) * exp(A * (1 - beta / betaop));
	    xi = exp((0.792 + 0.681 * pow(sigma[fuel[col]], 0.5)) * (beta +
								     0.1)) /
		(192 + 0.2595 * sigma[fuel[col]]);
	    IR = gamma * h * (wn_dead * etaM_dead +
			      wn_live * etaM_live) * etas;

	    R0 = IR * xi / (rhob * class_sum);

	    if (parm.vel->answer && parm.dir->answer) {
		C = 7.47 * exp(-0.133 * pow(sigma[fuel[col]], 0.55));
		B = 0.02526 * pow(sigma[fuel[col]], 0.54);
		E = 0.715 * exp(-0.000359 * sigma[fuel[col]]);
		phiw = C * pow((double)vel[col], B) / pow(beta / betaop, E);
	    }
	    else
		phiw = 0.0;

	    if (parm.slope->answer && parm.aspect->answer) {
		phis =
		    5.275 * pow(beta,
				-0.3) * tan(0.01745 * slope[col]) *
		    tan(0.01745 * slope[col]);
	    }
	    else
		phis = 0.0;

	    /*compute the maximum ROS R and its direction, 
	     *vector adding for the effect of wind and slope*/
	    if (parm.dir->answer && parm.aspect->answer) {
		sin_fac =
		    phiw * sin(0.01745 * dir[col]) +
		    phis * sin(0.01745 * aspect[col]);
		cos_fac =
		    phiw * cos(0.01745 * dir[col]) +
		    phis * cos(0.01745 * aspect[col]);
		phi_ws = sqrt(sin_fac * sin_fac + cos_fac * cos_fac);
		Rdir = atan2(sin_fac, cos_fac) / 0.01745;
	    }
	    else if (parm.dir->answer && !(parm.aspect->answer)) {
		phi_ws = phiw;
		Rdir = dir[col];
	    }
	    else if (!(parm.dir->answer) && parm.aspect->answer) {
		phi_ws = phis;
		Rdir = (float)aspect[col];
	    }
	    else {
		phi_ws = 0.0;
		Rdir = 0.0;
	    }
	    R = R0 * (1 + phi_ws);
	    if (Rdir < 0.0)
		Rdir = Rdir + 360;
	    /*printf("\nparm.dir->aanswer=%s, parm.aspect->aanswer=%s, phis=%f, phi_ws=%f, aspect[col]=%d,Rdir=%f",parm.dir->answer,parm.aspect->answer,phis,phi_ws,aspect[col],Rdir); */

	    /*maximum spotting distance */
	    if (spotting)
		spotdist[col] =
		    spot_dist(fuel[col], R, vel[col], Rdir, row, col);

	    /*to avoid 0 R, convert ft./min to cm/min */
	    R0 = 30.5 * R0;
	    R = 30.5 * R;
	    /*4debugging            R0 = R0/30.5/1.1; R = R/30.5/1.1; */

	    base[col] = (int)R0;
	    max[col] = (int)R;
	    maxdir[col] = (int)Rdir;
	    /*printf("(%d, %d)\nR0=%.2f, vel=%d, dir=%d, phiw=%.2f, s=%d, as=%d, phis=%.2f, R=%.1f, Rdir=%.0f\n", row, col, R0, vel[col], dir[col], phiw, slope[col], aspect[col], phis, R, Rdir); */
	}
	G_put_raster_row(base_fd, base, CELL_TYPE);
	G_put_raster_row(max_fd, max, CELL_TYPE);
	G_put_raster_row(maxdir_fd, maxdir, CELL_TYPE);
	if (spotting)
	    G_put_raster_row(spotdist_fd, spotdist, CELL_TYPE);
    }
    G_percent(row, nrows, 2);

    G_close_cell(fuel_fd);
    if (parm.mois_1h->answer)
	G_close_cell(mois_1h_fd);
    if (parm.mois_10h->answer)
	G_close_cell(mois_10h_fd);
    if (parm.mois_100h->answer)
	G_close_cell(mois_100h_fd);
    G_close_cell(mois_live_fd);
    if (parm.vel->answer)
	G_close_cell(vel_fd);
    if (parm.dir->answer)
	G_close_cell(dir_fd);
    if (parm.slope->answer)
	G_close_cell(slope_fd);
    if (parm.aspect->answer)
	G_close_cell(aspect_fd);
    G_close_cell(base_fd);
    G_close_cell(max_fd);
    G_close_cell(maxdir_fd);
    if (spotting) {
	G_close_cell(spotdist_fd);
	G_close_cell(spotdist_fd);
	G_free(map_elev);
    }

    /*      
       for (model = 1; model <= 13; model++) {
       if (model == 1)
       printf("\n           Grass and Grass-dominated\n");
       else if (model == 4)
       printf("             Chaparral and Shrubfields\n");
       else if (model == 8)
       printf("                   Timber Litter\n");
       else if (model == 11)
       printf("                   Logging Slash\n");
       printf("Model %2d   ", model);
       for (class = 0; class <= 3; class++) 
       printf("%4.0f/%.3f ", SIGMA[class][model], WO[class][model]);
       printf("  %.1f  %.2f\n", DELTA[model], MX[model]);
       } 

       printf("\nWeight in All Fuel Subclasses:\n");
       for (model = 1; model <= 13; model++) {
       printf("model %2d  ", model);
       for (class = 0; class <= 3; class++) 
       printf("%.2f  ", Ffactor_all[class][model]);
       printf("%4.0f/%.3f=%6.0f  model %2d\n", sigma[model], wo_dead[model]+WO[3][model], sigma[model]/(wo_dead[model]+WO[3][model]), model);
       } 
       printf("\nWeight in Dead Fuel Subclasses, Dead Weight/Live Weight:\n");for (model = 1; model <= 13; model++) {
       printf("model %2d  ", model);
       for (class = 0; class <= 2; class++) 
       printf("%.2f  ", Ffactor_in_dead[class][model]);
       printf("%.2f/%.2f  model %2d\n", Ffactor_dead[model], Ffactor_live[model], model);
       } 
     */

    exit(EXIT_SUCCESS);
}
