/****************************************************************************
 *
 * MODULE:       d.barscale
 *
 * AUTHOR(S):    unknown but from CERL code (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 *		 Major rewrite for GRASS 7 by Hamish Bowman, June 2013
 *
 * PURPOSE:      Displays a barscale on graphics monitor
 *
 * COPYRIGHT:    (C) 1999-2013 by the GRASS Development Team
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
int use_feet;
int do_background = TRUE;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *bg_color_opt, *fg_color_opt, *coords, *fsize,
		 *barstyle, *text_placement;
    struct Flag *feet, *no_text;
    struct Cell_head W;
    double east, north;
    double fontsize;
    int bar_style, text_position;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description = _("Displays a barscale on the graphics monitor.");

    feet = G_define_flag();
    feet->key = 'f';
    feet->description = _("Use feet/miles instead of meters");

    no_text = G_define_flag();
    no_text->key = 't';
    no_text->description = _("Draw the scale bar without text");
    no_text->guisection = _("Text");

    barstyle = G_define_option();
    barstyle->key = "style";
    barstyle->description = _("Type of barscale to draw");
    barstyle->options =
	"classic,line,solid,hollow,full_checker,part_checker,mixed_checker,tail_checker,up_ticks,down_ticks,both_ticks,arrow_ends";
    barstyle->answer = "classic";
    barstyle->guisection = _("Style");

    coords = G_define_option();
    coords->key = "at";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->answer = "0.0,5.0";
    coords->options = "0-100";
    coords->label =
	_("Screen coordinates of the rectangle's top-left corner");
    coords->description = _("(0,0) is lower-left of the display frame");

    fg_color_opt = G_define_standard_option(G_OPT_C_FG);
    fg_color_opt->label = _("Bar scale and text color");
    fg_color_opt->guisection = _("Colors");

    bg_color_opt = G_define_standard_option(G_OPT_C_BG);
    bg_color_opt->label = _("Background color (drawn behind the bar)");
    bg_color_opt->guisection = _("Colors");

    text_placement = G_define_option();
    text_placement->key = "text_position";
    text_placement->description = _("Text position");
    text_placement->options = "under,over,left,right";
    text_placement->answer = "right";
    text_placement->guisection = _("Text");

    fsize = G_define_option();
    fsize->key = "fontsize";
    fsize->type = TYPE_DOUBLE;
    fsize->required = NO;
    fsize->answer = "12";
    fsize->options = "1-360";
    fsize->description = _("Font size");
    fsize->guisection = _("Text");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    G_get_window(&W);
    if (W.proj == PROJECTION_LL)
	G_fatal_error(_("%s does not work with a latitude-longitude location"),
		      argv[0]);

    use_feet = feet->answer ? TRUE : FALSE;

    switch (barstyle->answer[0]) {
    case 'c':
	bar_style = STYLE_CLASSIC_BAR;
	break;
    case 'p':
	bar_style = STYLE_PART_CHECKER;
	break;
    case 'f':
	bar_style = STYLE_FULL_CHECKER;
	break;
    case 'm':
	bar_style = STYLE_MIXED_CHECKER;
	break;
    case 't':
	bar_style = STYLE_TAIL_CHECKER;
	break;
    case 'l':
	bar_style = STYLE_THIN_WITH_ENDS;
	break;
    case 's':
	bar_style = STYLE_SOLID_BAR;
	break;
    case 'h':
	bar_style = STYLE_HOLLOW_BAR;
	break;
    case 'u':
	bar_style = STYLE_TICKS_UP;
	break;
    case 'd':
	bar_style = STYLE_TICKS_DOWN;
	break;
    case 'b':
	bar_style = STYLE_TICKS_BOTH;
	break;
    case 'a':
	bar_style = STYLE_ARROW_ENDS;
	break;
    default:
	G_fatal_error(_("Programmer error"));
    }

    switch (text_placement->answer[0]) {
    case 'u':
	text_position = TEXT_UNDER;
	break;
    case 'o':
	text_position = TEXT_OVER;
	break;
    case 'l':
	text_position = TEXT_LEFT;
	break;
    case 'r':
	text_position = TEXT_RIGHT;
	break;
    default:
	G_fatal_error(_("Programmer error"));
    }

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


    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    D_setup(0);


    draw_scale(east, north, bar_style, text_position, fontsize);


    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
