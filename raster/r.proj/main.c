/***************************************************************************
*
* MODULE:       r.proj
*
* AUTHOR(S):    Martin Schroeder
*		 University of Heidelberg
*		 Dept. of Geography
*		 emes@geo0.geog.uni-heidelberg.de
*
* 		 (With the help of a lot of existing GRASS sources, in 
*		  particular v.proj) 
*
* PURPOSE:      r.proj converts a map to a new geographic projection. It reads a
*	        map from a different location, projects it and write it out
*	        to the current location. The projected data is resampled with
*	        one of three different methods: nearest neighbor, bilinear and
*	        cubic convolution.
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
* Changes
*		 Morten Hulden <morten@ngb.se>, Aug 2000:
*		 - aborts if input map is outside current location.
*		 - can handle projections (conic, azimuthal etc) where 
*		 part of the map may fall into areas where south is 
*		 upward and east is leftward.
*		 - avoids passing location edge coordinates to PROJ
*		 (they may be invalid in some projections).
*		 - output map will be clipped to borders of the current region.
*		 - output map cell edges and centers will coinside with those 
*		 of the current region.
*		 - output map resolution (unless changed explicitly) will
*		 match (exactly) the resolution of the current region.
*		 - if the input map is smaller than the current region, the 
*		 output map will only cover the overlapping area.
*                - if the input map is larger than the current region, only the
*		 needed amount of memory will be allocated for the projection
*	
*		 Bugfixes 20050328: added floor() before (int) typecasts to in avoid
*		 assymetrical rounding errors. Added missing offset outcellhd.ew_res/2 
*		 to initial xcoord for each row in main projection loop (we want to  project 
*		 center of cell, not border).
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "r.proj.h"

/* modify this table to add new methods */
struct menu menu[] = {
	{ p_nearest,	"nearest",	"nearest neighbor" },
	{ p_bilinear,	"bilinear",	"bilinear" },
	{ p_cubic,	"cubic",	"cubic convolution" },
	{ NULL,		NULL,		NULL }
};

static char *make_ipol_list(void);

int main (int argc, char **argv)
{
	char     *mapname,		 /* ptr to name of output layer	 */
	         *setname,		 /* ptr to name of input mapset	 */
	         *ipolname;		 /* name of interpolation method */

	int       fdi,			 /* input map file descriptor	 */
	          fdo,			 /* output map file descriptor	 */
	          method,		 /* position of method in table	 */
	          permissions,		 /* mapset permissions		 */
	          cell_type,		 /* output celltype		 */
	          cell_size,		 /* size of a cell in bytes	 */
	          row, col,		 /* counters			 */
		  irows, icols,		 /* original rows, cols		 */
		  orows, ocols,
		  have_colors,     	 /* Input map has a colour table */
	          overwrite;             /* overwrite output map         */

	void     *obuffer,		 /* buffer that holds one output row	 */
	         *obufptr;		 /* column ptr in output buffer	 */
	FCELL   **ibuffer;		 /* buffer that holds the input map	 */
	func      interpolate;		 /* interpolation routine	 */

	double    xcoord1, xcoord2,	 /* temporary x coordinates 	 */
	          ycoord1, ycoord2,	 /* temporary y coordinates	 */
	          col_idx,		 /* column index in input matrix */
	          row_idx,		 /* row index in input matrix	 */
		  onorth, osouth,	 /* save original border coords  */
		  oeast, owest,
		  inorth, isouth,
		  ieast, iwest;

	struct Colors colr;		 /* Input map colour table       */
	struct History history;
   
	struct pj_info iproj,		 /* input map proj parameters	 */
	          oproj;		 /* output map proj parameters	 */

	struct Key_Value *in_proj_info,	 /* projection information of 	 */
	         *in_unit_info,		 /* input and output mapsets	 */
	         *out_proj_info,
	         *out_unit_info;

	struct GModule *module;

	struct Flag *list,		 /* list files in source location */
		 *nocrop;		 /* don't crop output map	 */

	struct Option *imapset,		 /* name of input mapset	 */
	         *inmap,		 /* name of input layer		 */
                 *inlocation,            /* name of input location       */
                 *outmap,		 /* name of output layer	 */
	         *indbase,		 /* name of input database	 */
	         *interpol,		 /* interpolation method:
 					    nearest neighbor, bilinear, cubic */
		 *res;			 /* resolution of target map     */
		  struct Cell_head incellhd,	 /* cell header of input map	 */
	          outcellhd;		 /* and output map		 */


	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("raster, projection");
	module->description =
		_("Re-projects a raster map from one location to the current location.");

	inmap = G_define_standard_option(G_OPT_R_INPUT);
	inmap -> description = _("Name of input raster map to re-project");
	inmap->required = NO;

	inlocation = G_define_option();
	inlocation->key = "location";
	inlocation->type = TYPE_STRING;
	inlocation->required = YES;
	inlocation->description = _("Location of input raster map");

	imapset = G_define_option();
	imapset->key = "mapset";
	imapset->type = TYPE_STRING;
	imapset->required = NO;
	imapset->description = _("Mapset of input raster map");

	indbase = G_define_option();
	indbase->key = "dbase";
	indbase->type = TYPE_STRING;
	indbase->required = NO;
	indbase->description = _("Path to GRASS database of input location");

	outmap = G_define_standard_option(G_OPT_R_OUTPUT);
	outmap->required = NO;
	outmap->description = _("Name for output raster map (default: input)");

	ipolname = make_ipol_list();

	interpol = G_define_option();
	interpol->key = "method";
	interpol->type = TYPE_STRING;
	interpol->required = NO;
	interpol->answer = "nearest";
	interpol->options = ipolname;
	interpol->description = _("Interpolation method to use");

	res = G_define_option();
	res->key = "resolution";
	res->type = TYPE_DOUBLE;
	res->required = NO;
	res->description = _("Resolution of output map");

	list = G_define_flag();
	list->key = 'l';
	list->description = _("List raster maps in input location and exit");

	nocrop = G_define_flag();
	nocrop->key = 'n';
	nocrop->description = _("Do not perform region cropping optimization");

	/* The parser checks if the map already exists in current mapset,
	   we switch out the check and do it
	 * in the module after the parser */
	overwrite = G_check_overwrite(argc, argv);
	
	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	/* get the method */
	for (method = 0; (ipolname = menu[method].name); method++)
		if (strcmp(ipolname, interpol->answer) == 0)
			break;

	if (!ipolname)
		G_fatal_error(_("<%s=%s> unknown %s"),
			interpol->key, interpol->answer, interpol->key);
	interpolate = menu[method].method;

	mapname = outmap->answer ? outmap->answer : inmap->answer;
	if (mapname && !list->answer && !overwrite && G_find_cell(mapname, G_mapset()))
	    G_fatal_error(_("option <%s>: <%s> exists."),
			  "output", mapname);
	
	setname = imapset->answer ? imapset->answer : G_store(G_mapset());

	if (!indbase->answer && strcmp(inlocation->answer, G_location()) == 0)
		G_fatal_error(_("Input and output locations can not be the same"));

	G_get_window(&outcellhd);

	/* Get projection info for output mapset */
	if ((out_proj_info = G_get_projinfo()) == NULL)
		G_fatal_error(_("Unable to get projection info of output raster map"));

	if ((out_unit_info = G_get_projunits()) == NULL)
		G_fatal_error(_("Unable to get projection units of output raster map"));

	if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
		G_fatal_error(_("Unable to get projection key values of output raster map"));

	/* Change the location 		 */
	G__create_alt_env();
	G__setenv("GISDBASE", indbase->answer ? indbase->answer : G_gisdbase());
	G__setenv("LOCATION_NAME", inlocation->answer);

	permissions = G__mapset_permissions(setname);
	if (permissions < 0)	/* can't access mapset 	 */
		G_fatal_error(_("Mapset <%s> in input location <%s> - %s"),
			      setname, inlocation->answer,
			      permissions == 0
			      ? _("permission denied")
			      : _("not found"));

	/* if requested, list the raster maps in source location - MN 5/2001*/
	if (list->answer)
	{
		if (isatty(0))  /* check if on command line */
			G_message(_("Checking location <%s>, mapset <%s>..."),
				inlocation->answer, setname);
		G_list_element ("cell", "raster", setname, 0);
		exit(EXIT_SUCCESS); /* leave r.proj after listing*/
	}

	if (!inmap->answer)
	    G_fatal_error (_("Required parameter <%s> not set"), inmap->key);

	if (!G_find_cell(inmap->answer, setname))
		G_fatal_error(_("Raster map <%s> in location <%s> in mapset <%s> not found"),
			      inmap->answer, inlocation->answer, setname);

	/* Read input map colour table */   
	have_colors = G_read_colors(inmap->answer, setname, &colr);
   
	/* Get projection info for input mapset */
	if ((in_proj_info = G_get_projinfo()) == NULL)
		G_fatal_error(_("Unable to get projection info of input map"));

	if ((in_unit_info = G_get_projunits()) == NULL)
		G_fatal_error(_("Unable to get projection units of input map"));

	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
		G_fatal_error(_("Unable to get projection key values of input map"));
   
        G_free_key_value( in_proj_info );	   
        G_free_key_value( in_unit_info );
        G_free_key_value( out_proj_info );	   
        G_free_key_value( out_unit_info );	   
        pj_print_proj_params(&iproj, &oproj);

	/* this call causes r.proj to read the entire map into memeory */
	G_get_cellhd(inmap->answer, setname, &incellhd);

	G_set_window(&incellhd);

	if (G_projection() == PROJECTION_XY)
		G_fatal_error(_("Unable to work with unprojected data (xy location)"));

	/* Save default borders so we can show them later */
	inorth = incellhd.north;
	isouth = incellhd.south;
	ieast = incellhd.east;
	iwest = incellhd.west;
	irows = incellhd.rows;
	icols = incellhd.cols;

	onorth = outcellhd.north;
	osouth = outcellhd.south;
	oeast = outcellhd.east;
	owest = outcellhd.west;
	orows = outcellhd.rows;
	ocols = outcellhd.cols;

	/* Cut non-overlapping parts of input map */
	if (!nocrop->answer)
		bordwalk(&outcellhd, &incellhd, &oproj, &iproj);

	/* Add 2 cells on each side for bilinear/cubic & future interpolation methods */
	/* (should probably be a factor based on input and output resolution) */
	incellhd.north += 2*incellhd.ns_res;
	incellhd.east  += 2*incellhd.ew_res;
	incellhd.south -= 2*incellhd.ns_res;
	incellhd.west  -= 2*incellhd.ew_res;
	if (incellhd.north > inorth) incellhd.north=inorth;
	if (incellhd.east > ieast) incellhd.east=ieast;
	if (incellhd.south < isouth) incellhd.south=isouth;
	if (incellhd.west < iwest) incellhd.west=iwest;

	G_set_window(&incellhd);

	/* And switch back to original location */

	G__switch_env();

	/* Adjust borders of output map */

	if (!nocrop->answer)
		bordwalk(&incellhd, &outcellhd, &iproj, &oproj);

#if 0
	outcellhd.west=outcellhd.south=HUGE_VAL;
	outcellhd.east=outcellhd.north=-HUGE_VAL;
	for(row=0;row<incellhd.rows;row++)
	{
		ycoord1=G_row_to_northing((double)(row+0.5),&incellhd);
		for(col=0;col<incellhd.cols;col++)
		{
			xcoord1=G_col_to_easting((double)(col+0.5),&incellhd);
			pj_do_proj(&xcoord1,&ycoord1,&iproj,&oproj);
			if(xcoord1>outcellhd.east)outcellhd.east=xcoord1;
			if(ycoord1>outcellhd.north)outcellhd.north=ycoord1;
			if(xcoord1<outcellhd.west)outcellhd.west=xcoord1;
			if(ycoord1<outcellhd.south)outcellhd.south=ycoord1;
		}
	}
#endif

	if (res->answer != NULL)   /* set user defined resolution */
		outcellhd.ns_res = outcellhd.ew_res = atof(res->answer);

	G_adjust_Cell_head(&outcellhd, 0, 0);
	G_set_window(&outcellhd);

	G_message(NULL);
	G_message(_("Input:"));
	G_message(_("Cols: %d (%d)"), incellhd.cols, icols);
	G_message(_("Rows: %d (%d)"), incellhd.rows, irows);
	G_message(_("North: %f (%f)"), incellhd.north, inorth);
	G_message(_("South: %f (%f)"), incellhd.south, isouth);
	G_message(_("West: %f (%f)"), incellhd.west, iwest);
	G_message(_("East: %f (%f)"), incellhd.east, ieast);
	G_message(_("EW-res: %f"), incellhd.ew_res);
	G_message(_("NS-res: %f"), incellhd.ns_res);

	G_message(NULL);
	G_message(_("Output:"));
	G_message(_("Cols: %d (%d)"), outcellhd.cols, ocols);
	G_message(_("Rows: %d (%d)"), outcellhd.rows, orows);
	G_message(_("North: %f (%f)"), outcellhd.north, onorth);
	G_message(_("South: %f (%f)"), outcellhd.south, osouth);
	G_message(_("West: %f (%f)"), outcellhd.west, owest);
	G_message(_("East: %f (%f)"), outcellhd.east, oeast);
	G_message(_("EW-res: %f"), outcellhd.ew_res);
	G_message(_("NS-res: %f"), outcellhd.ns_res);
	G_message(NULL);

	/* open and read the relevant parts of the input map and close it */
	G__switch_env();
	G_set_window(&incellhd);
	fdi = G_open_cell_old(inmap->answer, setname);
	cell_type = G_get_raster_map_type(fdi);
	ibuffer = (FCELL **) readcell(fdi);
	G_close_cell(fdi);

	G__switch_env();
	G_set_window(&outcellhd);

	if (strcmp(interpol->answer, "nearest") == 0)
	{
		fdo = G_open_raster_new(mapname, cell_type);
		obuffer = (CELL *) G_allocate_raster_buf(cell_type);
	}
	else
	{
		fdo = G_open_fp_cell_new(mapname);
		cell_type = FCELL_TYPE;
		obuffer = (FCELL *) G_allocate_raster_buf(cell_type);
	}

	cell_size = G_raster_size(cell_type);

	xcoord1 = xcoord2 = outcellhd.west + (outcellhd.ew_res / 2);	/**/
	ycoord1 = ycoord2 = outcellhd.north - (outcellhd.ns_res / 2);	/**/

	G_important_message(_("Projecting..."));
	G_percent(0, outcellhd.rows, 2);

	for (row = 0; row < outcellhd.rows; row++)
	{
		obufptr = obuffer;

		for (col = 0; col < outcellhd.cols; col++)
		{
			/* project coordinates in output matrix to	 */
			/* coordinates in input matrix			 */
			if (pj_do_proj(&xcoord1, &ycoord1, &oproj, &iproj) < 0)
				G_set_null_value(obufptr, 1, cell_type);
			else
			{
				/* convert to row/column indices of input matrix */
				col_idx = (xcoord1 - incellhd.west) / incellhd.ew_res;
				row_idx = (incellhd.north - ycoord1) / incellhd.ns_res;

				/* and resample data point		 */
				interpolate(ibuffer, obufptr, cell_type,
					    &col_idx, &row_idx, &incellhd);
			}

			obufptr = G_incr_void_ptr(obufptr, cell_size);
			xcoord2 += outcellhd.ew_res;
			xcoord1 = xcoord2;
			ycoord1 = ycoord2;
		}

		if (G_put_raster_row(fdo, obuffer, cell_type) < 0)
			G_fatal_error(_("Failed writing raster map <%s> row %d"), mapname, row);

		xcoord1 = xcoord2 = outcellhd.west + (outcellhd.ew_res / 2);
		ycoord2 -= outcellhd.ns_res;
		ycoord1 = ycoord2;
		G_percent(row, outcellhd.rows - 1, 2);
	}

	G_close_cell(fdo);

	if(have_colors > 0)
	{    
		G_write_colors(mapname, G_mapset(), &colr);
		G_free_colors(&colr);
	}

	G_short_history(mapname, "raster", &history);
	G_command_history(&history);
	G_write_history(mapname, &history);

	G_done_msg(" ");
	exit(EXIT_SUCCESS);
}

static char *make_ipol_list(void)
{
	int size = 0;
	int i;
	char *buf;

	for (i = 0; menu[i].name; i++)
		size += strlen(menu[i].name) + 1;

	buf = G_malloc(size);
	*buf = '\0';

	for (i = 0; menu[i].name; i++)
	{
		if (i) strcat(buf, ",");
		strcat(buf, menu[i].name);
	}

	return buf;
}
