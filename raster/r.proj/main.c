
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
* COPYRIGHT:    (C) 2001, 2011 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
* Changes
*		 Morten Hulden <morten@untamo.net>, Aug 2000:
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
*		 asymmetrical rounding errors. Added missing offset outcellhd.ew_res/2 
*		 to initial xcoord for each row in main projection loop (we want to  project 
*		 center of cell, not border).
*
*                Glynn Clements 2006: Use G_interp_* functions, modified      
*                  version of r.proj which uses a tile cache instead  of loading the
*                  entire map into memory.
*                Markus Metz 2010: lanczos and lanczos fallback interpolation methods

*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "r.proj.h"

/* modify this table to add new methods */
struct menu menu[] = {
    {p_nearest, "nearest", "nearest neighbor"},
    {p_bilinear, "bilinear", "bilinear interpolation"},
    {p_cubic, "bicubic", "bicubic interpolation"},
    {p_lanczos, "lanczos", "lanczos filter"},
    {p_bilinear_f, "bilinear_f", "bilinear interpolation with fallback"},
    {p_cubic_f, "bicubic_f", "bicubic interpolation with fallback"},
    {p_lanczos_f, "lanczos_f", "lanczos filter with fallback"},
    {NULL, NULL, NULL}
};

static char *make_ipol_list(void);
static char *make_ipol_desc(void);

int main(int argc, char **argv)
{
    char *mapname,		/* ptr to name of output layer  */
     *setname,			/* ptr to name of input mapset  */
     *ipolname;			/* name of interpolation method */

    int fdi,			/* input map file descriptor    */
      fdo,			/* output map file descriptor   */
      method,			/* position of method in table  */
      permissions,		/* mapset permissions           */
      cell_type,		/* output celltype              */
      cell_size,		/* size of a cell in bytes      */
      row, col,			/* counters                     */
      irows, icols,		/* original rows, cols          */
      orows, ocols, have_colors,	/* Input map has a colour table */
      overwrite,		/* Overwrite                    */
      curr_proj;		/* output projection (see gis.h) */

    void *obuffer;		/* buffer that holds one output row     */

    struct cache *ibuffer;	/* buffer that holds the input map      */
    func interpolate;		/* interpolation routine        */

    double xcoord2,		/* temporary x coordinates      */
      ycoord2,			/* temporary y coordinates      */
      onorth, osouth,		/* save original border coords  */
      oeast, owest, inorth, isouth, ieast, iwest;
    char north_str[30], south_str[30], east_str[30], west_str[30];

    struct Colors colr;		/* Input map colour table       */
    struct History history;

    struct pj_info iproj,	/* input map proj parameters    */
		   oproj,	/* output map proj parameters   */
		   tproj;	/* transformation parameters   */

    struct Key_Value *in_proj_info,	/* projection information of    */
     *in_unit_info,		/* input and output mapsets     */
     *out_proj_info, *out_unit_info;

    struct GModule *module;

    struct Flag *list,		/* list files in source location */
     *nocrop,			/* don't crop output map        */
     *print_bounds,		/* print output bounds and exit */
     *gprint_bounds;		/* same but print shell style	*/

    struct Option *imapset,	/* name of input mapset         */
     *inmap,			/* name of input layer          */
     *inlocation,		/* name of input location       */
     *outmap,			/* name of output layer         */
     *indbase,			/* name of input database       */
     *interpol,			/* interpolation method         */
     *memory,			/* amount of memory for cache   */
     *res;			/* resolution of target map     */
#ifdef HAVE_PROJ_H
    struct Option *pipeline;	/* name of custom PROJ pipeline */
#endif
    struct Cell_head incellhd,	/* cell header of input map     */
      outcellhd;		/* and output map               */


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("import"));
    module->description =
	_("Re-projects a raster map from given location to the current location.");

    inlocation = G_define_standard_option(G_OPT_M_LOCATION);
    inlocation->required = YES;
    inlocation->label = _("Location containing input raster map");
    inlocation->guisection = _("Source");

    imapset = G_define_standard_option(G_OPT_M_MAPSET);
    imapset->label = _("Mapset containing input raster map");
    imapset->description = _("Default: name of current mapset");
    imapset->guisection = _("Source");

    inmap = G_define_standard_option(G_OPT_R_INPUT);
    inmap->description = _("Name of input raster map to re-project");
    inmap->required = NO;
    inmap->guisection = _("Source");
    
    indbase = G_define_standard_option(G_OPT_M_DBASE);
    indbase->label = _("Path to GRASS database of input location");

    outmap = G_define_standard_option(G_OPT_R_OUTPUT);
    outmap->required = NO;
    outmap->description = _("Name for output raster map (default: same as 'input')");
    outmap->guisection = _("Target");

    ipolname = make_ipol_list();
    
    interpol = G_define_option();
    interpol->key = "method";
    interpol->type = TYPE_STRING;
    interpol->required = NO;
    interpol->answer = "nearest";
    interpol->options = ipolname;
    interpol->description = _("Interpolation method to use");
    interpol->guisection = _("Target");
    interpol->descriptions = make_ipol_desc();

    memory = G_define_option();
    memory->key = "memory";
    memory->type = TYPE_INTEGER;
    memory->required = NO;
    memory->answer = "300";
    memory->label = _("Maximum memory to be used (in MB)");
    memory->description = _("Cache size for raster rows");

    res = G_define_option();
    res->key = "resolution";
    res->type = TYPE_DOUBLE;
    res->required = NO;
    res->description = _("Resolution of output raster map");
    res->guisection = _("Target");

#ifdef HAVE_PROJ_H
    pipeline = G_define_option();
    pipeline->key = "pipeline";
    pipeline->type = TYPE_STRING;
    pipeline->required = NO;
    pipeline->description = _("PROJ pipeline for coordinate transformation");
#endif

    list = G_define_flag();
    list->key = 'l';
    list->description = _("List raster maps in input mapset and exit");
    list->guisection = _("Print");
    
    nocrop = G_define_flag();
    nocrop->key = 'n';
    nocrop->description = _("Do not perform region cropping optimization");

    print_bounds = G_define_flag();
    print_bounds->key = 'p';
    print_bounds->description =
	_("Print input map's bounds in the current projection and exit");
    print_bounds->guisection = _("Print");
    
    gprint_bounds = G_define_flag();
    gprint_bounds->key = 'g';
    gprint_bounds->description =
	_("Print input map's bounds in the current projection and exit (shell style)");
    gprint_bounds->guisection = _("Print");

    /* The parser checks if the map already exists in current mapset,
       we switch out the check and do it
       in the module after the parser */
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
    if (mapname && !list->answer && !overwrite &&
	!print_bounds->answer && !gprint_bounds->answer &&
	G_find_raster(mapname, G_mapset()))
	G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the --overwrite flag"),
                      "output", mapname);

    setname = imapset->answer ? imapset->answer : G_store(G_mapset());
    if (strcmp(inlocation->answer, G_location()) == 0 &&
        (!indbase->answer || strcmp(indbase->answer, G_gisdbase()) == 0))
#if 0
	G_fatal_error(_("Input and output locations can not be the same"));
#else
	G_warning(_("Input and output locations are the same"));
#endif
    G_get_window(&outcellhd);

    if (gprint_bounds->answer && !print_bounds->answer)
	print_bounds->answer = gprint_bounds->answer;
    curr_proj = G_projection();

    /* Get projection info for output mapset */
    if ((out_proj_info = G_get_projinfo()) == NULL)
	G_fatal_error(_("Unable to get projection info of output raster map"));

    if ((out_unit_info = G_get_projunits()) == NULL)
	G_fatal_error(_("Unable to get projection units of output raster map"));

    if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
	G_fatal_error(_("Unable to get projection key values of output raster map"));

    /* Change the location           */
    G_create_alt_env();
    G_setenv_nogisrc("GISDBASE", indbase->answer ? indbase->answer : G_gisdbase());
    G_setenv_nogisrc("LOCATION_NAME", inlocation->answer);
    G_setenv_nogisrc("MAPSET", setname);

    permissions = G_mapset_permissions(setname);
    if (permissions < 0)	/* can't access mapset       */
	G_fatal_error(_("Mapset <%s> in input location <%s> - %s"),
		      setname, inlocation->answer,
		      permissions == 0 ? _("permission denied")
		      : _("not found"));

    /* if requested, list the raster maps in source location - MN 5/2001 */
    if (list->answer) {
	int i;
	char **srclist;
	G_verbose_message(_("Checking location <%s> mapset <%s>"),
			  inlocation->answer, setname);
	srclist = G_list(G_ELEMENT_RASTER, G_getenv_nofatal("GISDBASE"),
		      G_getenv_nofatal("LOCATION_NAME"), setname);
	for (i = 0; srclist[i]; i++) {
	    fprintf(stdout, "%s\n", srclist	[i]);
	}
	fflush(stdout);
	exit(EXIT_SUCCESS);	/* leave r.proj after listing */
    }

    if (!inmap->answer)
	G_fatal_error(_("Required parameter <%s> not set"), inmap->key);

    if (!G_find_raster(inmap->answer, setname))
	G_fatal_error(_("Raster map <%s> in location <%s> in mapset <%s> not found"),
		      inmap->answer, inlocation->answer, setname);

    /* Read input map colour table */
    have_colors = Rast_read_colors(inmap->answer, setname, &colr);

    /* Get projection info for input mapset */
    if ((in_proj_info = G_get_projinfo()) == NULL)
	G_fatal_error(_("Unable to get projection info of input map"));

    /* apparently the +over switch must be set in the input projection,
     * not the output latlon projection */
    if (curr_proj == PROJECTION_LL)
	G_set_key_value("+over", "defined", in_proj_info);

    if ((in_unit_info = G_get_projunits()) == NULL)
	G_fatal_error(_("Unable to get projection units of input map"));

    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	G_fatal_error(_("Unable to get projection key values of input map"));

    tproj.def = NULL;
#ifdef HAVE_PROJ_H
    if (pipeline->answer) {
	tproj.def = G_store(pipeline->answer);
    }
#endif

    /* switch back to current location */
    G_switch_env();
    if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
	G_fatal_error(_("Unable to initialize coordinate transformation"));

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);
    G_free_key_value(out_proj_info);
    G_free_key_value(out_unit_info);
    if (G_verbose() > G_verbose_std())
	pj_print_proj_params(&iproj, &oproj);

    /* switch to input location */
    G_switch_env();

    /* this call causes r.proj to read the entire map into memeory */
    Rast_get_cellhd(inmap->answer, setname, &incellhd);

    Rast_set_input_window(&incellhd);

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


    if (print_bounds->answer) {
	G_message(_("Input map <%s@%s> in location <%s>:"),
	    inmap->answer, setname, inlocation->answer);

	outcellhd.north = -1e9;
	outcellhd.south =  1e9;
	outcellhd.east  = -1e9;
	outcellhd.west  =  1e9;
	bordwalk_edge(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);
	inorth = outcellhd.north;
	isouth = outcellhd.south;
	ieast  = outcellhd.east;
	iwest  = outcellhd.west;

	G_format_northing(inorth, north_str, curr_proj);
	G_format_northing(isouth, south_str, curr_proj);
	G_format_easting(ieast, east_str, curr_proj);
	G_format_easting(iwest, west_str, curr_proj);

	if (gprint_bounds->answer) {
	    fprintf(stdout, "n=%s s=%s w=%s e=%s rows=%d cols=%d\n",
		north_str, south_str, west_str, east_str, irows, icols);
	}
	else {
	    fprintf(stdout, "Source cols: %d\n", icols);
	    fprintf(stdout, "Source rows: %d\n", irows);
	    fprintf(stdout, "Local north: %s\n",  north_str);
	    fprintf(stdout, "Local south: %s\n", south_str);
	    fprintf(stdout, "Local west: %s\n", west_str);
	    fprintf(stdout, "Local east: %s\n", east_str);
	}

	exit(EXIT_SUCCESS);
    }


    /* Cut non-overlapping parts of input map */
    if (!nocrop->answer)
	bordwalk(&outcellhd, &incellhd, &iproj, &oproj, &tproj, PJ_INV);

    /* Add 2 cells on each side for bilinear/cubic & future interpolation methods */
    /* (should probably be a factor based on input and output resolution) */
    incellhd.north += 2 * incellhd.ns_res;
    incellhd.east += 2 * incellhd.ew_res;
    incellhd.south -= 2 * incellhd.ns_res;
    incellhd.west -= 2 * incellhd.ew_res;
    if (incellhd.north > inorth)
	incellhd.north = inorth;
    if (incellhd.east > ieast)
	incellhd.east = ieast;
    if (incellhd.south < isouth)
	incellhd.south = isouth;
    if (incellhd.west < iwest)
	incellhd.west = iwest;

    Rast_set_input_window(&incellhd);

    /* And switch back to original location */

    G_switch_env();

    /* Adjust borders of output map */

    if (!nocrop->answer)
	bordwalk(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);

#if 0
    outcellhd.west = outcellhd.south = HUGE_VAL;
    outcellhd.east = outcellhd.north = -HUGE_VAL;
    for (row = 0; row < incellhd.rows; row++) {
	ycoord1 = Rast_row_to_northing((double)(row + 0.5), &incellhd);
	for (col = 0; col < incellhd.cols; col++) {
	    xcoord1 = Rast_col_to_easting((double)(col + 0.5), &incellhd);
	    if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD,
			      &xcoord1, &ycoord1, NULL) < 0)
		G_fatal_error(_("Error in %s"), "GPJ_transform()");
	    if (xcoord1 > outcellhd.east)
		outcellhd.east = xcoord1;
	    if (ycoord1 > outcellhd.north)
		outcellhd.north = ycoord1;
	    if (xcoord1 < outcellhd.west)
		outcellhd.west = xcoord1;
	    if (ycoord1 < outcellhd.south)
		outcellhd.south = ycoord1;
	}
    }
#endif

    if (res->answer != NULL)	/* set user defined resolution */
	outcellhd.ns_res = outcellhd.ew_res = atof(res->answer);

    G_adjust_Cell_head(&outcellhd, 0, 0);
    Rast_set_output_window(&outcellhd);

    G_message(" ");
    G_message(_("Input:"));
    G_message(_("Cols: %d (original: %d)"), incellhd.cols, icols);
    G_message(_("Rows: %d (original: %d)"), incellhd.rows, irows);
    G_message(_("North: %f (original: %f)"), incellhd.north, inorth);
    G_message(_("South: %f (original: %f)"), incellhd.south, isouth);
    G_message(_("West: %f (original: %f)"), incellhd.west, iwest);
    G_message(_("East: %f (original: %f)"), incellhd.east, ieast);
    G_message(_("EW-res: %f"), incellhd.ew_res);
    G_message(_("NS-res: %f"), incellhd.ns_res);
    G_message(" ");

    G_message(_("Output:"));
    G_message(_("Cols: %d (original: %d)"), outcellhd.cols, ocols);
    G_message(_("Rows: %d (original: %d)"), outcellhd.rows, orows);
    G_message(_("North: %f (original: %f)"), outcellhd.north, onorth);
    G_message(_("South: %f (original: %f)"), outcellhd.south, osouth);
    G_message(_("West: %f (original: %f)"), outcellhd.west, owest);
    G_message(_("East: %f (original: %f)"), outcellhd.east, oeast);
    G_message(_("EW-res: %f"), outcellhd.ew_res);
    G_message(_("NS-res: %f"), outcellhd.ns_res);
    G_message(" ");

    /* open and read the relevant parts of the input map and close it */
    G_switch_env();
    Rast_set_input_window(&incellhd);
    fdi = Rast_open_old(inmap->answer, setname);
    cell_type = Rast_get_map_type(fdi);
    ibuffer = readcell(fdi, memory->answer);
    Rast_close(fdi);

    G_switch_env();
    Rast_set_output_window(&outcellhd);

    if (strcmp(interpol->answer, "nearest") == 0) {
	fdo = Rast_open_new(mapname, cell_type);
	obuffer = (CELL *) Rast_allocate_output_buf(cell_type);
    }
    else {
	fdo = Rast_open_fp_new(mapname);
	cell_type = FCELL_TYPE;
	obuffer = (FCELL *) Rast_allocate_output_buf(cell_type);
    }

    cell_size = Rast_cell_size(cell_type);

    xcoord2 = outcellhd.west + (outcellhd.ew_res / 2);
    ycoord2 = outcellhd.north - (outcellhd.ns_res / 2);

    G_important_message(_("Projecting..."));
    for (row = 0; row < outcellhd.rows; row++) {
	/* obufptr = obuffer */;

        G_percent(row, outcellhd.rows - 1, 2);

#if 0
	/* parallelization does not always work,
	 * segfaults in the interpolation functions 
	 * can happen */
        #pragma omp parallel for schedule (static)
#endif

	for (col = 0; col < outcellhd.cols; col++) {
	    void *obufptr = (void *)((const unsigned char *)obuffer + col * cell_size);

	    double xcoord1 = xcoord2 + (col) * outcellhd.ew_res;
	    double ycoord1 = ycoord2;

	    /* project coordinates in output matrix to       */
	    /* coordinates in input matrix                   */
	    if (GPJ_transform(&iproj, &oproj, &tproj, PJ_INV,
			      &xcoord1, &ycoord1, NULL) < 0) {
		G_warning(_("Error in %s"), "GPJ_transform()");
		Rast_set_null_value(obufptr, 1, cell_type);
	    }
	    else {
		/* convert to row/column indices of input matrix */

		/* column index in input matrix */
		double col_idx = (xcoord1 - incellhd.west) / incellhd.ew_res;
		/* row index in input matrix    */
		double row_idx = (incellhd.north - ycoord1) / incellhd.ns_res;

		/* and resample data point               */
		interpolate(ibuffer, obufptr, cell_type,
			    col_idx, row_idx, &incellhd);
	    }

	    /* obufptr = G_incr_void_ptr(obufptr, cell_size); */
	}

	Rast_put_row(fdo, obuffer, cell_type);

	xcoord2 = outcellhd.west + (outcellhd.ew_res / 2);
	ycoord2 -= outcellhd.ns_res;
    }

    Rast_close(fdo);
    release_cache(ibuffer);

    if (have_colors > 0) {
	Rast_write_colors(mapname, G_mapset(), &colr);
	Rast_free_colors(&colr);
    }

    Rast_short_history(mapname, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(mapname, &history);

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}

char *make_ipol_list(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
	size += strlen(menu[i].name) + 1;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(buf, ",");
	strcat(buf, menu[i].name);
    }

    return buf;
}

char *make_ipol_desc(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
	size += strlen(menu[i].name) + strlen(menu[i].text) + 2;
    
    buf = G_malloc(size);
    *buf = '\0';
    
    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(buf, ";");
	strcat(buf, menu[i].name);
	strcat(buf, ";");
	strcat(buf, menu[i].text);
    }

    return buf;
}
