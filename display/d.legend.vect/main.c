/*
 ****************************************************************************
 *
 * MODULE:       d.vect.legend
 * AUTHOR(S):    Adam Laza, CTU, GSoC 2016
 * PURPOSE:      Display a vector layer
 * COPYRIGHT:    (C) 2007-2016 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char **argv)
{
    struct GModule *module;
/*    struct Option *opt_input, *opt_sep; */
    struct Option *opt_at, *opt_cols, *opt_font, *opt_fontsize,
            *opt_fontcolor, *opt_title, *opt_tit_font, *opt_tit_fontsize, *opt_sub_font,
            *opt_sub_fontsize, *opt_bcolor, *opt_bgcolor, *opt_symb_size,
            *opt_bg_width, *opt_output, *opt_input, *opt_sep;
    struct Flag *fl_bg;

    double LL, LT;
    char *title, *file_name;
    int bcolor, bgcolor, do_bg;
    int fontsize, fontcolor, tit_size, sub_size;
    char *font, *tit_font, *sub_font;
    int cols, symb_size, bg_width;
    char *out_file;
    FILE *source;
    char buf[BUFFSIZE];
    char *sep;
    size_t nread;


    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    G_add_keyword(_("vector"));
    G_add_keyword(_("legend"));
    module->description =
    _("Displays a vector legend "
      "in the active graphics frame.");

    opt_at = G_define_option();
    opt_at->key = "at";
    opt_at->key_desc = "left,top";
    opt_at->type = TYPE_DOUBLE;
    opt_at->options = "0-100";
    opt_at->answer = "10,40";
    opt_at->required = NO;
    opt_at->description =
    _("Screen position of legend to be drawn (percentage, [0,0] is lower left)");

    opt_cols = G_define_option();
    opt_cols->key = "columns";
    opt_cols->type = TYPE_INTEGER;
    opt_cols->answer = "1";
    opt_cols->required = NO;
    opt_cols->description =
    _("Number of legend columns");
    opt_cols->guisection = _("Layout");

    opt_title = G_define_option();
    opt_title->key = "title";
    opt_title->type = TYPE_STRING;
    opt_title->required = NO;
    opt_title->description = _("Legend title");
    opt_title->guisection = _("Title");

    opt_symb_size = G_define_option();
    opt_symb_size->key = "symbol_size";
    opt_symb_size->type = TYPE_INTEGER;
    opt_symb_size->required = NO;
    opt_symb_size->description = _("Symbol size");
    opt_symb_size->answer = "20";
    opt_symb_size->guisection = _("Layout");

    opt_bcolor = G_define_standard_option(G_OPT_CN);
    opt_bcolor->key = "border_color";
    opt_bcolor->answer = "black";
    opt_bcolor->label = _("Border color");
    opt_bcolor->guisection = _("Background");

    opt_bgcolor = G_define_standard_option(G_OPT_CN);
    opt_bgcolor->key = "bgcolor";
    opt_bgcolor->answer = "white";
    opt_bgcolor->label = _("Background color");
    opt_bgcolor->guisection = _("Background");

    opt_bg_width = G_define_option();
    opt_bg_width->type = TYPE_INTEGER;
    opt_bg_width->key = "border_width";
    opt_bg_width->answer = "2";
    opt_bg_width->label = _("Background border width");
    opt_bg_width->guisection = _("Background");

    opt_font = G_define_option();
    opt_font->key = "font";
    opt_font->type = TYPE_STRING;
    opt_font->required = NO;
    opt_font->description = _("Font name");
    opt_font->guisection = _("Font settings");

    opt_fontsize = G_define_option();
    opt_fontsize->key = "fontsize";
    opt_fontsize->type = TYPE_DOUBLE;
    opt_fontsize->required = NO;
    opt_fontsize->options = "1-360";
    opt_fontsize->label = _("Font size");
    opt_fontsize->description = _("Default: 12");
    opt_fontsize->guisection = _("Font settings");

    opt_tit_font = G_define_option();
    opt_tit_font->key = "title_font";
    opt_tit_font->type = TYPE_STRING;
    opt_tit_font->required = NO;
    opt_tit_font->description = _("Title font name");
    opt_tit_font->guisection = _("Font settings");

    opt_tit_fontsize = G_define_option();
    opt_tit_fontsize->key = "title_fontsize";
    opt_tit_fontsize->type = TYPE_DOUBLE;
    opt_tit_fontsize->required = NO;
    opt_tit_fontsize->options = "1-360";
    opt_tit_fontsize->label = _("Title font size");
    opt_tit_fontsize->description = _("Default: 18");
    opt_tit_fontsize->guisection = _("Title");

    opt_sub_font = G_define_option();
    opt_sub_font->key = "sub_font";
    opt_sub_font->type = TYPE_STRING;
    opt_sub_font->required = NO;
    opt_sub_font->description = _("Subtitle font name");
    opt_sub_font->guisection = _("Font settings");

    opt_sub_fontsize = G_define_option();
    opt_sub_fontsize->key = "sub_fontsize";
    opt_sub_fontsize->type = TYPE_DOUBLE;
    opt_sub_fontsize->required = NO;
    opt_sub_fontsize->options = "1-360";
    opt_sub_fontsize->label = _("Subtitle font size");
    opt_sub_fontsize->description = _("Default: 14");
    opt_sub_fontsize->guisection = _("Font settings");

    opt_fontcolor = G_define_standard_option(G_OPT_C);
    opt_fontcolor->key = "fontcolor";
    opt_fontcolor->answer = "black";
    opt_fontcolor->label = _("Font color");
    opt_fontcolor->guisection = _("Font settings");

    fl_bg = G_define_flag();
    fl_bg->key = 'b';
    fl_bg->description = _("Display legend background");
    fl_bg->guisection = _("Background");

    opt_sep = G_define_standard_option(G_OPT_F_SEP);
    opt_sep->guisection = _("In/Out");
    opt_sep->label = _("Field separator for input file");

    opt_input = G_define_standard_option(G_OPT_F_INPUT);
    opt_input->label = _("Input legend file");
    opt_input->description = _("Path to legend file ");
    opt_input->required = NO;
    opt_input->guisection = _("In/Out");

    opt_output = G_define_standard_option(G_OPT_F_OUTPUT);
    opt_output->label = _("Output csv file");
    opt_output->description = _("Path to output file or '-' "
                                "for standard output");
    opt_output->required = NO;
    opt_output->guisection = _("In/Out");


    /* Check command line */
    if (G_parser(argc, argv)) {
        exit(EXIT_FAILURE);
    }

    D_open_driver();
    D_setup_unity(0);

    /* parse and check options and flags */
    if (opt_at->answer) {
        sscanf(opt_at->answers[0], "%lf", &LL);
        sscanf(opt_at->answers[1], "%lf", &LT);
    }
    else {
        LL = 10;
        LT = 40;
    }

    if (opt_title->answer)
        title = opt_title->answer;
    else
        title = "";

    if (opt_cols->answer)
        sscanf(opt_cols->answer, "%d", &cols);
    else
        cols = 1;

    sscanf(opt_symb_size->answer, "%d", &symb_size);
    sscanf(opt_bg_width->answer, "%d", &bg_width);

    /* Background */
    do_bg = fl_bg->answer;
    bcolor = D_parse_color(opt_bcolor->answer, TRUE);
    bgcolor = D_parse_color(opt_bgcolor->answer, TRUE);

    /* Font settings */
    if (opt_font->answer)
        font = opt_font->answer;
    else
        font = "sans";
    if (opt_fontsize->answer != NULL)
        sscanf(opt_fontsize->answer, "%d", &fontsize);
    else
        fontsize = 14;

    if (opt_tit_font->answer)
        tit_font = opt_tit_font->answer;
    else
        tit_font = font;
    if (opt_tit_fontsize->answer)
        sscanf(opt_tit_fontsize->answer, "%d", &tit_size);
    else
        tit_size = fontsize;

    if (opt_sub_font->answer)
        sub_font = opt_sub_font->answer;
    else
        sub_font = font;
    if (opt_sub_fontsize->answer)
        sscanf(opt_sub_fontsize->answer, "%d", &sub_size);
    else
        sub_size = fontsize;

    fontcolor = D_parse_color(opt_fontcolor->answer, FALSE); /*default color: black */

    /* I/O */
    if (opt_input->answer) {
        sep = G_option_to_separator(opt_sep);
        file_name = opt_input->answer;
        if (!file_name)
            G_fatal_error(_("Unable to open input file <%s>"), file_name);
    }
    else {
        sep = "|";
        file_name = getenv("GRASS_LEGEND_FILE");
        if (!file_name)
            G_fatal_error("No legend file defined.");
    }


    if (opt_output->answer) {
        if (strcmp(opt_output->answer,"-") == 0) {
            source = fopen(file_name, "r");
            if (!source)
                G_fatal_error(_("Unable to open input file <%s>"), file_name);
            while ((nread = fread(buf, 1, sizeof(buf), source)))
                fwrite(buf, 1, nread, stdout);
            fclose(source);
        }
        else {
            out_file = opt_output->answer;
            G_copy_file(file_name, out_file);
        }
    }

    /* Pre-calculate the layout */
    if (do_bg)
        draw(file_name, LL, LT, title, cols, bgcolor, bcolor, bg_width, 1, tit_font, tit_size, sub_font, sub_size, font, fontsize, fontcolor, symb_size, sep);

    /* Draw legend */
    draw(file_name, LL, LT, title, cols, bgcolor, bcolor, bg_width, 0, tit_font, tit_size, sub_font, sub_size, font, fontsize, fontcolor, symb_size, sep);

    D_close_driver();

    exit(EXIT_SUCCESS);
}
