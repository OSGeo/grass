/*
 ****************************************************************************
 *
 * MODULE:       d.erase
 * AUTHOR(S):    James Westervelt - USA CERL
 * PURPOSE:      Erase the current display frame with user defined color.
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int main(int argc,char *argv[])
{
	struct Option *color;
	struct Flag *eraseframe, *dontaddtolist;
	struct GModule *module;
	char name[256], buf[128];
	char *err;

	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("display");
    module->description = 
	  _("Erase the contents of the active display frame with user defined color");

	color = G_define_option();
	color->key = "color";
	color->type = TYPE_STRING;
	color->required = NO;
	color->answer = DEFAULT_BG_COLOR;
	color->description =
	   _("Color to erase with, either a standard GRASS color or R:G:B triplet (separated by colons)");
	color->gisprompt = GISPROMPT_COLOR;

	eraseframe = G_define_flag();
	eraseframe->key = 'f';
	eraseframe->description = _("Remove all frames and erase the screen");

	dontaddtolist = G_define_flag();
	dontaddtolist->key = 'x';
	dontaddtolist->description = _("Don't add to list of commands in monitor");

	if (argc > 1 && G_parser(argc, argv))
		exit(1);

	if (R_open_driver() != 0)
		G_fatal_error (_("No graphics device selected"));

	err = NULL;
	if (D_get_cur_wind(name))
		err = _("No current frame") ;
	else if (D_set_cur_wind(name))
		err = _("Current frame not available") ;
	else {
		/* D_setup(1);
		R_standard_color(D_translate_color(color->answer));
		D_erase_window(); */
		D_erase(color->answer) ;
		
		if (eraseframe->answer){
			D_full_screen();
		}
		if (!dontaddtolist->answer) {
			sprintf(buf, "d.erase color=%s", color->answer);
			D_add_to_list(buf);
		}
	}

	R_close_driver();
	if (err)
		G_fatal_error(err) ;

	exit(0);
}
