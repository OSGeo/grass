
/****************************************************************************
 *
 * MODULE:       r.spreadpath
 * AUTHOR(S):    Jianping Xu 1995: WIldfire SPread Simulation, WiSpS (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>, Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>, Jachym Cepicky <jachym les-ejk.cz>
 * PURPOSE:      This is the main program for tracing out the shortest path(s)
 *               based on the raster map showing back path cells from which the   
 *               cumulative costs were determined.
 * COPYRIGHT:    (C) 2000-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/**********************************************************************/
/*                                                                    */
/*      This is the main program for tracing out the shortest path(s) */
/*      based on the raster map showing back path cells from which the */
/*      cumulative costs were determined.                             */
/*                                                                    */

/**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <grass/segment.h>
#include <grass/gis.h>
#include <grass/raster.h>

#include "stash.h"
#include <grass/glocale.h>
#include "local_proto.h"


struct variables
{
    char *alias;
    int position;
} variables[] = {
    {"x_input", BACKCOL_LAYER},
    {"y_input", BACKROW_LAYER},
    {"coor", START_PT},
    {"output", PATH_LAYER}
};

char path_layer[64];
char backrow_layer[64];
char backcol_layer[64];
struct point *head_start_pt = NULL;

char *value;
int nrows, ncols;
SEGMENT in_row_seg, in_col_seg, out_seg;


int main(int argc, char **argv)
{
    int n, 
	backrow, backcol,
	col, row,
	len, flag,
	srows, scols,
	backrow_fd, backcol_fd, path_fd;
    const char *search_mapset,
	       *path_mapset,
	       *backrow_mapset,
	       *backcol_mapset;
    char *in_row_file, *in_col_file, *out_file;
    CELL *cell;
    POINT *PRES_PT, *PRESENT_PT, *OLD_PT;
    struct Cell_head window;
    double east, north;
    struct Option *opt1, *opt2, *opt3, *opt4;
    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("fire"));
    G_add_keyword(_("cumulative costs"));
    module->description =
	_("Recursively traces the least cost path backwards to "
	  "cells from which the cumulative cost was determined.");

    opt1 = G_define_standard_option(G_OPT_R_INPUT);
    opt1->key = "x_input";
    opt1->description =
	_("Name of raster map containing back-path easting information");

    opt2 = G_define_standard_option(G_OPT_R_INPUT);
    opt2->key = "y_input";
    opt2->description =
	_("Name of raster map containing back-path northing information");

    opt3 = G_define_standard_option(G_OPT_M_COORDS);
    opt3->multiple = YES;
    opt3->description =
	_("The map E and N grid coordinates of starting points");

    opt4 = G_define_standard_option(G_OPT_R_OUTPUT);

    /*   Do command line parsing    */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in_row_file = G_tempfile();
    in_col_file = G_tempfile();
    out_file = G_tempfile();

    /*  Get database window parameters      */
    G_get_window(&window);

    /*  Check if backrow layer exists in data base  */
    search_mapset = "";

    strcpy(backrow_layer, opt2->answer);
    strcpy(backcol_layer, opt1->answer);

    backrow_mapset = G_find_raster(backrow_layer, search_mapset);
    backcol_mapset = G_find_raster(backcol_layer, search_mapset);

    if (backrow_mapset == NULL)
	G_fatal_error("%s - not found", backrow_layer);

    if (backcol_mapset == NULL)
	G_fatal_error("%s - not found", backcol_layer);

    search_mapset = "";

    strcpy(path_layer, opt4->answer);

    path_mapset = G_find_raster(path_layer, search_mapset);

    /*  find number of rows and cols in window    */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    cell = Rast_allocate_c_buf();

    /*  Open back cell layers for reading  */
    backrow_fd = Rast_open_old(backrow_layer, backrow_mapset);
    backcol_fd = Rast_open_old(backcol_layer, backcol_mapset);

    /*   Parameters for map submatrices   */
    len = sizeof(CELL);

    /* TODO: improve segment handling */
    srows = nrows / 4 + 1;
    scols = ncols / 4 + 1;

    G_verbose_message(_("Reading the input map -%s- and -%s- and creating some temporary files..."),
	     backrow_layer, backcol_layer);

    /* Create segmented files for back cell and output layers  */
    Segment_open(&in_row_seg, in_row_file, nrows, ncols, srows, scols, len, 4);
    Segment_open(&in_col_seg, in_col_file, nrows, ncols, srows, scols, len, 4);

    Segment_open(&out_seg, out_file, nrows, ncols, srows, scols, len, 4);

    /*   Write the back cell layers in the segmented files, and  
     *   Change UTM coordinates to ROWs and COLUMNs */
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(backrow_fd, cell, row);

	for (col = 0; col < ncols; col++)
	    if (cell[col] > 0)
		cell[col] =
		    (window.north - cell[col]) / window.ns_res /* - 0.5 */ ;
	    else
		cell[col] = -1;
	Segment_put_row(&in_row_seg, cell, row);
	Rast_get_c_row(backcol_fd, cell, row);

	for (col = 0; col < ncols; col++)
	    if (cell[col] > 0)
		cell[col] =
		    (cell[col] - window.west) / window.ew_res /* - 0.5 */ ;
	Segment_put_row(&in_col_seg, cell, row);
    }

    /* Convert easting and northing from the command line to row and col */
    if (opt3->answer) {
	for (n = 0; opt3->answers[n] != NULL; n += 2) {
	    G_scan_easting(opt3->answers[n], &east, G_projection());
	    G_scan_northing(opt3->answers[n + 1], &north, G_projection());
	    row = (window.north - north) / window.ns_res;
	    col = (east - window.west) / window.ew_res;
	    /* ignore pt outside window */
	    if (east < window.west || east > window.east ||
		north < window.south || north > window.north) {
		G_warning("Ignoring point outside window: ");
		G_warning("   %.4f,%.4f", east, north);
		continue;
	    }

	    value = (char *)&backrow;
	    Segment_get(&in_row_seg, value, row, col);
	    /* ignore pt in no-data area */
	    if (backrow < 0) {
		G_warning("Ignoring point in NO-DATA area :");
		G_warning("   %.4f,%.4f", east, north);
		continue;
	    }
	    value = (char *)&backcol;
	    Segment_get(&in_col_seg, value, row, col);

	    insert(&PRESENT_PT, row, col, backrow, backcol);
	}
    }

    /*  Set flag according to input */
    if (path_mapset != NULL) {
	if (head_start_pt == NULL)
	    /*output layer exists and start pts are not given on cmd line */
	    flag = 1;

	/* output layer exists and starting pts are given on cmd line */
	else
	    flag = 2;
    }
    else
	flag = 3;		/* output layer does not previously exist */

    /* If the output layer containing the starting positions */
    /* create a linked list of of them  */
    if (flag == 1) {
	path_fd = Rast_open_old(path_layer, path_mapset);

	/*  Search for the marked starting pts and make list    */
	for (row = 0; row < nrows; row++) {
	    Rast_get_c_row(path_fd, cell, row);

	    for (col = 0; col < ncols; col++) {
		if (cell[col] > 0) {
		    value = (char *)&backrow;
		    Segment_get(&in_row_seg, value, row, col);
		    /* ignore pt in no-data area */
		    if (backrow < 0) {
			G_warning("Ignoring point in NO-DATA area:");
			G_warning("   %.4f,%.4f\n",
				  window.west + window.ew_res * (col + 0.5),
				  window.north - window.ns_res * (row + 0.5));
			continue;
		    }
		    value = (char *)&backcol;
		    Segment_get(&in_col_seg, value, row, col);
		    insert(&PRESENT_PT, row, col, backrow, backcol);
		}
	    }			/* loop over cols */
	}			/* loop over rows */

	Rast_close(path_fd);
    }

    /* loop over the starting points to find the least cost paths */
    G_verbose_message(_("Finding the least cost paths ..."));

    PRES_PT = head_start_pt;
    while (PRES_PT != NULL) {
	path_finder(PRES_PT->row, PRES_PT->col, PRES_PT->backrow,
		    PRES_PT->backcol);

	OLD_PT = PRES_PT;
	PRES_PT = NEXT_PT;
	G_free(OLD_PT);
    }

    /* Write pending updates by Segment_put() to outputmap */
    Segment_flush(&out_seg);

    G_verbose_message(_("Writing the output map  -%s-..."), path_layer);

    path_fd = Rast_open_c_new(path_layer);
    for (row = 0; row < nrows; row++) {
	Segment_get_row(&out_seg, cell, row);
	Rast_put_row(path_fd, cell, CELL_TYPE);
    }

    Segment_close(&in_row_seg);	/* release memory  */
    Segment_close(&in_col_seg);
    Segment_close(&out_seg);

    Rast_close(path_fd);
    Rast_close(backrow_fd);
    Rast_close(backcol_fd);

    exit(EXIT_SUCCESS);
}
