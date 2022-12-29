
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
 *               Major rewrite for GRASS 7 by Hamish Bowman, June 2013
 *               Adam Laza <ad.laza32@gmail.com>, GSoC 2016
 *               Anna Petrasova <kratochanna gmail.com>, width_scale added
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
int north_arrow;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *bg_color_opt, *fg_color_opt, *coords, *fsize, *barstyle,
            *text_placement, *length_opt, *segm_opt, *units_opt, *label_opt,
            *width_scale_opt, *font, *path, *charset;
    struct Flag *feet, *no_text, *n_symbol;
    struct Cell_head W;
    double east, north;
    double fontsize;
    int bar_style, text_position, units;
    double length;
    int segm;
    char *label;
    double width_scale;

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

    n_symbol = G_define_flag();
    n_symbol->key = 'n';
    n_symbol->description = _("Display north-arrow symbol.");
    n_symbol->guisection = _("Style");

    barstyle = G_define_option();
    barstyle->key = "style";
    barstyle->description = _("Type of barscale to draw");
    barstyle->options =
        "classic,line,solid,hollow,full_checker,part_checker,mixed_checker,tail_checker,up_ticks,down_ticks,both_ticks,arrow_ends";
    barstyle->answer = "classic";
    barstyle->gisprompt = "old,barscale,barscale";
    barstyle->guisection = _("Style");
    G_asprintf((char **)&(barstyle->descriptions),
               "classic;%s;"
               "line;%s;"
               "solid;%s;"
               "hollow;%s;"
               "full_checker;%s;"
               "part_checker;%s;"
               "mixed_checker;%s;"
               "tail_checker;%s;"
               "up_ticks;%s;"
               "down_ticks;%s;"
               "both_ticks;%s;"
               "arrow_ends;%s",
               _("Classic style"),
               _("Line style"),
               _("Solid style"),
               _("Hollow style"),
               _("Full checker style"),
               _("Part checker style"),
               _("Mixed checker style"),
               _("Tail checker style"),
               _("Up ticks style"),
               _("Down ticks style"),
               _("Both ticks style"), _("Arrow ends style"));

    coords = G_define_option();
    coords->key = "at";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->answer = "0.0,10.0";
    coords->options = "0-100";
    coords->label =
        _("Screen coordinates of the rectangle's top-left corner");
    coords->description = _("(0,0) is lower-left of the display frame");

    length_opt = G_define_option();
    length_opt->key = "length";
    length_opt->key_desc = "integer";
    length_opt->type = TYPE_INTEGER;
    length_opt->answer = "0";
    length_opt->options = "0-";
    length_opt->label = _("Length of barscale in map units");

    units_opt = G_define_option();
    units_opt->key = "units";
    units_opt->description = _("Barscale units to display");
    units_opt->options = "meters, kilometers, feet, miles";

    label_opt = G_define_option();
    label_opt->key = "label";
    label_opt->description = _("Custom label of unit");
    label_opt->type = TYPE_STRING;
    label_opt->guisection = _("Text");

    segm_opt = G_define_option();
    segm_opt->key = "segment";
    segm_opt->type = TYPE_INTEGER;
    segm_opt->answer = "10";
    segm_opt->options = "1-100";
    segm_opt->label = _("Number of segments");
    segm_opt->guisection = _("Style");

    fg_color_opt = G_define_standard_option(G_OPT_C);
    fg_color_opt->label = _("Bar scale and text color");
    fg_color_opt->guisection = _("Colors");

    bg_color_opt = G_define_standard_option(G_OPT_CN);
    bg_color_opt->key = "bgcolor";
    bg_color_opt->answer = "white";
    bg_color_opt->label = _("Background color (drawn behind the bar)");
    bg_color_opt->guisection = _("Colors");

    text_placement = G_define_option();
    text_placement->key = "text_position";
    text_placement->description = _("Text position");
    text_placement->options = "under,over,left,right";
    text_placement->answer = "right";
    text_placement->guisection = _("Text");
    
    width_scale_opt = G_define_option();
    width_scale_opt->key = "width_scale";
    width_scale_opt->type = TYPE_DOUBLE;
    width_scale_opt->required = NO;
    width_scale_opt->answer = "1";
    width_scale_opt->options = "0.5-100";
    width_scale_opt->description = _("Scale factor to change bar width");

    font = G_define_option();
    font->key = "font";
    font->type = TYPE_STRING;
    font->required = NO;
    font->description = _("Font name");
    font->guisection = _("Text");

    fsize = G_define_option();
    fsize->key = "fontsize";
    fsize->type = TYPE_DOUBLE;
    fsize->required = NO;
    fsize->answer = "12";
    fsize->options = "1-360";
    fsize->description = _("Font size");
    fsize->guisection = _("Text");

    path = G_define_standard_option(G_OPT_F_INPUT);
    path->key = "path";
    path->required = NO;
    path->description = _("Path to font file");
    path->gisprompt = "old,font,file";
    path->guisection = _("Font settings");

    charset = G_define_option();
    charset->key = "charset";
    charset->type = TYPE_STRING;
    charset->required = NO;
    charset->description =
	_("Text encoding (only applicable to TrueType fonts)");
    charset->guisection = _("Text");

    G_option_exclusive(feet, units_opt, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);


    G_get_window(&W);
    if (W.proj == PROJECTION_LL)
        G_fatal_error(_("%s does not work with a latitude-longitude location"),
                      argv[0]);


    north_arrow = n_symbol->answer ? TRUE : FALSE;

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


    length = atof(length_opt->answer);
    sscanf(segm_opt->answer, "%d", &segm);

    if (feet->answer == 1){
        use_feet = 1;
        units = U_FEET;
        label = "ft";
    }
    else {
        if (!units_opt->answer)
            units = G_database_unit();
        else
            units = G_units(units_opt->answer);
        switch (units) {
        case U_METERS:
            label = "m";
            break;
        case U_KILOMETERS:
            label = "km";
            break;
        case U_FEET:
            use_feet = 1;
            label = "ft";
            break;
        case U_USFEET:
            use_feet = 1;
            label = "ft";
            break;
        case U_MILES:
            use_feet = 1;
            label = "mi";
            break;
        default:
            units = U_METERS;
            label = "m";
        }
    }

    if (label_opt->answer){
        label = label_opt->answer;
    }

    fontsize = atof(fsize->answer);
    if (no_text->answer)
        fontsize = -1;

    width_scale = atof(width_scale_opt->answer);

    /* Parse and select foreground color */
    fg_color = D_parse_color(fg_color_opt->answer, 0);

    /* Parse and select background color */
    bg_color = D_parse_color(bg_color_opt->answer, 1);
    if (bg_color == 0)
        do_background = FALSE;


    D_open_driver();

    if (font->answer)
	D_font(font->answer);
    else if (path->answer)
	D_font(path->answer);
    if (charset->answer)
	D_encoding(charset->answer);

    D_setup(0);

    draw_scale(east, north, length, segm, units, label, bar_style, text_position, width_scale, fontsize);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
