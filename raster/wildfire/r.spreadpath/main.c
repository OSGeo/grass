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
/*      based on the raster map showing back path cells from which the*/ 
/*	cumulative costs were determined.			      */
/*                                                                    */
/**********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <grass/segment.h>
#include <grass/gis.h>
#define MAIN
#include "stash.h"
#include <grass/glocale.h>
#include "local_proto.h"


char *value;
int nrows, ncols;
SEGMENT in_row_seg, in_col_seg, out_seg;


int main(int argc, char **argv)
{
	int n, verbose = 1,
	backrow, backcol,
	col, row, 
	len, flag,
	srows, scols,
	backrow_fd, backcol_fd, path_fd,
	in_row_fd, in_col_fd, out_fd;
	char *current_mapset,
	*search_mapset,
	*path_mapset, 
	*backrow_mapset,
	*backcol_mapset,
	*in_row_file, *in_col_file, *out_file;
	CELL *cell;
	POINT *PRES_PT, *PRESENT_PT, *OLD_PT;
	struct Cell_head window;
	double east, north;
	struct Option *opt1, *opt2, *opt3, *opt4;
	struct Flag *flag1;
	struct GModule *module;

	G_gisinit (argv[0]);
	
	/* Set description */
	module              = G_define_module();
	module->keywords = _("raster");
	module->description =
	_("Recursively traces the least cost path backwards to "
	"cells from which the cumulative cost was determined.");

	opt1 = G_define_option() ;
	opt1->key        = "x_input" ;
	opt1->type       = TYPE_STRING ;
	opt1->required   = YES ;
	opt1->gisprompt  = "old,cell,raster" ;
	opt1->description= _("Name of raster map containing back-path easting information");

	opt2 = G_define_option() ;
	opt2->key        = "y_input" ;
	opt2->type       = TYPE_STRING ;
	opt2->required   = YES ;
	opt2->gisprompt  = "old,cell,raster" ;
	opt2->description= _("Name of raster map containing back-path norhting information");

	opt3 = G_define_option() ;
	opt3->key        = "coordinate" ;
	opt3->type       = TYPE_STRING ;
	opt3->multiple   = YES;
	opt3->key_desc   = "x,y" ;
	opt3->description= _("The map E and N grid coordinates of starting points");

	opt4 = G_define_option() ;
	opt4->key        = "output" ;
	opt4->type       = TYPE_STRING ;
	opt4->required   = YES ;
	opt4->gisprompt  = "new,cell,raster" ;
	opt4->description= _("Name of spread path raster map") ;

     	flag1 = G_define_flag();
    	flag1->key = 'v';
     	flag1->description = _("Run verbosly");

	/*   Do command line parsing	*/
	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	current_mapset = G_mapset();
	in_row_file = G_tempfile();
	in_col_file = G_tempfile();
	out_file = G_tempfile();

	/*  Get database window parameters      */
	if(G_get_window (&window) < 0)
		G_fatal_error("can't read current window parameters");
	
        verbose = flag1->answer;

	/*  Check if backrow layer exists in data base  */
	search_mapset = "";

	strcpy(backrow_layer, opt2->answer);
	strcpy(backcol_layer, opt1->answer);

	backrow_mapset = G_find_cell 
	    (backrow_layer, search_mapset);
	backcol_mapset = G_find_cell 
	    (backcol_layer, search_mapset);

	if (backrow_mapset == NULL)
		G_fatal_error("%s - not found", backrow_layer);

	if (backcol_mapset == NULL)
		G_fatal_error("%s - not found", backcol_layer);

	search_mapset = "";

	strcpy (path_layer, opt4->answer);

	path_mapset = G_find_cell (path_layer, search_mapset);

	/*  find number of rows and cols in window    */
	nrows = G_window_rows();
	ncols = G_window_cols();

	cell = G_allocate_cell_buf();

	/*  Open back cell layers for reading  */
	backrow_fd = G_open_cell_old(backrow_layer, backrow_mapset);
	if (backrow_fd < 0)
		G_fatal_error("%s - can't open raster map", backrow_layer);

	backcol_fd = G_open_cell_old(backcol_layer, backcol_mapset);
	if (backcol_fd < 0)
		G_fatal_error("%s - can't open raster map", backcol_layer);

	/*   Parameters for map submatrices   */
	len = sizeof(CELL);

	srows = nrows/4 + 1;
	scols = ncols/4 + 1;

	if (verbose)
                G_message("\nReading the input map -%s- and -%s- and creating some temporary files...",
				backrow_layer, backcol_layer);

	/* Create segmented files for back cell and output layers  */
	in_row_fd = creat(in_row_file,0666);
	segment_format(in_row_fd, nrows, ncols, srows, scols, len);
	close(in_row_fd);
	in_col_fd = creat(in_col_file,0666);
	segment_format(in_col_fd, nrows, ncols, srows, scols, len);
	close(in_col_fd);

	out_fd = creat(out_file,0666);
	segment_format(out_fd, nrows, ncols, srows, scols, len);
	close(out_fd);

	/*   Open initialize and segment all files  */
	in_row_fd = open(in_row_file,2);
	segment_init(&in_row_seg,in_row_fd,4);
	in_col_fd = open(in_col_file,2);
	segment_init(&in_col_seg,in_col_fd,4);

	out_fd = open(out_file,2);
	segment_init(&out_seg,out_fd,4);

	/*   Write the back cell layers in the segmented files, and  
	 *   Change UTM coordinates to ROWs and COLUMNs */
	for( row=0 ; row<nrows ; row++ )
	{
		if( G_get_map_row(backrow_fd, cell, row)<0)
			G_fatal_error("unable to get map row %d", row);

		for (col=0; col<ncols; col++)
			if (cell[col] > 0)
				cell[col] = (window.north - cell[col])/window.ns_res/* - 0.5*/;
			else cell[col] = -1;
		segment_put_row(&in_row_seg, cell, row);
		if( G_get_map_row(backcol_fd, cell, row)<0)
			G_fatal_error("unable to get map row %d", row);

		for (col=0; col<ncols; col++)
			if (cell[col] > 0)
				cell[col] = (cell[col] - window.west)/window.ew_res/* - 0.5*/;
		segment_put_row(&in_col_seg, cell, row);
	}
	
	/* Convert easting and northing from the command line to row and col */
	if (opt3->answer) {
		for(n=0; opt3->answers[n] != NULL; n+=2) {
			G_scan_easting  (opt3->answers[n  ], &east, G_projection()) ;
			G_scan_northing (opt3->answers[n+1], &north, G_projection()) ;
			row = (window.north - north) / window.ns_res;
			col = (east - window.west) / window.ew_res;
			/* ignore pt outside window */
          		if(east < window.west || east > window.east ||
             			north < window.south || north > window.north) {
               			G_warning("Ignoring point outside window: ") ;
               			G_warning("   %.4f,%.4f", east, north) ;
               			continue ;
          		}

			value = (char *) &backrow;
			segment_get (&in_row_seg, value, row, col); 
			/* ignore pt in no-data area */
         		if(backrow < 0) {
               			G_warning("Ignoring point in NO-DATA area :") ;
               			G_warning("   %.4f,%.4f", east, north) ;
               			continue ;
          		}
			value = (char *)&backcol;
			segment_get (&in_col_seg, value, row, col);

			insert (&PRESENT_PT, row, col, backrow, backcol);
		}
	}

	/*  Set flag according to input */
	if (path_mapset != NULL)
	{
		if (head_start_pt == NULL)
			/*output layer exists and start pts are not given on cmd line*/
			flag = 1;

			/* output layer exists and starting pts are given on cmd line*/
		else flag = 2;
	}
	else
		flag = 3;    /* output layer does not previously exist */


	/*  Check if specified output layer name is legal   */
	if (flag == 3)
	{
		if (G_legal_filename (path_layer) < 0)
			G_fatal_error("%s - illegal name", path_layer);
	}

	/* If the output layer containing the starting positions*/
	/* create a linked list of of them  */
	if (flag == 1) {
		path_fd = G_open_cell_old (path_layer, path_mapset);
		if (path_fd < 0)
			G_fatal_error("%s -can't open raster map", path_layer);

		/*  Search for the marked starting pts and make list	*/
		for(row = 0; row < nrows; row++)
		{
			if(G_get_map_row(path_fd,cell,row) < 0)
				G_fatal_error("unable to get map row %d", row);

			for(col = 0; col < ncols; col++)
			{
				if(cell[col] > 0)
				{
					value = (char *)&backrow;
					segment_get (&in_row_seg, value, row, col);
					/* ignore pt in no-data area */
         				if(backrow < 0) {
               					G_warning("Ignoring point in NO-DATA area:") ;
                                                G_warning("   %.4f,%.4f\n", window.west + window.ew_res*(col + 0.5), window.north - window.ns_res*(row + 0.5)) ;
               			continue ;
          		}
					value = (char *)&backcol;
					segment_get (&in_col_seg, value, row, col);
					insert (&PRESENT_PT, row, col, backrow, backcol);
				}
			}	/* loop over cols */
		}	/* loop over rows */

		G_close_cell(path_fd);
	}

	/* loop over the starting points to find the least cost paths */
	if (verbose) 
                G_message("\nFinding the least cost paths ...");

	PRES_PT = head_start_pt;
	while(PRES_PT != NULL)
	{
		path_finder (PRES_PT->row, PRES_PT->col, PRES_PT->backrow, PRES_PT->backcol);

		OLD_PT = PRES_PT;
		PRES_PT = NEXT_PT;
		G_free (OLD_PT);
	}

	/* Write pending updates by segment_put() to outputmap */
	segment_flush(&out_seg);

	if (verbose) 
                G_message("\nWriting the output map  -%s-...", path_layer);

	path_fd = G_open_cell_new(path_layer);
	for ( row=0 ; row<nrows; row++ )
	{
		segment_get_row(&out_seg,cell,row);
		if (G_put_raster_row(path_fd,cell, CELL_TYPE) < 0)
			G_fatal_error("unable to write map row %d", row);
	}

	if (verbose)
		G_message("finished.");

	segment_release(&in_row_seg);   /* release memory  */
	segment_release(&in_col_seg);
	segment_release(&out_seg);

	close(in_row_fd);               /* close all files */
	close(in_col_fd);
	close(out_fd);

	G_close_cell(path_fd);
	G_close_cell(backrow_fd);
	G_close_cell(backcol_fd);

	unlink(in_row_file);       /* remove submatrix files  */
	unlink(in_col_file);
	unlink(out_file);

	exit(EXIT_SUCCESS);
}
