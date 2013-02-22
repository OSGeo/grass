
/****************************************************************************
 *
 * MODULE:       r.spread
 * AUTHOR(S):    Jianping Xu, 1995 (original contributor)
 *                 Center for Remote Sensing and Spatial Analysis (CRSSA)
 *                 Rutgers University.
 *               Andreas.Lange <andreas.lange rhein-main.de>, Eric G. Miller <egm2 jps.net>,
 *               Markus Neteler <neteler itc.it>, Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>
 * PURPOSE:      
 *      This is the main program for simulating elliptical spread.
 *                  
 *      It
 *      1) determines the earliest time a phenomenon REACHES to a
 *         map cell, NOT the time that cell is EXHAUSTED.
 *      3) If a cell is spread barrier, a no-data value is assigned
 *         to it.
 * COPYRIGHT:    (C) 2000-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "cmd_line.h"
#include "costHa.h"
#include "cell_ptrHa.h"
#include "local_proto.h"

#define DATA(map, r, c)		(map)[(r) * ncols + (c)]

CELL range_min, range_max;
CELL *cell;
CELL *x_cell;
CELL *y_cell;

CELL *map_max;
CELL *map_dir;
CELL *map_base;
CELL *map_spotdist;
CELL *map_velocity;
CELL *map_mois;
float *map_out;
CELL *map_x_out;
CELL *map_y_out;
CELL *map_visit;

char buf[400];

float zero = 0.0;
float neg = -2.0;

int BARRIER = 0;
int max_fd, dir_fd, base_fd, start_fd;
int spotdist_fd, velocity_fd, mois_fd;
int cum_fd, x_fd, y_fd;
int nrows, ncols;

long heap_len;

struct Cell_head window;
struct Range range;

struct costHa *heap;


int main(int argc, char *argv[])
{
    int col, row;

    struct
    {
	struct Option *max, *dir, *base, *start,
	    *spotdist, *velocity, *mois,
	    *least, *comp_dens, *init_time,
	    *time_lag, *backdrop, *out, *x_out, *y_out;
    } parm;
    struct
    {
	/* please, remove before GRASS 7 released */
	struct Flag *display, *spotting;
    } flag;
    struct GModule *module;

    /* initialize access to database and create temporary files */

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("fire"));
    module->label =
	_("Simulates elliptically anisotropic spread on a graphics window and "
	 "generates a raster map of the cumulative time of spread, "
	 "given raster maps containing the rates of spread (ROS), the ROS "
	 "directions and the spread origins.");
    module->description =
	_("It optionally produces raster maps to contain backlink UTM "
	  "coordinates for tracing spread paths.");

    parm.max = G_define_option();
    parm.max->key = "max";
    parm.max->type = TYPE_STRING;
    parm.max->required = YES;
    parm.max->gisprompt = "old,cell,raster";
    parm.max->guisection = _("Input maps");
    parm.max->description =
	_("Name of raster map containing MAX rate of spread (ROS) (cm/min)");

    parm.dir = G_define_option();
    parm.dir->key = "dir";
    parm.dir->type = TYPE_STRING;
    parm.dir->required = YES;
    parm.dir->gisprompt = "old,cell,raster";
    parm.dir->guisection = _("Input maps");
    parm.dir->description =
	_("Name of raster map containing DIRections of max ROS (degree)");

    parm.base = G_define_option();
    parm.base->key = "base";
    parm.base->type = TYPE_STRING;
    parm.base->required = YES;
    parm.base->gisprompt = "old,cell,raster";
    parm.base->guisection = _("Input maps");
    parm.base->description =
	_("Name of raster map containing BASE ROS (cm/min)");

    parm.start = G_define_option();
    parm.start->key = "start";
    parm.start->type = TYPE_STRING;
    parm.start->required = YES;
    parm.start->gisprompt = "old,cell,raster";
    parm.start->guisection = _("Input maps");
    parm.start->description =
	_("Name of raster map containing STARTing sources");

    parm.spotdist = G_define_option();
    parm.spotdist->key = "spot_dist";
    parm.spotdist->type = TYPE_STRING;
    parm.spotdist->gisprompt = "old,cell,raster";
    parm.spotdist->guisection = _("Input maps");
    parm.spotdist->description =
	_("Name of raster map containing max SPOTting DISTance (m) (required w/ -s)");

    parm.velocity = G_define_option();
    parm.velocity->key = "w_speed";
    parm.velocity->type = TYPE_STRING;
    parm.velocity->gisprompt = "old,cell,raster";
    parm.velocity->guisection = _("Input maps");
    parm.velocity->description =
	_("Name of raster map containing midflame Wind SPEED (ft/min) (required w/ -s)");

    parm.mois = G_define_option();
    parm.mois->key = "f_mois";
    parm.mois->type = TYPE_STRING;
    parm.mois->gisprompt = "old,cell,raster";
    parm.mois->guisection = _("Input maps");
    parm.mois->description =
	_("Name of raster map containing fine Fuel MOISture of the cell receiving a spotting firebrand (%) (required w/ -s)");

    parm.least = G_define_option();
    parm.least->key = "least_size";
    parm.least->type = TYPE_STRING;
    parm.least->key_desc = "odd int";
    parm.least->options = "3,5,7,9,11,13,15";
    parm.least->description =
	_("Basic sampling window SIZE needed to meet certain accuracy (3)");

    parm.comp_dens = G_define_option();
    parm.comp_dens->key = "comp_dens";
    parm.comp_dens->type = TYPE_STRING;
    parm.comp_dens->key_desc = "decimal";
    parm.comp_dens->description =
	_("Sampling DENSity for additional COMPutin (range: 0.0 - 1.0 (0.5))");

    parm.init_time = G_define_option();
    parm.init_time->key = "init_time";
    parm.init_time->type = TYPE_STRING;
    parm.init_time->key_desc = "int (>= 0)";
    parm.init_time->description =
	_("INITial TIME for current simulation (0) (min)");

    parm.time_lag = G_define_option();
    parm.time_lag->key = "lag";
    parm.time_lag->type = TYPE_STRING;
    parm.time_lag->key_desc = "int (>= 0)";
    parm.time_lag->description =
	_("Simulating time duration LAG (fill the region) (min)");

    parm.backdrop = G_define_option();
    parm.backdrop->key = "backdrop";
    parm.backdrop->type = TYPE_STRING;
    parm.backdrop->gisprompt = "old,cell,raster";
    parm.backdrop->description =
	_("Name of raster map as a display backdrop");

    parm.out = G_define_option();
    parm.out->key = "output";
    parm.out->type = TYPE_STRING;
    parm.out->required = YES;
    parm.out->gisprompt = "new,cell,raster";
    parm.out->guisection = _("Output maps");
    parm.out->description =
	_("Name of raster map to contain OUTPUT spread time (min)");

    parm.x_out = G_define_option();
    parm.x_out->key = "x_output";
    parm.x_out->type = TYPE_STRING;
    parm.x_out->gisprompt = "new,cell,raster";
    parm.x_out->guisection = _("Output maps");
    parm.x_out->description =
	_("Name of raster map to contain X_BACK coordinates");

    parm.y_out = G_define_option();
    parm.y_out->key = "y_output";
    parm.y_out->type = TYPE_STRING;
    parm.y_out->gisprompt = "new,cell,raster";
    parm.y_out->guisection = _("Output maps");
    parm.y_out->description =
	_("Name of raster map to contain Y_BACK coordinates");

    flag.display = G_define_flag();
    flag.display->key = 'd';
#if 0
    flag.display->description = _("DISPLAY 'live' spread process on screen");
#else
    flag.display->description = _("Live display - currently DISABLED");
#endif

    flag.spotting = G_define_flag();
    flag.spotting->key = 's';
    flag.spotting->description = _("For wildfires: consider SPOTTING effect");

    /*   Parse command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    srand(getpid());

    display = flag.display->answer;
#if 1
    if (display)
	G_fatal_error(_("The display feature is disabled"));
#endif
    spotting = flag.spotting->answer;

    max_layer = parm.max->answer;
    dir_layer = parm.dir->answer;
    base_layer = parm.base->answer;
    start_layer = parm.start->answer;
    backdrop_layer = parm.backdrop->answer;
    out_layer = parm.out->answer;
    if (parm.x_out->answer) {
	x_out = 1;
	x_out_layer = parm.x_out->answer;
    }
    if (parm.y_out->answer) {
	y_out = 1;
	y_out_layer = parm.y_out->answer;
    }
    if (spotting) {
	if (!
	    (parm.spotdist->answer && parm.velocity->answer &&
	     parm.mois->answer)) {
	    G_warning
		("SPOTTING DISTANCE, fuel MOISTURE, or wind VELOCITY map not given w/ -s");
	    G_usage();
	    exit(EXIT_FAILURE);
	}
	else {
	    spotdist_layer = parm.spotdist->answer;
	    velocity_layer = parm.velocity->answer;
	    mois_layer = parm.mois->answer;
	}
    }
    /*Check the given the least sampling size, assign the default if needed */
    if (parm.least->answer)
	least = atoi(parm.least->answer);
    else
	least = 3;
    /*Check the given computing density, assign the default if needed */
    if (parm.comp_dens->answer) {
	comp_dens = atof(parm.comp_dens->answer);
	if (comp_dens < 0.0 || comp_dens > 1.0) {
	    G_warning("Illegal computing density <%s>",
		      parm.comp_dens->answer);
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    }
    else {
	comp_dens = 0.5;
    }
    /*Check the given initial time and simulation time lag, assign the default if needed */
    if (parm.init_time->answer) {
	init_time = atoi(parm.init_time->answer);
	if (init_time < 0) {
	    G_warning("Illegal initial time <%s>", parm.init_time->answer);
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    }
    else {
	time_lag = 0;
    }
    if (parm.time_lag->answer) {
	time_lag = atoi(parm.time_lag->answer);
	if (time_lag < 0) {
	    G_warning("Illegal simulating time lag <%s>",
		      parm.time_lag->answer);
	    G_usage();
	    exit(EXIT_FAILURE);
	}
    }
    else {
	time_lag = 99999;
    }

    /*  Get database window parameters  */

    G_get_window(&window);

    /*  find number of rows and columns in window    */

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /*transfor measurement unit from meters to centimeters due to ROS unit
     *if the input ROSs are in m/min units, cancell the following*/
    window.ns_res = 100 * window.ns_res;
    window.ew_res = 100 * window.ew_res;

    /* Initialize display screens */
#if 0
    if (display)
	display_init();
#endif

    /*  Check if input layers exists in data base  */

    if (G_find_raster2(max_layer, "") == NULL)
	G_fatal_error("Raster map <%s> not found", max_layer);

    if (G_find_raster2(dir_layer, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), dir_layer);

    if (G_find_raster2(base_layer, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), base_layer);

    if (G_find_raster2(start_layer, "") == NULL)
	G_fatal_error(_("Raster map <%s> not found"), start_layer);

    if (spotting) {
	if (G_find_raster2(spotdist_layer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), spotdist_layer);

	if (G_find_raster2(velocity_layer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), velocity_layer);

	if (G_find_raster2(mois_layer, "") == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), mois_layer);
    }

    /*  Open input cell layers for reading  */

    max_fd = Rast_open_old(max_layer, G_find_raster2(max_layer, ""));

    dir_fd = Rast_open_old(dir_layer, G_find_raster2(dir_layer, ""));

    base_fd = Rast_open_old(base_layer, G_find_raster2(base_layer, ""));

    if (spotting) {
	spotdist_fd =
	    Rast_open_old(spotdist_layer, G_find_raster2(spotdist_layer, ""));

	velocity_fd =
	    Rast_open_old(velocity_layer, G_find_raster2(velocity_layer, ""));

	mois_fd = Rast_open_old(mois_layer, G_find_raster2(mois_layer, ""));
    }

    /*  Allocate memories for a row  */
    cell = Rast_allocate_c_buf();
    if (x_out)
	x_cell = Rast_allocate_c_buf();
    if (y_out)
	y_cell = Rast_allocate_c_buf();

    /*  Allocate memories for a map  */
    map_max = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    map_dir = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    map_base = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    map_visit = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    map_out = (float *)G_calloc(nrows * ncols + 1, sizeof(float));
    if (spotting) {
	map_spotdist = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
	map_velocity = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
	map_mois = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    }
    if (x_out)
	map_x_out = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));
    if (y_out)
	map_y_out = (CELL *) G_calloc(nrows * ncols + 1, sizeof(CELL));


    /*   Write the input layers in the map "arrays"  */

    G_message(_("Reading inputs..."));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_c_row(max_fd, cell, row);
	for (col = 0; col < ncols; col++)
	    DATA(map_max, row, col) = cell[col];
	Rast_get_c_row(dir_fd, cell, row);
	for (col = 0; col < ncols; col++)
	    DATA(map_dir, row, col) = cell[col];
	Rast_get_c_row(base_fd, cell, row);
	for (col = 0; col < ncols; col++)
	    DATA(map_base, row, col) = cell[col];
	if (spotting) {
	    Rast_get_c_row(spotdist_fd, cell, row);
	    for (col = 0; col < ncols; col++)
		DATA(map_spotdist, row, col) = cell[col];
	    Rast_get_c_row(velocity_fd, cell, row);
	    for (col = 0; col < ncols; col++)
		DATA(map_velocity, row, col) = cell[col];
	    Rast_get_c_row(mois_fd, cell, row);
	    for (col = 0; col < ncols; col++)
		DATA(map_mois, row, col) = cell[col];
	}
    }
    G_percent(row, nrows, 2);


    /*   Scan the START layer searching for starting points.
     *   Create an array of starting points (min_heap) ordered by costs.
     */

    start_fd = Rast_open_old(start_layer, G_find_raster2(start_layer, ""));

    Rast_read_range(start_layer, G_find_file("cell", start_layer, ""), &range);
    Rast_get_range_min_max(&range, &range_min, &range_max);

    /*  Initialize the heap  */
    heap =
	(struct costHa *)G_calloc(nrows * ncols + 1, sizeof(struct costHa));
    heap_len = 0;

    G_message(_("Reading %s..."), start_layer);
    G_debug(1, "Collecting origins...");
    collect_ori(start_fd);
    G_debug(1, "Done");


    /* Major computation of spread time */
    G_debug(1, "Spreading...");
    spread();
    G_debug(1, "Done");


    /*  Open cumulative cost layer (and x, y direction layers) for writing */

    cum_fd = Rast_open_c_new(out_layer);
    if (x_out)
	x_fd = Rast_open_c_new(x_out_layer);
    if (y_out)
	y_fd = Rast_open_c_new(y_out_layer);

    /* prepare output -- adjust from cm to m */
    window.ew_res = window.ew_res / 100;
    window.ns_res = window.ns_res / 100;

    /* copy maps in ram to output maps */
    ram2out();

    G_free(map_max);
    G_free(map_dir);
    G_free(map_base);
    G_free(map_out);
    G_free(map_visit);
    if (x_out)
	G_free(map_x_out);
    if (y_out)
	G_free(map_y_out);
    if (spotting) {
	G_free(map_spotdist);
	G_free(map_mois);
	G_free(map_velocity);
    }

    Rast_close(max_fd);
    Rast_close(dir_fd);
    Rast_close(base_fd);
    Rast_close(start_fd);
    Rast_close(cum_fd);
    if (x_out)
	Rast_close(x_fd);
    if (y_out)
	Rast_close(y_fd);
    if (spotting) {
	Rast_close(spotdist_fd);
	Rast_close(velocity_fd);
	Rast_close(mois_fd);
    }

    /* close graphics */
#if 0
    if (display)
	display_close();
#endif

    exit(EXIT_SUCCESS);
}
