
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
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "options.h"

int fg_color, bg_color, text_color;
//int do_background = TRUE;

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *bg_color_opt, *fg_color_opt, *coords, *n_arrow, *fsize,
        *width_opt, *rotation_opt, *lbl_opt, *text_color_opt, *font, *path, *charset;
    struct Flag *no_text, *rotate_text, *rads;
    double east, north;
    double rotation;
    double fontsize, line_width;
    int rot_with_text;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    module->description =
        _("Displays a north arrow on the graphics monitor.");

    n_arrow = G_define_option();
    n_arrow->key = "style";
    n_arrow->description = _("North arrow style");
    n_arrow->options =
        "1a,1b,2,3,4,5,6,7a,7b,8a,8b,9,fancy_compass,basic_compass,arrow1,arrow2,arrow3,star";
    G_asprintf((char **)&(n_arrow->descriptions),
               "1a;%s;" "1b;%s;" "2;%s;" "3;%s;" "4;%s;" "5;%s;" "6;%s;"
               "7a;%s;" "7b;%s;" "8a;%s;" "8b;%s;" "9;%s;" "fancy_compass;%s;"
               "basic_compass;%s;" "arrow1;%s;" "arrow2;%s;" "arrow3;%s;"
               "star;%s;",
               _("Two color arrowhead"),
               _("Two color arrowhead with circle"),
               _("Narrow with blending N"), _("Long with small arrowhead"),
               _("Inverted narrow inside a circle"),
               _("Triangle and N inside a circle"),
               _("Arrowhead and N inside a circle"),
               _("Tall half convex arrowhead"),
               _("Tall half concave arrowhead"), _("Thin arrow in a circle"),
               _("Fat arrow in a circle"), _("One color arrowhead"),
               _("Fancy compass"), _("Basic compass"), _("Simple arrow"),
               _("Thin arrow"), _("Fat arrow"), _("4-point star"));
    n_arrow->answer = "1a";
    n_arrow->guisection = _("Style");
    n_arrow->gisprompt = "old,northarrow,northarrow";

    coords = G_define_option();
    coords->key = "at";
    coords->key_desc = "x,y";
    coords->type = TYPE_DOUBLE;
    coords->answer = "85.0,15.0";
    coords->options = "0-100";
    coords->label =
        _("Screen coordinates of the rectangle's top-left corner");
    coords->description = _("(0,0) is lower-left of the display frame");

    rotation_opt = G_define_option();
    rotation_opt->key = "rotation";
    rotation_opt->type = TYPE_DOUBLE;
    rotation_opt->required = NO;
    rotation_opt->answer = "0";
    rotation_opt->description =
        _("Rotation angle in degrees (counter-clockwise)");

    lbl_opt = G_define_option();
    lbl_opt->key = "label";
    lbl_opt->required = NO;
    lbl_opt->answer = "N";
    lbl_opt->description =
        _("Displayed letter on the top of arrow");
    lbl_opt->guisection = _("Text");

    fg_color_opt = G_define_standard_option(G_OPT_CN);
    fg_color_opt->label = _("Line color");
    fg_color_opt->guisection = _("Colors");

    bg_color_opt = G_define_standard_option(G_OPT_CN);
    bg_color_opt->key = "fill_color";
    bg_color_opt->label = _("Fill color");
    bg_color_opt->guisection = _("Colors");

    text_color_opt = G_define_standard_option(G_OPT_C);
    text_color_opt->key = "text_color";
    text_color_opt->label = _("Text color");
    text_color_opt->answer = NULL;
    text_color_opt->guisection = _("Colors");

    width_opt = G_define_option();
    width_opt->key = "width";
    width_opt->type = TYPE_DOUBLE;
    width_opt->answer = "0";
    width_opt->description = _("Line width");

    font = G_define_option();
    font->key = "font";
    font->type = TYPE_STRING;
    font->required = NO;
    font->description = _("Font name");
    font->guisection = _("Text");

    fsize = G_define_option();
    fsize->key = "fontsize"; /* size in d.text */
    fsize->type = TYPE_DOUBLE;
    fsize->required = NO;
    fsize->answer = "14";
    fsize->options = "1-360";
    fsize->description = _("Font size");
    fsize->guisection = _("Text");

    path = G_define_standard_option(G_OPT_F_INPUT);
    path->key = "path";
    path->required = NO;
    path->description = _("Path to font file");
    path->gisprompt = "old,font,file";
    path->guisection = _("Font settings");

    no_text = G_define_flag();
    no_text->key = 't';
    no_text->description = _("Draw the symbol without text");
    no_text->guisection = _("Text");

    charset = G_define_option();
    charset->key = "charset";
    charset->type = TYPE_STRING;
    charset->required = NO;
    charset->description =
	_("Text encoding (only applicable to TrueType fonts)");
    charset->guisection = _("Text");

    rotate_text = G_define_flag();
    rotate_text->key = 'w';
    rotate_text->description = _("Do not rotate text with symbol");
    rotate_text->guisection = _("Text");

    rads = G_define_flag();
    rads->key = 'r';
    rads->description = _("Use radians instead of degrees for rotation");

    /* TODO:
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

    rot_with_text = 0;
    if (!rotate_text->answer)
        rot_with_text = 1;

    /* Convert to radians */
    rotation = atof(rotation_opt->answer);
    if (!rads->answer)
        rotation *= M_PI / 180.0;
    rotation = fmod(rotation, 2.0 * M_PI);
    if (rotation < 0.0)
        rotation += 2.0 * M_PI;

    /* Parse and select foreground color */
    fg_color = D_parse_color(fg_color_opt->answer, 1);

    /* Parse and select background color */
    bg_color = D_parse_color(bg_color_opt->answer, 1);

    /* Parse and select text color */
    if (text_color_opt->answer)
        text_color = D_parse_color(text_color_opt->answer, 0);
    else if (strcmp(fg_color_opt->answer, "none") != 0)
        text_color = D_parse_color(fg_color_opt->answer, 1);
    else if (strcmp(bg_color_opt->answer, "none") != 0)
        text_color = D_parse_color(bg_color_opt->answer, 1);
    else
        text_color = 0;

    line_width = atof(width_opt->answer);
    if (line_width < 0)
        line_width = 0;
    else if (line_width > 72)
        line_width = 72;

    D_open_driver();

    if (font->answer)
	D_font(font->answer);
    else if (path->answer)
	D_font(path->answer);
    if (charset->answer)
	D_encoding(charset->answer);

    draw_n_arrow(east, north, rotation, lbl_opt->answer, rot_with_text,
                 fontsize, n_arrow->answer, line_width);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
