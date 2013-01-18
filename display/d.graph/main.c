
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
 *               Hamish Bowman <hamish_b yahoo.com>, 
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
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

#include "options.h"
#include "local_proto.h"

float hsize;
float vsize;
int mapunits;
FILE *infile;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *opt1, *opt2;
    struct Flag *mapcoords;
    int R, G, B, color = 0;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description =
	_("Program for generating and displaying simple graphics on the "
	  "display monitor.");

    opt1 = G_define_standard_option(G_OPT_F_INPUT);
    opt1->required = NO;
    opt1->description = _("Name of file containing graphics commands, "
			  "if not given reads from standard input");
    
    opt2 = G_define_option();
    opt2->key = "color";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->description = _("Color to draw with, either a standard GRASS color "
			  "or R:G:B triplet");
    opt2->answer = DEFAULT_FG_COLOR;
    opt2->gisprompt = "old_color,color,color";

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
    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    /* Parse and select color */
    if (opt2->answer != NULL) {
	color = G_str_to_color(opt2->answer, &R, &G, &B);

	if (color == 0)
	    G_fatal_error(_("[%s]: No such color"), opt2->answer);

	if (color == 1) {
	    D_RGB_color(R, G, B);
	    set_last_color(R, G, B, RGBA_COLOR_OPAQUE);
	}
	else			/* (color==2) is "none" */
	    set_last_color(0, 0, 0, RGBA_COLOR_NONE);
    }

    if (mapcoords->answer) {
	mapunits = TRUE;
	D_setup(0);
    }
    else {
	D_setup2(0, 0, 100, 0, 0, 100);
	mapunits = FALSE;
    }

    /* Do the graphics */
    set_graph_stuff();
    set_text_size();
    graphics(infile);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
