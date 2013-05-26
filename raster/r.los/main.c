
/****************************************************************************
 *
 * MODULE:       r.los
 * AUTHOR(S):    Kewan Q. Khawaja, Intelligent Engineering Systems Laboratory, M.I.T. (original contributor)
 *               Markus Neteler <neteler itc.it> (original contributor)
 *               Ron Yorston <rmy tigress co uk>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      line of sight
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/****************************************************************
 *      main.c
 *
 *      This is the main program for line-of-sight analysis.
 *      It takes a digital elevation map and identifies all
 *      the grid cells that are visible from a user specified
 *      observer location.  Other input parameters to the
 *      program include the height of the observer above the
 *      ground, a map marking areas of interest in which the
 *      analysis is desired, and a maximum range for the line
 *      of sight.
 *
 ****************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/raster.h>

#include <grass/segment.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "cmd_line.h"
#include "point.h"
#include "local_proto.h"

#define   COLOR_SHIFT      155.0
#define   COLOR_MAX        255.0

#define   SEARCH_PT_INCLINATION		SEARCH_PT->inclination
#define	  NEXT_SEARCH_PT		SEARCH_PT->next

struct Cell_head window;	/*      database window         */

struct point *DELAYED_DELETE;

double east;
double north;
double obs_elev;
double max_dist;
char *elev_layer;
char *patt_layer;
char *out_layer;

int main(int argc, char *argv[])
{
    int row_viewpt, col_viewpt, nrows, ncols, a, b, row, patt_flag;
    int segment_no, flip, xmax, ymax, sign_on_y, sign_on_x;
    int submatrix_rows, submatrix_cols, lenth_data_item;
    int patt = 0, in_fd, out_fd, patt_fd = 0;
    int old, new;
    double slope_1, slope_2, max_vert_angle = 0.0, color_factor;
    const char *old_mapset, *patt_mapset = NULL;
    FCELL *value;
    const char *search_mapset, *current_mapset;
    const char *in_name, *out_name, *patt_name = NULL;
    struct Categories cats;
    struct Cell_head cellhd_elev, cellhd_patt;
    extern struct Cell_head window;
    CELL *cell;
    FCELL *fcell, data, viewpt_elev;
    SEGMENT seg_in, seg_out, seg_patt;
    struct point *heads[16], *SEARCH_PT;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt5, *opt6, *opt7;
    struct Flag *curvature;
    struct History history;
    char title[128];
    double aa, e2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("viewshed"));
    G_add_keyword(_("line of sight"));
    module->description = _("Line-of-sight raster analysis program.");

    /* Define the different options */

    opt1 = G_define_standard_option(G_OPT_R_ELEV);
    opt1->key = "input";

    opt7 = G_define_standard_option(G_OPT_R_OUTPUT);

    opt3 = G_define_option();
    opt3->key = "coordinate";
    opt3->type = TYPE_STRING;
    opt3->required = YES;
    opt3->key_desc = "x,y";
    opt3->description = _("Coordinate identifying the viewing position");

    opt2 = G_define_standard_option(G_OPT_R_COVER);
    opt2->key = "patt_map";
    opt2->required = NO;
    opt2->description = _("Binary (1/0) raster map to use as a mask");

    opt5 = G_define_option();
    opt5->key = "obs_elev";
    opt5->type = TYPE_DOUBLE;
    opt5->required = NO;
    opt5->answer = "1.75";
    opt5->description = _("Viewing position height above the ground");

    opt6 = G_define_option();
    opt6->key = "max_dist";
    opt6->type = TYPE_DOUBLE;
    opt6->required = NO;
    opt6->answer = "10000";
    opt6->options = "0-5000000";	/* observer can be in a plane, too */
    opt6->description = _("Maximum distance from the viewing point (meters)");
    /* http://mintaka.sdsu.edu/GF/explain/atmos_refr/horizon.html */

    curvature = G_define_flag();
    curvature->key = 'c';
    curvature->description =
	_("Consider earth curvature (current ellipsoid)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* initialize delayed point deletion */
    DELAYED_DELETE =  NULL;

    G_scan_easting(opt3->answers[0], &east, G_projection());
    G_scan_northing(opt3->answers[1], &north, G_projection());

    sscanf(opt5->answer, "%lf", &obs_elev);
    sscanf(opt6->answer, "%lf", &max_dist);
    elev_layer = opt1->answer;
    patt_layer = opt2->answer;
    out_layer = opt7->answer;

    G_get_window(&window);

    current_mapset = G_mapset();
    /* set flag to indicate presence of areas of interest   */
    if (patt_layer == NULL)
	patt_flag = FALSE;
    else
	patt_flag = TRUE;

    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/Long support is not (yet) implemented for this module."));

    /* check if specified observer location inside window   */
    if (east < window.west || east > window.east
	|| north > window.north || north < window.south)
	G_fatal_error(_("Specified observer coordinate is outside current region bounds."));

    search_mapset = "";
    old_mapset = G_find_raster2(elev_layer, search_mapset);

    /*  check if elevation layer present in database    */
    if (old_mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), elev_layer);

    /* if pattern layer used, check if present in database  */
    if (patt_flag == TRUE) {
	patt_mapset = G_find_raster(patt_layer, search_mapset);
	if (patt_mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), patt_layer);
    }

    /*  read header info for elevation layer        */
    Rast_get_cellhd(elev_layer, old_mapset, &cellhd_elev);

    /*  if pattern layer present, read in its header info   */
    if (patt_flag == TRUE) {
	Rast_get_cellhd(patt_layer, patt_mapset, &cellhd_patt);

	/*  allocate buffer space for row-io to layer           */
	cell = Rast_allocate_buf(CELL_TYPE);
    }

    /*  allocate buffer space for row-io to layer           */
    fcell = Rast_allocate_buf(FCELL_TYPE);

    /*  find number of rows and columns in elevation map    */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /*      open elevation overlay file for reading         */
    old = Rast_open_old(elev_layer, old_mapset);

    /*      open cell layer for writing output              */
    new = Rast_open_new(out_layer, FCELL_TYPE);

    /* if pattern layer specified, open it for reading      */
    if (patt_flag == TRUE) {
	patt = Rast_open_old(patt_layer, patt_mapset);
	if (Rast_get_map_type(patt) != CELL_TYPE)
	    G_fatal_error(_("Pattern map should be a binary 0/1 CELL map"));
    }

    /*      parameters for map submatrices                  */
    lenth_data_item = sizeof(FCELL);
    submatrix_rows = nrows / 4 + 1;
    submatrix_cols = ncols / 4 + 1;

    /* create segmented format files for elevation layer,   */
    /* output layer and pattern layer (if present)          */
    in_name = G_tempfile();
    in_fd = creat(in_name, 0666);
    segment_format(in_fd, nrows, ncols,
		   submatrix_rows, submatrix_cols, lenth_data_item);
    close(in_fd);
    out_name = G_tempfile();
    out_fd = creat(out_name, 0666);
    segment_format(out_fd, nrows, ncols,
		   submatrix_rows, submatrix_cols, lenth_data_item);
    close(out_fd);

    if (patt_flag == TRUE) {
	patt_name = G_tempfile();
	patt_fd = creat(patt_name, 0666);
	segment_format(patt_fd, nrows, ncols,
		       submatrix_rows, submatrix_cols, sizeof(CELL));
	close(patt_fd);
    }

    if (curvature->answer) {
	/* try to get the radius the standard GRASS way from the libs */
	G_get_ellipsoid_parameters(&aa, &e2);
	if (aa == 0) {
	    /* since there was a problem, take a hardcoded radius :( */
	    G_warning(_("Problem to obtain current ellipsoid parameters, using sphere (6370997.0)"));
	    aa = 6370997.00;
	}
	G_debug(3, "radius: %f", aa);
    }
    G_message(_("Using maximum distance from the viewing point (meters): %f"),
	      max_dist);

    /*      open, initialize and segment all files          */
    in_fd = open(in_name, 2);
    segment_init(&seg_in, in_fd, 4);
    out_fd = open(out_name, 2);
    segment_init(&seg_out, out_fd, 4);

    if (patt_flag == TRUE) {
	patt_fd = open(patt_name, 2);
	segment_init(&seg_patt, patt_fd, 4);
	for (row = 0; row < nrows; row++) {
	    Rast_get_row(patt, cell, row, CELL_TYPE);
	    segment_put_row(&seg_patt, cell, row);
	}
    }

    for (row = 0; row < nrows; row++) {
	Rast_get_row(old, fcell, row, FCELL_TYPE);
	segment_put_row(&seg_in, fcell, row);
    }

    /* calc map array coordinates for viewing point         */
    row_viewpt = (window.north - north) / window.ns_res;
    col_viewpt = (east - window.west) / window.ew_res;

    /*       read elevation of viewing point                */
    value = &viewpt_elev;
    segment_get(&seg_in, value, row_viewpt, col_viewpt);
    viewpt_elev += obs_elev;

    /*      DO LOS ANALYSIS FOR SIXTEEN SEGMENTS            */
    for (segment_no = 1; segment_no <= 16; segment_no++) {
	sign_on_y = 1 - (segment_no - 1) / 8 * 2;

	if (segment_no > 4 && segment_no < 13)
	    sign_on_x = -1;
	else
	    sign_on_x = 1;

	/*      calc slopes for bounding rays of a segment      */
	if (segment_no == 1 || segment_no == 4 || segment_no == 5 ||
	    segment_no == 8 || segment_no == 9 || segment_no == 12 ||
	    segment_no == 13 || segment_no == 16) {
	    slope_1 = 0.0;
	    slope_2 = 0.5;
	}
	else {
	    slope_1 = 0.5;
	    slope_2 = 1.0;
	}

	if (segment_no == 1 || segment_no == 2 || segment_no == 7 ||
	    segment_no == 8 || segment_no == 9 || segment_no == 10 ||
	    segment_no == 15 || segment_no == 16)
	    flip = 0;
	else
	    flip = 1;

	/*      calculate max and min 'x' and 'y'               */
	a = ((ncols - 1) * (sign_on_x + 1) / 2 - sign_on_x * col_viewpt);
	b = (1 - sign_on_y) / 2 * (nrows - 1) + sign_on_y * row_viewpt;

	if (flip == 0) {
	    xmax = a;
	    ymax = b;
	}
	else {
	    xmax = b;
	    ymax = a;
	}

	/*      perform analysis for every segment              */
	heads[segment_no - 1] = segment(segment_no, xmax, ymax,
					slope_1, slope_2, flip, sign_on_y,
					sign_on_x, viewpt_elev, &seg_in,
					&seg_out, &seg_patt, row_viewpt,
					col_viewpt, patt_flag,
					curvature->answer, aa);

	G_percent(segment_no, 16, 5);
    }				/*      end of for-loop over segments           */

    /* loop over all segment lists to find maximum vertical */
    /* angle of any point when viewed from observer location */
    for (segment_no = 1; segment_no <= 16; segment_no++) {
	SEARCH_PT = heads[segment_no - 1];
	while (SEARCH_PT != NULL) {
	    if (fabs(SEARCH_PT_INCLINATION) > max_vert_angle)
		max_vert_angle = fabs(SEARCH_PT_INCLINATION);
	    SEARCH_PT = NEXT_SEARCH_PT;
	}
    }

    /* calculate factor to be multiplied to every vertical  */
    /* angle for suitable color variation on output map     */
    /*    color_factor = decide_color_range(max_vert_angle * 57.3,
       COLOR_SHIFT, COLOR_MAX);   */
    color_factor = 1.0;		/* to give true angle? */

    /* mark visible points for all segments on outputmap    */
    for (segment_no = 1; segment_no <= 16; segment_no++) {
	mark_visible_points(heads[segment_no - 1], &seg_out,
			    row_viewpt, col_viewpt, color_factor,
			    COLOR_SHIFT);
    }

    /*      mark viewpt on output map                       */
    data = 180;
    value = &data;
    segment_put(&seg_out, value, row_viewpt, col_viewpt);

    /* write pending updates by segment_put() to outputmap  */
    segment_flush(&seg_out);

    /* convert output submatrices to full cell overlay      */
    for (row = 0; row < nrows; row++) {
	int col;

	segment_get_row(&seg_out, fcell, row);
	for (col = 0; col < ncols; col++)
	    /* set to NULL if beyond max_dist (0) or blocked view (1) */
	    if (fcell[col] == 0 || fcell[col] == 1)
		Rast_set_null_value(&fcell[col], 1, FCELL_TYPE);
	Rast_put_row(new, fcell, FCELL_TYPE);
    }

    segment_release(&seg_in);	/* release memory       */
    segment_release(&seg_out);

    if (patt_flag == TRUE)
	segment_release(&seg_patt);

    close(in_fd);		/* close all files      */
    close(out_fd);
    unlink(in_name);		/* remove temp files as well */
    unlink(out_name);
    Rast_close(old);
    Rast_close(new);

    if (patt_flag == TRUE) {
	close(patt_fd);
	Rast_close(patt);
    }

    /*      create category file for output map             */
    Rast_read_cats(out_layer, current_mapset, &cats);
    Rast_set_cats_fmt("$1 degree$?s", 1.0, 0.0, 0.0, 0.0, &cats);
    Rast_write_cats(out_layer, &cats);

    sprintf(title, "Line of sight %.2fm above %s", obs_elev, opt3->answer);
    Rast_put_cell_title(out_layer, title);
    Rast_write_units(out_layer, "degrees");

    Rast_short_history(out_layer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out_layer, &history);

    /* release that last tiny bit of memory ... */
    if ( DELAYED_DELETE != NULL ) {
        G_free ( DELAYED_DELETE );
    }

    exit(EXIT_SUCCESS);
}
