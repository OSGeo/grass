/*
 ****************************************************************************
 *
 * MODULE:       d.info
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      Display information about the active display monitor
 * COPYRIGHT:    (C) 2004 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

int main(int argc,char *argv[])
{
	struct GModule *module;
	struct Flag *rflag, *dflag, *cflag, *fflag, *bflag, *gflag;
	int l, r, t, b;
	char window_name[128];
	struct Cell_head window;

	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("display");
    module->description = 
		_("Display information about the active display monitor");

	rflag = G_define_flag();
	rflag->key = 'r';
	rflag->description = _("Display screen rectangle (left, right, top, bottom)");

	dflag = G_define_flag();
	dflag->key = 'd';
	dflag->description = _("Display screen dimensions (width, height)");

	fflag = G_define_flag();
	fflag->key = 'f';
	fflag->description = _("Display active frame rectangle");

	bflag = G_define_flag();
	bflag->key = 'b';
	bflag->description = _("Display screen rectangle of current region");

	gflag = G_define_flag();
	gflag->key = 'g';
	gflag->description = _("Display screen rectangle coordinates and resolution (west, east, north, south, ewres, nsres)");

	cflag = G_define_flag();
	cflag->key = 'c';
	cflag->description = _("Display number of colors");

	if (argc > 1 && G_parser(argc, argv))
		exit(EXIT_FAILURE);

	if(!rflag->answer && !dflag->answer && !cflag->answer &&
				!fflag->answer && !bflag->answer && !gflag->answer)
	{
		G_usage();
		exit(EXIT_FAILURE);
	}

	if (R_open_driver() != 0)
		G_fatal_error(_("No graphics device selected"));

	if (rflag->answer || dflag->answer)
	{
		l = R_screen_left();
		r = R_screen_rite();
		t = R_screen_top();
		b = R_screen_bot();
	}

	if (rflag->answer)
		fprintf(stdout, "rectangle: %d %d %d %d\n", l, r, t, b);

	if (dflag->answer)
		fprintf(stdout, "dimensions: %d %d\n", r - l, b - t);

	if (cflag->answer)
	{
		int colors;
		R_get_num_colors(&colors);
		fprintf(stdout, "colors: %d\n", colors);
	}

	if (fflag->answer)
	{
		D_get_screen_window(&t, &b, &l, &r);
		fprintf(stdout, "frame: %d %d %d %d\n", l, r, t, b);
	}

	if (bflag->answer)
	{
	    if (D_get_cur_wind(window_name))
		G_fatal_error(_("No current window"));
	    if (D_set_cur_wind(window_name))
		G_fatal_error(_("Current window not available"));

	    /* Read in the map window associated with window */
	    G_get_window(&window) ;

	    if (D_check_map_window(&window))
		G_fatal_error(_("Setting map window"));
	    if (D_get_screen_window(&t, &b, &l, &r))
		G_fatal_error(_("Getting screen window"));
	    if (D_do_conversions(&window, t, b, l, r))
		G_fatal_error(_("Error in calculating conversions"));

	    l = D_get_d_west();
	    r = D_get_d_east();
	    t = D_get_d_north();
	    b = D_get_d_south();

	    fprintf(stdout, "region: %d %d %d %d\n", l, r, t, b);
	}

	if (gflag->answer)
	{
	    if (D_get_cur_wind(window_name))
		G_fatal_error(_("No current window"));
	    if (D_set_cur_wind(window_name))
		G_fatal_error(_("Current window not available"));

	    /* Read in the map window associated with window */
	    G_get_window(&window) ;
	    fprintf(stdout, "w=%f\n", window.west);
	    fprintf(stdout, "e=%f\n", window.east);
	    fprintf(stdout, "n=%f\n", window.north);
	    fprintf(stdout, "s=%f\n", window.south);
	    fprintf(stdout, "ewres=%f\n", window.ew_res);
	    fprintf(stdout, "nsres=%f\n", window.ns_res);
	   
	}
	
	R_close_driver();	

	return EXIT_SUCCESS;
}
