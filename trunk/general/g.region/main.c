
/***************************************************************************
 *
 * MODULE: 	g.region (commandline)
 * AUTHOR(S):	Michael Shapiro, CERL
 *              datum added by Andreas Lange <andreas.lange@rhein-main.de>
 * PURPOSE: 	Program to manage and print the boundary definitions for the
 *              geographic region.
 * 
 * COPYRIGHT:  	(C) 2000 by the GRASS Development Team
 *
 *   	    	This program is free software under the GPL (>=v2)
 *   	    	Read the file COPYING that comes with GRASS for details.
 ****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"

static int nsew(const char *, const char *, const char *, const char *);
static void die(struct Option *);

int main(int argc, char *argv[])
{
    int i;
    int print_flag = 0;
    int flat_flag; 
    int set_flag;
    double x, xs, ys, zs;
    int ival;
    int row_flag = 0, col_flag = 0;
    struct Cell_head window, temp_window;
    const char *value;
    const char *name;
    const char *mapset;
    char **rast_ptr, **vect_ptr;
    int pix;

    struct GModule *module;
    struct
    {
	struct Flag
	    *update, *print, *gprint, *flprint, *lprint, *eprint, *nangle,
	    *center, *res_set, *dist_res, *dflt, *z, *savedefault,
	    *bbox, *gmt_style, *wms_style;
    } flag;
    struct
    {
	struct Option
	    *north, *south, *east, *west, *top, *bottom,
	    *res, *nsres, *ewres, *res3, *tbres, *rows, *cols,
	    *save, *region, *raster, *raster3d, *align,
	    *zoom, *vect, *grow;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("computational region"));
    G_add_keyword(_("extent"));
    G_add_keyword(_("resolution"));
    G_add_keyword(_("level1"));
    module->description =
	_("Manages the boundary definitions for the " "geographic region.");

    /* flags */

    flag.dflt = G_define_flag();
    flag.dflt->key = 'd';
    flag.dflt->description = _("Set from default region");
    flag.dflt->guisection = _("Existing");

    flag.savedefault = G_define_flag();
    flag.savedefault->key = 's';
    flag.savedefault->label = _("Save as default region");
    flag.savedefault->description = _("Only possible from the PERMANENT mapset");
    flag.savedefault->guisection = _("Existing");

    flag.print = G_define_flag();
    flag.print->key = 'p';
    flag.print->description = _("Print the current region");
    flag.print->guisection = _("Print");

    flag.lprint = G_define_flag();
    flag.lprint->key = 'l';
    flag.lprint->description = _("Print the current region in lat/long "
				 "using the current ellipsoid/datum");
    flag.lprint->guisection = _("Print");

    flag.eprint = G_define_flag();
    flag.eprint->key = 'e';
    flag.eprint->description = _("Print the current region extent");
    flag.eprint->guisection = _("Print");

    flag.center = G_define_flag();
    flag.center->key = 'c';
    flag.center->description =
	_("Print the current region map center coordinates");
    flag.center->guisection = _("Print");

    flag.gmt_style = G_define_flag();
    flag.gmt_style->key = 't';
    flag.gmt_style->description =
	_("Print the current region in GMT style");
    flag.gmt_style->guisection = _("Print");

    flag.wms_style = G_define_flag();
    flag.wms_style->key = 'w';
    flag.wms_style->description =
	_("Print the current region in WMS style");
    flag.wms_style->guisection = _("Print");

    flag.dist_res = G_define_flag();
    flag.dist_res->key = 'm';
    flag.dist_res->description =
	_("Print region resolution in meters (geodesic)");
    flag.dist_res->guisection = _("Print");

    flag.nangle = G_define_flag();
    flag.nangle->key = 'n';
    flag.nangle->label = _("Print the convergence angle (degrees CCW)");
    flag.nangle->description =
	_("The difference between the projection's grid north and true north, "
	  "measured at the center coordinates of the current region.");
    flag.nangle->guisection = _("Print");

    flag.z = G_define_flag();
    flag.z->key = '3';
    flag.z->description = _("Print also 3D settings");
    flag.z->guisection = _("Print");

    flag.bbox = G_define_flag();
    flag.bbox->key = 'b';
    flag.bbox->description =
	_("Print the maximum bounding box in lat/long on WGS84");
    flag.bbox->guisection = _("Print");

    flag.gprint = G_define_flag();
    flag.gprint->key = 'g';
    flag.gprint->description = _("Print in shell script style");
    flag.gprint->guisection = _("Print");

    flag.flprint = G_define_flag();
    flag.flprint->key = 'f';
    flag.flprint->description = _("Print in shell script style, but in one line (flat)");
    flag.flprint->guisection = _("Print");

    flag.res_set = G_define_flag();
    flag.res_set->key = 'a';
    flag.res_set->description =
	_("Align region to resolution (default = align to bounds, "
	  "works only for 2D resolution)");
    flag.res_set->guisection = _("Bounds");

    flag.update = G_define_flag();
    flag.update->key = 'u';
    flag.update->description = _("Do not update the current region");
    flag.update->guisection = _("Effects");

    /* parameters */

    parm.region = G_define_standard_option(G_OPT_M_REGION);
    parm.region->description = _("Set current region from named region");
    parm.region->guisection = _("Existing");

    parm.raster = G_define_standard_option(G_OPT_R_MAP);
    parm.raster->key = "raster";
    parm.raster->required = NO;
    parm.raster->multiple = YES;
    parm.raster->description = _("Set region to match raster map(s)");
    parm.raster->guisection = _("Existing");

    parm.raster3d = G_define_standard_option(G_OPT_R3_MAP);
    parm.raster3d->key = "raster_3d";
    parm.raster3d->required = NO;
    parm.raster3d->multiple = NO;
    parm.raster3d->description =
	_("Set region to match 3D raster map(s) (both 2D and 3D "
	  "values)");
    parm.raster3d->guisection = _("Existing");

    parm.vect = G_define_standard_option(G_OPT_V_MAP);
    parm.vect->key = "vector";
    parm.vect->required = NO;
    parm.vect->multiple = YES;
    parm.vect->label = _("Set region to match vector map(s)");
    parm.vect->description = NULL;
    parm.vect->guisection = _("Existing");

    parm.north = G_define_option();
    parm.north->key = "n";
    parm.north->key_desc = "value";
    parm.north->required = NO;
    parm.north->multiple = NO;
    parm.north->type = TYPE_STRING;
    parm.north->description = _("Value for the northern edge");
    parm.north->guisection = _("Bounds");

    parm.south = G_define_option();
    parm.south->key = "s";
    parm.south->key_desc = "value";
    parm.south->required = NO;
    parm.south->multiple = NO;
    parm.south->type = TYPE_STRING;
    parm.south->description = _("Value for the southern edge");
    parm.south->guisection = _("Bounds");

    parm.east = G_define_option();
    parm.east->key = "e";
    parm.east->key_desc = "value";
    parm.east->required = NO;
    parm.east->multiple = NO;
    parm.east->type = TYPE_STRING;
    parm.east->description = _("Value for the eastern edge");
    parm.east->guisection = _("Bounds");

    parm.west = G_define_option();
    parm.west->key = "w";
    parm.west->key_desc = "value";
    parm.west->required = NO;
    parm.west->multiple = NO;
    parm.west->type = TYPE_STRING;
    parm.west->description = _("Value for the western edge");
    parm.west->guisection = _("Bounds");

    parm.top = G_define_option();
    parm.top->key = "t";
    parm.top->key_desc = "value";
    parm.top->required = NO;
    parm.top->multiple = NO;
    parm.top->type = TYPE_STRING;
    parm.top->description = _("Value for the top edge");
    parm.top->guisection = _("Bounds");

    parm.bottom = G_define_option();
    parm.bottom->key = "b";
    parm.bottom->key_desc = "value";
    parm.bottom->required = NO;
    parm.bottom->multiple = NO;
    parm.bottom->type = TYPE_STRING;
    parm.bottom->description = _("Value for the bottom edge");
    parm.bottom->guisection = _("Bounds");

    parm.rows = G_define_option();
    parm.rows->key = "rows";
    parm.rows->key_desc = "value";
    parm.rows->required = NO;
    parm.rows->multiple = NO;
    parm.rows->type = TYPE_INTEGER;
    parm.rows->description = _("Number of rows in the new region");
    parm.rows->guisection = _("Resolution");

    parm.cols = G_define_option();
    parm.cols->key = "cols";
    parm.cols->key_desc = "value";
    parm.cols->required = NO;
    parm.cols->multiple = NO;
    parm.cols->type = TYPE_INTEGER;
    parm.cols->description = _("Number of columns in the new region");
    parm.cols->guisection = _("Resolution");

    parm.res = G_define_option();
    parm.res->key = "res";
    parm.res->key_desc = "value";
    parm.res->required = NO;
    parm.res->multiple = NO;
    parm.res->type = TYPE_STRING;
    parm.res->description =
	_("2D grid resolution (north-south and east-west)");
    parm.res->guisection = _("Resolution");

    parm.res3 = G_define_option();
    parm.res3->key = "res3";
    parm.res3->key_desc = "value";
    parm.res3->required = NO;
    parm.res3->multiple = NO;
    parm.res3->type = TYPE_STRING;
    parm.res3->description =
	_("3D grid resolution (north-south, east-west and top-bottom)");
    parm.res3->guisection = _("Resolution");

    parm.nsres = G_define_option();
    parm.nsres->key = "nsres";
    parm.nsres->key_desc = "value";
    parm.nsres->required = NO;
    parm.nsres->multiple = NO;
    parm.nsres->type = TYPE_STRING;
    parm.nsres->description = _("North-south 2D grid resolution");
    parm.nsres->guisection = _("Resolution");

    parm.ewres = G_define_option();
    parm.ewres->key = "ewres";
    parm.ewres->key_desc = "value";
    parm.ewres->required = NO;
    parm.ewres->multiple = NO;
    parm.ewres->type = TYPE_STRING;
    parm.ewres->description = _("East-west 2D grid resolution");
    parm.ewres->guisection = _("Resolution");

    parm.tbres = G_define_option();
    parm.tbres->key = "tbres";
    parm.tbres->key_desc = "value";
    parm.tbres->required = NO;
    parm.tbres->multiple = NO;
    parm.tbres->type = TYPE_STRING;
    parm.tbres->description = _("Top-bottom 3D grid resolution");
    parm.tbres->guisection = _("Resolution");

    parm.zoom = G_define_option();
    parm.zoom->key = "zoom";
    parm.zoom->key_desc = "name";
    parm.zoom->required = NO;
    parm.zoom->multiple = NO;
    parm.zoom->type = TYPE_STRING;
    parm.zoom->description =
	_("Shrink region until it meets non-NULL data from this raster map");
    parm.zoom->gisprompt = "old,cell,raster";
    parm.zoom->guisection = _("Bounds");

    parm.align = G_define_option();
    parm.align->key = "align";
    parm.align->key_desc = "name";
    parm.align->required = NO;
    parm.align->multiple = NO;
    parm.align->type = TYPE_STRING;
    parm.align->description =
	_("Adjust region cells to cleanly align with this raster map");
    parm.align->gisprompt = "old,cell,raster";
    parm.align->guisection = _("Bounds");

    parm.grow = G_define_option();
    parm.grow->key = "grow";
    parm.grow->key_desc = "value";
    parm.grow->required = NO;
    parm.grow->multiple = NO;
    parm.grow->type = TYPE_INTEGER;
    parm.grow->label =
	_("Number of cells to add to each side of the current region extent");
    parm.grow->description =
	_("A negative number shrinks the current region extent");
    parm.grow->guisection = _("Bounds");

    parm.save = G_define_option();
    parm.save->key = "save";
    parm.save->key_desc = "name";
    parm.save->required = NO;
    parm.save->multiple = NO;
    parm.save->type = TYPE_STRING;
    parm.save->description =
	_("Save current region settings in named region file");
    parm.save->gisprompt = "new,windows,region";
    parm.save->guisection = _("Effects");

    G_option_required(flag.dflt, flag.savedefault, flag.print, flag.lprint,
                      flag.eprint, flag.center, flag.gmt_style, flag.wms_style,
                      flag.dist_res, flag.nangle, flag. z, flag.bbox, flag.gprint,
                      flag.res_set, flag.update, parm.region, parm.raster,
                      parm.raster3d, parm.vect, parm.north, parm.south, parm.east,
                      parm.west, parm.top, parm.bottom, parm.rows, parm.cols,
                      parm.res, parm.res3, parm.nsres, parm.ewres, parm.tbres,
                      parm.zoom, parm.align, parm.save, parm.grow, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_default_window(&window);

    set_flag = !flag.update->answer;
    flat_flag = flag.flprint->answer;

    if (flag.print->answer)
	print_flag |= PRINT_REG;

    if (flag.gprint->answer)
	print_flag |= PRINT_SH;

    if (flag.lprint->answer)
	print_flag |= PRINT_LL;

    if (flag.eprint->answer)
	print_flag |= PRINT_EXTENT;

    if (flag.center->answer)
	print_flag |= PRINT_CENTER;

    if (flag.gmt_style->answer)
	print_flag |= PRINT_GMT;

    if (flag.wms_style->answer)
	print_flag |= PRINT_WMS;

    if (flag.nangle->answer)
	print_flag |= PRINT_NANGLE;

    if (flag.dist_res->answer)
	print_flag |= PRINT_METERS;

    if (flag.z->answer)
	print_flag |= PRINT_3D;

    if (flag.bbox->answer)
	print_flag |= PRINT_MBBOX;

    if (print_flag == PRINT_METERS)
	print_flag |= PRINT_SH;

    if (print_flag == PRINT_SH ||
	print_flag & PRINT_3D || print_flag == PRINT_METERS + PRINT_SH) {
	print_flag |= PRINT_REG;
    }

    if (!flag.dflt->answer)
	G_get_window(&window);

    /* region= */
    if ((name = parm.region->answer)) {
	mapset = G_find_file2("windows", name, "");
	if (!mapset)
	    G_fatal_error(_("Region <%s> not found"), name);
	G_get_element_window(&window, "windows", name, mapset);
    }

    /* raster= */
    if (parm.raster->answer) {
	int first = 0;

	rast_ptr = parm.raster->answers;
	for (; *rast_ptr != NULL; rast_ptr++) {
	    char rast_name[GNAME_MAX];

	    strcpy(rast_name, *rast_ptr);
	    mapset = G_find_raster2(rast_name, "");
	    if (!mapset)
		G_fatal_error(_("Raster map <%s> not found"), rast_name);
	    Rast_get_cellhd(rast_name, mapset, &temp_window);
	    if (!first) {
		window = temp_window;
		first = 1;
	    }
	    else {
		window.north = (window.north > temp_window.north) ?
		    window.north : temp_window.north;
		window.south = (window.south < temp_window.south) ?
		    window.south : temp_window.south;
		window.east = (window.east > temp_window.east) ?
		    window.east : temp_window.east;
		window.west = (window.west < temp_window.west) ?
		    window.west : temp_window.west;
	    }
	}
	G_adjust_Cell_head3(&window, 0, 0, 0);
    }


    /* raster3d= */
    if ((name = parm.raster3d->answer)) {
	RASTER3D_Region win;

	if ((mapset = G_find_raster3d(name, "")) == NULL)
	    G_fatal_error(_("3D raster map <%s> not found"), name);

	if (Rast3d_read_region_map(name, mapset, &win) < 0)
	    G_fatal_error(_("Unable to read header of 3D raster map <%s@%s>"),
			  name, mapset);

	Rast3d_region_to_cell_head(&win, &window);
    }

    /* vector= */
    if (parm.vect->answer) {
	int first = 0;

	vect_ptr = parm.vect->answers;
	for (; *vect_ptr != NULL; vect_ptr++) {
            int ret;
	    struct Map_info Map;
	    struct bound_box box;
	    char vect_name[GNAME_MAX];
	    struct Cell_head map_window;

	    strcpy(vect_name, *vect_ptr);
	    mapset = G_find_vector2(vect_name, "");
	    if (!mapset)
		G_fatal_error(_("Vector map <%s> not found"), vect_name);

	    temp_window = window;

            /* try to open head-only on level 2 */
            if (Vect_open_old_head(&Map, vect_name, mapset) < 2) {
                /* force level 1, open fully */
                Vect_close(&Map);
                Vect_set_open_level(1); /* no topology */
                if (Vect_open_old(&Map, vect_name, mapset) < 1)
                    G_fatal_error(_("Unable to open vector map <%s>"),
                                  vect_name);
            }

            ret = 0;
            if (Vect_level(&Map) > 1)            /* level 2 - topology available */
                ret = Vect_get_map_box(&Map, &box);
            else                                 /* level 1 */
                ret = Vect_get_map_box1(&Map, &box);
            if (ret != 1)
                G_fatal_error(_("Unable to get map bounding box"));
            
            map_window = window;
            map_window.north = box.N;
            map_window.south = box.S;
            map_window.west = box.W;
            map_window.east = box.E;
            map_window.top = box.T;
            map_window.bottom = box.B;

	    if (!first) {
		window = map_window;
		first = 1;
	    }
	    else {
		window.north = (window.north > map_window.north) ?
		    window.north : map_window.north;
		window.south = (window.south < map_window.south) ?
		    window.south : map_window.south;
		window.east = (window.east > map_window.east) ?
		    window.east : map_window.east;
		window.west = (window.west < map_window.west) ?
		    window.west : map_window.west;
		if (map_window.top > window.top)
		    window.top = map_window.top;
		if (map_window.bottom < window.bottom)
		    window.bottom = map_window.bottom;
	    }

	    if (window.north == window.south) {
		window.north = window.north + 0.5 * temp_window.ns_res;
		window.south = window.south - 0.5 * temp_window.ns_res;
	    }
	    if (window.east == window.west) {
		window.west = window.west - 0.5 * temp_window.ew_res;
		window.east = window.east + 0.5 * temp_window.ew_res;
	    }
	    if (window.top == window.bottom) {
		window.bottom = (window.bottom - 0.5 * temp_window.tb_res);
		window.top = (window.top + 0.5 * temp_window.tb_res);
	    }

	    if (flag.res_set->answer)
		Rast_align_window(&window, &temp_window);

	    Vect_close(&Map);
	}
    }

    /* n= */
    if ((value = parm.north->answer)) {
	if ((i = nsew(value, "n+", "n-", "s+"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.north);
	    switch (i) {
	    case 1:
		window.north += x;
		break;
	    case 2:
		window.north -= x;
		break;
	    case 3:
		window.north = window.south + x;
		break;
	    }
	}
	else if (G_scan_northing(value, &x, window.proj))
	    window.north = x;
	else
	    die(parm.north);
    }

    /* s= */
    if ((value = parm.south->answer)) {
	if ((i = nsew(value, "s+", "s-", "n-"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.south);
	    switch (i) {
	    case 1:
		window.south += x;
		break;
	    case 2:
		window.south -= x;
		break;
	    case 3:
		window.south = window.north - x;
		break;
	    }
	}
	else if (G_scan_northing(value, &x, window.proj))
	    window.south = x;
	else
	    die(parm.south);
    }

    /* e= */
    if ((value = parm.east->answer)) {
	if ((i = nsew(value, "e+", "e-", "w+"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.east);
	    switch (i) {
	    case 1:
		window.east += x;
		break;
	    case 2:
		window.east -= x;
		break;
	    case 3:
		window.east = window.west + x;
		break;
	    }
	}
	else if (G_scan_easting(value, &x, window.proj))
	    window.east = x;
	else
	    die(parm.east);
    }

    /* w= */
    if ((value = parm.west->answer)) {
	if ((i = nsew(value, "w+", "w-", "e-"))) {
	    if (!G_scan_resolution(value + 2, &x, window.proj))
		die(parm.west);
	    switch (i) {
	    case 1:
		window.west += x;
		break;
	    case 2:
		window.west -= x;
		break;
	    case 3:
		window.west = window.east - x;
		break;
	    }
	}
	else if (G_scan_easting(value, &x, window.proj))
	    window.west = x;
	else
	    die(parm.west);
    }

    /* t= */
    if ((value = parm.top->answer)) {
	if ((i = nsew(value, "t+", "t-", "b+"))) {
	    if (sscanf(value + 2, "%lf", &x) != 1)
		die(parm.top);
	    switch (i) {
	    case 1:
		window.top += x;
		break;
	    case 2:
		window.top -= x;
		break;
	    case 3:
		window.top = window.bottom + x;
		break;
	    }
	}
	else if (sscanf(value, "%lf", &x) == 1)
	    window.top = x;
	else
	    die(parm.top);
    }

    /* b= */
    if ((value = parm.bottom->answer)) {
	if ((i = nsew(value, "b+", "b-", "t-"))) {
	    if (sscanf(value + 2, "%lf", &x) != 1)
		die(parm.bottom);
	    switch (i) {
	    case 1:
		window.bottom += x;
		break;
	    case 2:
		window.bottom -= x;
		break;
	    case 3:
		window.bottom = window.top - x;
		break;
	    }
	}
	else if (sscanf(value, "%lf", &x) == 1)
	    window.bottom = x;
	else
	    die(parm.bottom);
    }

    /* res= */
    if ((value = parm.res->answer)) {
	if (!G_scan_resolution(value, &x, window.proj))
	    die(parm.res);
	window.ns_res = x;
	window.ew_res = x;

	if (flag.res_set->answer) {
	    window.north = ceil(window.north / x) * x;
	    window.south = floor(window.south / x) * x;
	    window.east = ceil(window.east / x) * x;
	    window.west = floor(window.west / x) * x;
	}
    }

    /* res3= */
    if ((value = parm.res3->answer)) {
	if (!G_scan_resolution(value, &x, window.proj))
	    die(parm.res);
	window.ns_res3 = x;
	window.ew_res3 = x;
	window.tb_res = x;
    }

    /* nsres= */
    if ((value = parm.nsres->answer)) {
	if (!G_scan_resolution(value, &x, window.proj))
	    die(parm.nsres);
	window.ns_res = x;

	if (flag.res_set->answer) {
	    window.north = ceil(window.north / x) * x;
	    window.south = floor(window.south / x) * x;
	}
    }

    /* ewres= */
    if ((value = parm.ewres->answer)) {
	if (!G_scan_resolution(value, &x, window.proj))
	    die(parm.ewres);
	window.ew_res = x;

	if (flag.res_set->answer) {
	    window.east = ceil(window.east / x) * x;
	    window.west = floor(window.west / x) * x;
	}
    }

    /* tbres= */
    if ((value = parm.tbres->answer)) {
	if (sscanf(value, "%lf", &x) != 1)
	    die(parm.tbres);
	window.tb_res = x;

	if (flag.res_set->answer) {
	    window.top = ceil(window.top / x) * x;
	    window.bottom = floor(window.bottom / x) * x;
	}
    }

    /* rows= */
    if ((value = parm.rows->answer)) {
	if (sscanf(value, "%i", &ival) != 1)
	    die(parm.rows);
	window.rows = ival;
	row_flag = 1;
    }

    /* cols= */
    if ((value = parm.cols->answer)) {
	if (sscanf(value, "%i", &ival) != 1)
	    die(parm.cols);
	window.cols = ival;
	col_flag = 1;
    }

    /* zoom= */
    if ((name = parm.zoom->answer)) {
	mapset = G_find_raster2(name, "");
	if (!mapset)
	    G_fatal_error(_("Raster map <%s> not found"), name);
	zoom(&window, name, mapset);
    }

    /* align= */
    if ((name = parm.align->answer)) {
	mapset = G_find_raster2(name, "");
	if (!mapset)
	    G_fatal_error(_("Raster map <%s> not found"), name);
	Rast_get_cellhd(name, mapset, &temp_window);
	Rast_align_window(&window, &temp_window);
    }
    
    /* grow by number of cells */
    if ((value = parm.grow->answer)){
        if (sscanf(value, "%i", &pix)){
            xs = window.ns_res * pix;
            if (window.north + xs > window.south - xs){
                if (G_projection() == PROJECTION_LL && (
                    window.north + xs <= 90.0 + 0.5 * window.ns_res ||
                    window.south - xs >= -90.0 - 0.5 * window.ns_res)) {
                    G_warning(_("'%s' option not used with <%s> because a coordinate would become invalid"), 
		              parm.grow->key, "latitude");
                } else {
                    window.north += xs;
                    window.south -= xs;
                }
            } else {
                G_warning(_("'%s' option not used with <%s> because <%s> would become minor than <%s>"),
		          parm.grow->key, "latitude", "north", "south");
            }
            ys = window.ew_res * pix;
            if (window.east + ys > window.west - ys){
                window.west -= ys;
                window.east += ys;
            } else {
                G_warning(_("'%s' option not used with <%s> because <%s> would become minor than <%s>"),
		          parm.grow->key, "longitude", "east", "west");
            }
            zs = window.tb_res * pix;
            if (window.top + zs > window.bottom - zs){
                window.top += zs;
                window.bottom -= zs;
            } else {
                G_warning(_("'%s' option not used with <%s> because <%s> would become minor than <%s>"),
		          parm.grow->key, "3D", "top", "bottom");
            }
        }
    }
	
    /* save= */
    if ((name = parm.save->answer)) {
	temp_window = window;
	G_adjust_Cell_head3(&temp_window, 0, 0, 0);
	if (G_put_element_window(&temp_window, "windows", name) < 0)
	    G_fatal_error(_("Unable to set region <%s>"), name);
    }

    G_adjust_Cell_head3(&window, row_flag, col_flag, 0);
    if (set_flag) {
	if (G_put_window(&window) < 0)
	    G_fatal_error(_("Unable to update current region"));
    }

    if (flag.savedefault->answer) {
	if (strcmp(G_mapset(), "PERMANENT") == 0) {
	    G_put_element_window(&window, "", "DEFAULT_WIND");
	}
	else {
	    G_fatal_error(_("Unable to change default region. "
			    "The current mapset is not <PERMANENT>."));
	}
    }				/* / flag.savedefault->answer */


    if (print_flag)
	print_window(&window, print_flag, flat_flag);

    exit(EXIT_SUCCESS);
}

static void die(struct Option *parm)
{
    /*
       G_usage();
     */
    G_fatal_error(_("Invalid input <%s=%s>"), parm->key, parm->answer);
}

static int nsew(const char *value, const char *a, const char *b, const char *c)
{
    if (strncmp(value, a, strlen(a)) == 0)
	return 1;
    if (strncmp(value, b, strlen(b)) == 0)
	return 2;
    if (strncmp(value, c, strlen(c)) == 0)
	return 3;
    return 0;
}

