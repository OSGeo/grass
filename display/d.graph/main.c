
/****************************************************************************
 *
 * MODULE:       d.graph
 * AUTHOR(S):    Jim Westervelt (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      Draw graphics in a graphics window. Graph lines come from 
 *               stdin unless input specified.
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 *   d.graph
 *
 *   
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define MAIN
#include "options.h"
#include "local_proto.h"

int main(int argc, char **argv)
{
    char window_name[64];
    struct GModule *module;
    struct Option *opt1, *opt2;
    struct Flag *mapcoords;
    int R, G, B, color = 0;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display, cartography");
    module->description =
	_("Program for generating and displaying simple graphics on the "
	  "display monitor.");

    opt1 = G_define_option();
    opt1->key = "input";
    opt1->type = TYPE_STRING;
    opt1->required = NO;
    opt1->description = _("Name of file containing graphics commands, "
			  "if not given reads from standard input");
    opt1->gisprompt = "old_file,file,input";

    opt2 = G_define_option();
    opt2->key = "color";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->description = _("Color to draw with, either a standard GRASS color "
			  "or R:G:B triplet");
    opt2->answer = DEFAULT_FG_COLOR;
    opt2->gisprompt = GISPROMPT_COLOR;

    mapcoords = G_define_flag();
    mapcoords->key = 'm';
    mapcoords->description = _("Coordinates are given in map units");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* default font scaling: 5% of active frame */
    hsize = vsize = 5.;

    if (opt1->answer != NULL) {
	if ((infile = fopen(opt1->answer, "r")) == NULL)
	    G_fatal_error(_("Graph file <%s> not found"), opt1->answer);
    }
    else
	infile = stdin;

    /* open graphics window */
    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    /* Parse and select color */
    if (opt2->answer != NULL) {
	color = G_str_to_color(opt2->answer, &R, &G, &B);

	if (color == 0)
	    G_fatal_error(_("[%s]: No such color"), opt2->answer);

	if (color == 1) {
	    R_RGB_color(R, G, B);
	    set_last_color(R, G, B, RGBA_COLOR_OPAQUE);
	}
	else			/* (color==2) is "none" */
	    set_last_color(0, 0, 0, RGBA_COLOR_NONE);
    }

    if (mapcoords->answer)
	mapunits = TRUE;
    else
	mapunits = FALSE;

    if (D_get_cur_wind(window_name))
	G_fatal_error(_("No current window"));

    if (D_set_cur_wind(window_name))
	G_fatal_error(_("Current window not available"));

    if (D_get_screen_window(&t, &b, &l, &r))
	G_fatal_error(_("Getting screen window"));

    /* Finish graphics setup */
    R_set_window(t, b, l, r);
    D_setup(0);

    /* Do the graphics */
    set_graph_stuff();
    set_text_size();
    graphics(infile);

    R_text_rotation(0.0);	/* reset */
    R_line_width(0);		/* reset */

    if (opt1->answer)
	D_add_to_list(G_recreate_command());

    R_close_driver();

    exit(EXIT_SUCCESS);
}
