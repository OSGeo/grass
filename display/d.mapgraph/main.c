/****************************************************************************
 *
 * MODULE:       d.mapgraph
 * AUTHOR(S):    James Westervelt (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      draws graphs - now superceded by d.graph
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/colors.h>
#include <grass/glocale.h>

#define MAIN
#include "options.h"
#include "local_proto.h"


struct Cell_head window ;

int 
main (int argc, char **argv)
{
	struct GModule *module;
	struct Option *opt1, *opt2/*, *opt3, *opt4*/ ;
	int R, G, B, color = 0;

	/* Initialize the GIS calls */
	G_gisinit(argv[0]) ;

	module = G_define_module();
	module->keywords = _("display");
    module->description =
		_("Generates and displays simple graphics on map "
		"layers drawn in the active graphics monitor display frame.");

	opt1 = G_define_option() ;
	opt1->key        = "input" ;
	opt1->type       = TYPE_STRING ;
	opt1->required   = NO ;
	opt1->description= _("Unix file containg graphing instructions, "
			   "if not given reads from standard input");
	opt1->gisprompt  = "old_file,file,input";

	opt2 = G_define_option() ;
	opt2->key        = "color" ;
	opt2->type       = TYPE_STRING ;
	opt2->required   = NO;
	opt2->answer     = DEFAULT_FG_COLOR ;

	opt2->description= _("Color to draw with, either a standard GRASS color "
			   "or R:G:B triplet (separated by colons)");
	opt2->gisprompt  = GISPROMPT_COLOR ;

/*
	opt3 = G_define_option() ;
	opt3->key        = "vsize" ;
	opt3->type       = TYPE_DOUBLE;
	opt3->answer     = "5.0" ;
	opt3->options    = "0-100" ;
	opt3->description= "Vertical text height as % of display frame height" ;

	opt4 = G_define_option() ;
	opt4->key        = "hsize" ;
	opt4->type       = TYPE_DOUBLE;
	opt4->answer     = "5.0" ;
	opt4->options    = "0-100" ;
	opt4->description= "Horizontal text width as % of display frame width" ;
*/

	/* Check command line */
	if (G_parser(argc, argv))
		exit(1);

	G_warning("This module is superseded. Please use 'd.graph -m' instead.");

	if (opt1->answer != NULL)
	{
		/* 1/4/91  jmoorman
		mapset = G_find_file ("mapgraph", opt1->answer, "");
		if (mapset == NULL)
		{
			G_usage() ;
			G_fatal_error("Mapgraph file [%s] not available", opt1->answer);
		}
		Infile = G_fopen_old ("mapgraph", opt1->answer, mapset);
		if (Infile == NULL)
		{
			G_usage() ;
			G_fatal_error ("Graph file <%s> not available", opt1->answer);
		}
		*/
		/* using fopen instead to facilitate finding the file */
		if ((Infile = fopen(opt1->answer,"r")) == NULL) 
		    {
			G_usage() ;
			G_fatal_error ("Mapgraph file [%s] not available", opt1->answer);
		}
	}
	else
	{
		Infile = stdin ;
		if (isatty(0))
			fprintf (stdout,"\nEnter mapgraph commands; terminate with a ^D\n\n") ;
	}


	/* Parse and select color */
	if (opt2->answer != NULL) {
	   color = G_str_to_color(opt2->answer, &R, &G, &B);
	   if(color == 0)
		G_fatal_error("[%s]: No such color", opt2->answer);
	   if(color == 1)
		R_RGB_color(R, G, B);

	   /* (color==2) is "none", noop */
	}

	/*
	sscanf(opt3->answer,"%lf",&temp);
	vsize = temp ;

	sscanf(opt4->answer,"%lf",&temp);
	hsize = temp ;
	*/

	vsize = hsize = 5.0 ;

	if (R_open_driver() != 0)
		G_fatal_error ("No graphics device selected");

	D_setup(0);

	G_get_set_window(&window) ;

	R_move_abs(
	    (int)(D_get_d_west() + D_get_d_east() / 2.0),
	    (int)(D_get_d_north() + D_get_d_south() / 2.0)) ;
	set_text_size() ;

	/* Do the graphics */
	G_setup_plot (
	    D_get_d_north(), D_get_d_south(), D_get_d_west(), D_get_d_east(),
	    D_move_abs, D_cont_abs);

	graphics () ;

	/* Add this command to list */
	/*
	if(argc > 1)
	{
		D_add_to_list(G_recreate_command()) ;
	}
	*/

	R_close_driver();
	exit (0);
}



