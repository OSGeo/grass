/*
 * MODULE:      d.legend
 *
 *              Based on the old d.leg.thin, which replaced an even older
 *              module called "d.legend".
 *
 * PURPOSE:     Draw a graphical legend for a raster on the display monitor
 *
 * AUTHORS:
 *      Original version:
 *         Bill Brown, U.S. Army Construction Engineering Research Laboratories
 *      FP Support:
 *         Radim Blazek
 *      Merge of original "d.legend" code into d.leg.thin (now this module):
 *         Markus Neteler
 *      Late 2002: Rewrite of much of the code:
 *         Hamish Bowman, Otago University, New Zealand
 *      Improvements (ticks, more label options, title):
 *         Adam Laza (part of GSoC 2016)
 *
 * COPYRIGHT:   (c) 2002-2014 by The GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "local_proto.h"


int main(int argc, char **argv)
{
    char *map_name;
    int maptype;
    int color;
    int i;
    int thin, lines, steps;
    int fp;
    int label_indent;
    int hide_catnum, hide_catstr, show_ticks, show_bg, hide_nodata, do_smooth;
    struct Categories cats;
    struct Colors colors;
    struct GModule *module;
    struct Option *opt_rast2d, *opt_rast3d, *opt_color, *opt_lines,
        *opt_thin, *opt_labelnum, *opt_at, *opt_use, *opt_range,
        *opt_font, *opt_path, *opt_charset, *opt_fontsize, *opt_title,
        *opt_ticks, *opt_tstep, *opt_brdcolor, *opt_bgcolor,
        *opt_tit_fontsize, *opt_digits, *opt_units;
    struct Flag *hidestr, *hidenum, *hidenodata, *smooth, *flipit, *histo,
        *showtick, *showbg, *log_sc;
    double X0, X1, Y0, Y1;
    int flip, UserRange;
    double UserRangeMin, UserRangeMax, UserRangeTemp;
    double *catlist;
    int catlistCount, use_catlist, ticksCount;
    double fontsize;
    char *title;
    char *units;
    double *tick_values;
    double t_step;
    int colorb, colorbg;
    double tit_fontsize;
    int log_scale, digits;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    G_add_keyword(_("legend"));
    module->description =
        _("Displays a legend for a 2D or 3D raster map in the active frame "
          "of the graphics monitor.");

    opt_rast2d = G_define_standard_option(G_OPT_R_MAP);
    opt_rast2d->key = "raster";
    opt_rast2d->required = NO;
    opt_rast2d->guisection = _("Input");

    opt_rast3d = G_define_standard_option(G_OPT_R3_MAP);
    opt_rast3d->key = "raster_3d";
    opt_rast3d->required = NO;
    opt_rast3d->guisection = _("Input");

    opt_title = G_define_option();
    opt_title->key = "title";
    opt_title->type = TYPE_STRING;
    opt_title->required = NO;
    opt_title->description = _("Legend title");
    opt_title->guisection = _("Title");

    opt_tit_fontsize = G_define_option();
    opt_tit_fontsize->key = "title_fontsize";
    opt_tit_fontsize->type = TYPE_DOUBLE;
    opt_tit_fontsize->required = NO;
    opt_tit_fontsize->options = "1-360";
    opt_tit_fontsize->label = _("Title font size");
    opt_tit_fontsize->description = _("Default: Same as fontsize");
    opt_tit_fontsize->guisection = _("Title");

    opt_lines = G_define_option();
    opt_lines->key = "lines";
    opt_lines->type = TYPE_INTEGER;
    opt_lines->answer = "0";
    opt_lines->options = "0-1000";
    opt_lines->description =
        _("Number of text lines (useful for truncating long legends)");
    opt_lines->guisection = _("Advanced");

    opt_thin = G_define_option();
    opt_thin->key = "thin";
    opt_thin->type = TYPE_INTEGER;
    opt_thin->required = NO;
    opt_thin->answer = "1";
    opt_thin->options = "1-1000";
    opt_thin->description =
        _("Thinning factor (thin=10 gives cats 0,10,20...)");
    opt_thin->guisection = _("Advanced");

    opt_units = G_define_option();
    opt_units->key = "units";
    opt_units->type = TYPE_STRING;
    opt_units->required = NO;
    opt_units->description =
            _("Units to display after labels (e.g. meters)");
    opt_units->guisection = _("Advanced");

    opt_labelnum = G_define_option();
    opt_labelnum->key = "labelnum";
    opt_labelnum->type = TYPE_INTEGER;
    opt_labelnum->answer = "5";
    opt_labelnum->options = "2-100";
    opt_labelnum->description =
        _("Number of text labels for smooth gradient legend");
    opt_labelnum->guisection = _("Gradient");

    opt_ticks = G_define_option();
    opt_ticks->key = "label_values";
    opt_ticks->type = TYPE_DOUBLE;
    opt_ticks->required = NO;
    opt_ticks->description = _("Specific values to draw ticks");
    opt_ticks->required = NO;
    opt_ticks->multiple = YES;
    opt_ticks->guisection = _("Gradient");

    opt_tstep = G_define_option();
    opt_tstep->key = "label_step";
    opt_tstep->type = TYPE_DOUBLE;
    opt_tstep->required = NO;
    opt_tstep->description = _("Display label every step");
    opt_tstep->guisection = _("Gradient");

    opt_digits = G_define_option();
    opt_digits->key = "digits";
    opt_digits->type = TYPE_INTEGER;
    opt_digits->required = NO;
    opt_digits->description = _("Number of digits after decimal point");
    opt_digits->guisection = _("Advanced");
    opt_digits->answer = NULL;
    opt_digits->options = "0-6";

    opt_at = G_define_option();
    opt_at->key = "at";
    opt_at->key_desc = "bottom,top,left,right";
    opt_at->type = TYPE_DOUBLE; /* needs to be TYPE_DOUBLE to get past options check */
    opt_at->required = NO;
    opt_at->options = "0-100";
    opt_at->label =
        _("Size and placement as percentage of screen coordinates "
          "(0,0 is lower left)");
    opt_at->description = opt_at->key_desc;
    opt_at->answer = NULL;

    opt_use = G_define_option();
    opt_use->key = "use";
    opt_use->type = TYPE_DOUBLE;        /* string as it is fed through the parser? */
    opt_use->required = NO;
    opt_use->description =
        _("List of discrete category numbers/values for legend");
    opt_use->multiple = YES;
    opt_use->guisection = _("Subset");

    opt_range = G_define_option();
    opt_range->key = "range";
    opt_range->key_desc = "min,max";
    opt_range->type = TYPE_DOUBLE;      /* should it be type_double or _string ?? */
    opt_range->required = NO;
    opt_range->description =
        _("Use a subset of the map range for the legend (min,max)");
    opt_range->guisection = _("Subset");

    opt_color = G_define_standard_option(G_OPT_C);
    opt_color->label = _("Text color");
    opt_color->guisection = _("Font settings");

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
    opt_fontsize->description = _("Default: Auto-scaled");
    opt_fontsize->guisection = _("Font settings");

    opt_path = G_define_standard_option(G_OPT_F_INPUT);
    opt_path->key = "path";
    opt_path->required = NO;
    opt_path->description = _("Path to font file");
    opt_path->gisprompt = "old_file,font,file";
    opt_path->guisection = _("Font settings");

    opt_charset = G_define_option();
    opt_charset->key = "charset";
    opt_charset->type = TYPE_STRING;
    opt_charset->required = NO;
    opt_charset->description =
        _("Text encoding (only applicable to TrueType fonts)");
    opt_charset->guisection = _("Font settings");

    opt_brdcolor = G_define_standard_option(G_OPT_CN);
    opt_brdcolor->key = "border_color";
    opt_brdcolor->answer = "black";
    opt_brdcolor->label = _("Border color");
    opt_brdcolor->guisection = _("Background");

    opt_bgcolor = G_define_standard_option(G_OPT_CN);
    opt_bgcolor->key = "bgcolor";
    opt_bgcolor->answer = "white";
    opt_bgcolor->label = _("Background color");
    opt_bgcolor->guisection = _("Background");


    hidestr = G_define_flag();
    hidestr->key = 'v';
    hidestr->description = _("Do not show category labels");
    hidestr->guisection = _("Advanced");

    hidenum = G_define_flag();
    hidenum->key = 'c';
    hidenum->description = _("Do not show category numbers");
    hidenum->guisection = _("Advanced");

    showtick = G_define_flag();
    showtick->key = 't';
    showtick->description = _("Draw legend ticks for labels");
    showtick->guisection = _("Gradient");

    hidenodata = G_define_flag();
    hidenodata->key = 'n';
    hidenodata->description = _("Skip categories with no label");
    hidenodata->guisection = _("Advanced");

    smooth = G_define_flag();
    smooth->key = 's';
    smooth->description = _("Draw smooth gradient");
    smooth->guisection = _("Gradient");

    flipit = G_define_flag();
    flipit->key = 'f';
    flipit->description = _("Flip legend");
    flipit->guisection = _("Advanced");

    histo = G_define_flag();
    histo->key = 'd';
    histo->description = _("Add histogram to smoothed legend");
    histo->guisection = _("Gradient");

    showbg = G_define_flag();
    showbg->key = 'b';
    showbg->description = _("Show background");
    showbg->guisection = _("Background");

    log_sc = G_define_flag();
    log_sc->key = 'l';
    log_sc->description = _("Use logarithmic scale");
    log_sc->guisection = _("Advanced");

    G_option_required(opt_rast2d, opt_rast3d, NULL);
    G_option_exclusive(opt_rast2d, opt_rast3d, NULL);
    G_option_exclusive(hidenum, opt_ticks, NULL);
    G_option_exclusive(hidenum, opt_tstep, NULL);

    /* Check command line */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (opt_rast2d->answer) {
        map_name = opt_rast2d->answer;
        maptype = MAP_TYPE_RASTER2D;
    }
    else {
        map_name = opt_rast3d->answer;
        maptype = MAP_TYPE_RASTER3D;
    }

    if (opt_title->answer)
        title = opt_title->answer;
    else
        title = "";

    if (opt_units->answer) {
        units = opt_units->answer;
    }
    else
        units = "";

    hide_catstr = hidestr->answer;      /* note hide_catstr gets changed and re-read below */
    hide_catnum = hidenum->answer;
    show_ticks = showtick->answer;
    hide_nodata = hidenodata->answer;
    do_smooth = smooth->answer;
    flip = flipit->answer;
    show_bg = showbg->answer;
    log_scale = log_sc->answer;

    if (showtick->answer) {
        label_indent = 12;
    }
    else
        label_indent = 6;

    if (opt_digits->answer != NULL)
        sscanf(opt_digits->answer, "%d", &digits);
    else
        digits = -1;

    color = D_parse_color(opt_color->answer, TRUE);

    if (opt_lines->answer != NULL)
        sscanf(opt_lines->answer, "%d", &lines);

    thin = 1;
    if (opt_thin->answer != NULL)
        sscanf(opt_thin->answer, "%d", &thin);
    if (!thin)
        thin = 1;

    if (opt_labelnum->answer != NULL)
        sscanf(opt_labelnum->answer, "%d", &steps);

    if ((opt_tstep->answer) || (opt_ticks->answer))
        steps = 0;

    if (opt_tstep->answer != NULL)
        t_step = atof(opt_tstep->answer);

    ticksCount = 0;
    if (opt_ticks->answer != NULL) {
        tick_values = (double *)G_calloc(100 + 1, sizeof(double));
        for (i = 0; i < 100; i++)       /* fill with dummy values */
            tick_values[i] = 1.0 * (i + 1);
        tick_values[i] = 0;

        for (i = 0; (opt_ticks->answers[i] != NULL) && i < 100; i++)
            tick_values[i] = atof(opt_ticks->answers[i]);
        ticksCount = i;
    }

    catlistCount = 0;
    if (opt_use->answer != NULL) {      /* should this be answerS ? */
        use_catlist = TRUE;

        catlist = (double *)G_calloc(100 + 1, sizeof(double));
        for (i = 0; i < 100; i++)       /* fill with dummy values */
            catlist[i] = 1.0 * (i + 1);
        catlist[i] = 0;

        for (i = 0; (opt_use->answers[i] != NULL) && i < 100; i++)
            catlist[i] = atof(opt_use->answers[i]);

        catlistCount = i;
    }
    else
        use_catlist = FALSE;


    UserRange = FALSE;
    if (opt_range->answer != NULL) {    /* should this be answerS ? */
        sscanf(opt_range->answers[0], "%lf", &UserRangeMin);
        sscanf(opt_range->answers[1], "%lf", &UserRangeMax);
        UserRange = TRUE;
        if (UserRangeMin > UserRangeMax) {
            UserRangeTemp = UserRangeMax;
            UserRangeMax = UserRangeMin;
            UserRangeMin = UserRangeTemp;
            flip = !flip;
        }
    }

    if (maptype == MAP_TYPE_RASTER2D) {
        if (Rast_read_colors(map_name, "", &colors) == -1)
            G_fatal_error(_("Color file for <%s> not available"), map_name);

        fp = Rast_map_is_fp(map_name, "");

        Rast_read_cats(map_name, "", &cats);
    }
    else {
        if (Rast3d_read_colors(map_name, "", &colors) == -1)
            G_fatal_error(_("Color file for <%s> not available"), map_name);

        fp = TRUE;              /* currently raster 3D is always floating point */

        Rast3d_read_cats(map_name, "", &cats);
    }

    if (fp && !use_catlist) {
        do_smooth = TRUE;
        /* fprintf(stderr, "FP map found - switching gradient legend on\n"); */
        flip = !flip;
    }

    D_open_driver();

    /* Parse and select background color */
    colorb = D_parse_color(opt_brdcolor->answer, TRUE);
    colorbg = D_parse_color(opt_bgcolor->answer, TRUE);

    if (opt_font->answer)
        D_font(opt_font->answer);
    else if (opt_path->answer)
        D_font(opt_path->answer);

    if (opt_fontsize->answer != NULL)
        fontsize = atof(opt_fontsize->answer);
    else
        fontsize = 12;          /* dummy placeholder, should never be called */

    if (opt_charset->answer)
        D_encoding(opt_charset->answer);

    if (opt_tit_fontsize->answer != NULL)
        tit_fontsize = atof(opt_tit_fontsize->answer);
    else
        tit_fontsize = 0;

    if (opt_at->answer != NULL) {
        sscanf(opt_at->answers[0], "%lf", &Y1);
        sscanf(opt_at->answers[1], "%lf", &Y0);
        sscanf(opt_at->answers[2], "%lf", &X0);
        sscanf(opt_at->answers[3], "%lf", &X1);
    }
    else {                      /* default */
        Y1 = 12;
        Y0 = 88;
        X0 = 3;
        X1 = 7;

        if (histo->answer) {
            X0 += 5;
            X1 += 5;
        }
    }

    if (show_bg)
        draw(map_name, maptype, color, thin, lines, steps, fp, label_indent,
             hide_catnum, hide_catstr, show_ticks, hide_nodata, do_smooth,
             cats, colors, X0, X1, Y0, Y1, flip, UserRange, UserRangeMin,
             UserRangeMax, catlist, catlistCount, use_catlist, ticksCount,
             fontsize, tit_fontsize, title, tick_values, t_step, colorb,
             colorbg, opt_use, opt_at, opt_fontsize, opt_tstep,
             opt_range, histo, hidestr, log_scale, 0, digits, units);

    draw(map_name, maptype, color, thin, lines, steps, fp, label_indent,
         hide_catnum, hide_catstr, show_ticks, hide_nodata, do_smooth, cats,
         colors, X0, X1, Y0, Y1, flip, UserRange, UserRangeMin, UserRangeMax,
         catlist, catlistCount, use_catlist, ticksCount, fontsize,
         tit_fontsize, title, tick_values, t_step, colorb, colorbg, opt_use,
         opt_at, opt_fontsize, opt_tstep, opt_range, histo,
         hidestr, log_scale, 1, digits, units);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
