/****************************************************************************
 *
 * MODULE:       d.northarrow
 *
 * AUTHOR(S):    Hamish Bowman, Dunedin, NZ <hamish_b yahoo.com>
 *
 * PURPOSE:      Displays a north arrow on graphics monitor
 *
 * COPYRIGHT:    (C) 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "options.h"

int fg_color, bg_color;
int do_background = TRUE;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *bg_color_opt, *fg_color_opt, *coords, *n_arrow, *fsize,
		 *width_opt;
    struct Flag *no_text;
    double east, north;
    double fontsize, line_width;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description = _("Displays a north arrow on the graphics monitor.");

    n_arrow = G_define_option();
    n_arrow->key = "style";
    n_arrow->description = _("North arrow style");
    n_arrow->options = "1a,1b,2,3,4,5,6,7a,7b,8a,8b,9,fancy_compass,compass";
    n_arrow->answer = "1a";
    n_arrow->guisection = _("Style");

    coords = G_define_option();
    coords->key = "at";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->answer = "85.0,15.0";
    coords->options = "0-100";
    coords->label =
	_("Screen coordinates of the rectangle's top-left corner");
    coords->description = _("(0,0) is lower-left of the display frame");

    fg_color_opt = G_define_standard_option(G_OPT_C_FG);
    fg_color_opt->label = _("Line color");
    fg_color_opt->guisection = _("Colors");

    bg_color_opt = G_define_standard_option(G_OPT_C_BG);
    bg_color_opt->key = "fill_color";
    bg_color_opt->label = _("Fill color");
    bg_color_opt->answer = _("black");
    bg_color_opt->guisection = _("Colors");

    width_opt = G_define_option();
    width_opt->key = "width";
    width_opt->type = TYPE_DOUBLE;
    width_opt->answer = "0";
    width_opt->description = _("Line width");

    fsize = G_define_option();
    fsize->key = "fontsize";
    fsize->type = TYPE_DOUBLE;
    fsize->required = NO;
    fsize->answer = "14";
    fsize->options = "1-360";
    fsize->description = _("Font size");
    fsize->guisection = _("Text");
    
    no_text = G_define_flag();
    no_text->key = 't';
    no_text->description = _("Draw the symbol without text");
    no_text->guisection = _("Text");

/* TODO:
     - add rotation= option to rotate the north arrow by an arbitrary amount.
       do a bit of trig to figure out where to put the "N" (and rotate it too).
     - add a -n flag to rotate to match true north instead of grid north.
       Similar to 'g.region -n' but use the at=x,y coord for the convergence
       angle calc. (assuming that's the center of the icon)
 */


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    sscanf(coords->answers[0], "%lf", &east);
    sscanf(coords->answers[1], "%lf", &north);

    fontsize = atof(fsize->answer);
    if (no_text->answer)
	fontsize = -1;

    /* Parse and select foreground color */
    fg_color = D_parse_color(fg_color_opt->answer, 0);

    /* Parse and select background color */
    bg_color = D_parse_color(bg_color_opt->answer, 1);
    if (bg_color == 0)
	do_background = FALSE;

    line_width = atof(width_opt->answer);
    if (line_width < 0)
        line_width = 0;
    else if (line_width > 72)
        line_width = 72;


    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    D_setup(0);


    draw_n_arrow(east, north, fontsize, n_arrow->answer, line_width);


    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
