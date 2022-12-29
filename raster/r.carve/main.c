
/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "enforce.h"


/*
   Note: Use rast input type for rast output 
   Read vect file, 
   for each line,
   use a shadow line struct to represent stream profile,
   where x is distance along stream and y is elevation,
   adding each point to lobf as it's created.
   find trend using lobf
   from high to low, lower any points forming dams
   when next pnt elev increases,
   find next point <= than last confirmed pt
   just use linear interp for now
   write line to new raster  
   Use orig line struct for XYs, shadow struct Y for cell val
   if new raster already has value there, use lower?
   actually probably want to use val for trunk stream 
   and then verify branch in reverse
   - for now maybe create a conflict map
   After that's working, add width to lines? 
   or use r.grow
 */


/* function prototypes */
static int init_projection(struct Cell_head *window, int *wrap_ncols);


int main(int argc, char **argv)
{
    struct GModule *module;
    struct parms parm;
    struct Option *width, *depth;
    struct Flag *noflat;

    const char *vmapset, *rmapset;
    int infd, outfd;
    struct Map_info Map;
    struct Map_info outMap;
    struct Cell_head win;

    /* start GIS engine */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->label = _("Generates stream channels.");
    module->description = _("Takes vector stream data, transforms it "
			    "to raster and subtracts depth from the output DEM.");

    parm.inrast = G_define_standard_option(G_OPT_R_INPUT);
    parm.inrast->key = "raster";
    parm.inrast->description = _("Name of input raster elevation map");

    parm.invect = G_define_standard_option(G_OPT_V_INPUT);
    parm.invect->key = "vector";
    parm.invect->label =
	_("Name of input vector map containing stream(s)");

    parm.outrast = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.outvect = G_define_standard_option(G_OPT_V_OUTPUT);
    parm.outvect->key = "points";
    parm.outvect->required = NO;
    parm.outvect->description =
	_("Name for output vector map for adjusted stream points");

    width = G_define_option();
    width->key = "width";
    width->type = TYPE_DOUBLE;
    width->label = _("Stream width (in meters)");
    width->description = _("Default is raster cell width");

    depth = G_define_option();
    depth->key = "depth";
    depth->type = TYPE_DOUBLE;
    depth->description = _("Additional stream depth (in meters)");

    noflat = G_define_flag();
    noflat->key = 'n';
    noflat->description = _("No flat areas allowed in flow direction");

    /* parse options */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_check_input_output_name(parm.inrast->answer, parm.outrast->answer,
			      G_FATAL_EXIT);
    if (parm.outvect->answer)
      Vect_check_input_output_name(parm.invect->answer, parm.outvect->answer,
				   G_FATAL_EXIT);

    /* setup lat/lon projection and distance calculations */
    init_projection(&win, &parm.wrap);

    /* default width - one cell at center */
    if (width->answer == NULL) {
	parm.swidth = G_distance((win.east + win.west) / 2,
				 (win.north + win.south) / 2,
				 ((win.east + win.west) / 2) + win.ew_res,
				 (win.north + win.south) / 2);
    }
    else {
	if (sscanf(width->answer, "%lf", &parm.swidth) != 1) {
	    G_warning(_("Invalid width value '%s' - using default."),
		      width->answer);
	    parm.swidth =
		G_distance((win.east + win.west) / 2,
			   (win.north + win.south) / 2,
			   ((win.east + win.west) / 2) + win.ew_res,
			   (win.north + win.south) / 2);
	}
    }

    if (depth->answer == NULL)
	parm.sdepth = 0.0;
    else {
	if (sscanf(depth->answer, "%lf", &parm.sdepth) != 1) {
	    G_warning(_("Invalid depth value '%s' - using default."),
		      depth->answer);
	    parm.sdepth = 0.0;
	}
    }

    parm.noflat = noflat->answer;

    /* open input files */
    if ((vmapset = G_find_vector2(parm.invect->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), parm.invect->answer);

    Vect_set_open_level(2);
    if (Vect_open_old(&Map, parm.invect->answer, vmapset) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), parm.invect->answer);

    if ((rmapset = G_find_file2("cell", parm.inrast->answer, "")) == NULL)
	G_fatal_error(_("Raster map <%s> not found"), parm.inrast->answer);

    infd = Rast_open_old(parm.inrast->answer, rmapset);

    parm.raster_type = Rast_get_map_type(infd);

    /* open new map for output */
    outfd = Rast_open_new(parm.outrast->answer, parm.raster_type);

    /* if specified, open vector for output */
    if (parm.outvect->answer)
	open_new_vect(&outMap, parm.outvect->answer);

    enforce_downstream(infd, outfd, &Map, &outMap, &parm);

    Rast_close(infd);
    Rast_close(outfd);
    close_vect(&Map, 0);

    if (parm.outvect->answer)
	close_vect(&outMap, 1);

    /* write command line to history file */
    update_rast_history(&parm);

    return EXIT_SUCCESS;
}


static int init_projection(struct Cell_head *window, int *wrap_ncols)
{
#if 0
    double a, e2;
#endif

    G_get_set_window(window);

    if (((window->west == (window->east - 360.0)) ||
	 (window->east == (window->west - 360.0))) &&
	(G_projection() == PROJECTION_LL)) {
#if 0
	G_get_ellipsoid_parameters(&a, &e2);
	G_begin_geodesic_distance(a, e2);

	/* add 1.1, not 1.0, to ensure that we round up */
	*wrap_ncols =
	    (360.0 - (window->east - window->west)) / window->ew_res + 1.1;
#else
	G_fatal_error(_("Lat/Long location is not supported by %s. Please reproject map first."),
		      G_program_name());
#endif
    }
    else {
	*wrap_ncols = 0;
    }

    G_begin_distance_calculations();

    return 0;
}
