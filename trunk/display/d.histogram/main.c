
/****************************************************************************
 *
 * MODULE:       d.histogram
 * AUTHOR(S):    Dave Johnson, DBA Systems, Inc. (original contributor)
 *               10560 Arrowhead Drive Fairfax, Virginia 22030
 *               Markus Neteler <neteler itc.it> 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      draw a bar-chart or a pie-chart representing the
 *               histogram statistics of a cell-file
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/******************************************************************************
 * NOTE (shapiro):
 *  This program can NOT handle area information.
 *  Areas (as output by the r.stats command) are doubles.
 *  This program was written assuming areas are integers.
 *
 *  The area option has been #ifdef'ed out of the code until someone
 *   upgrades both the get_stats() and the pie() and bar() routines
 *   as well as the struct stat_list (defined in dhist.h).
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

#include "options.h"
#include "dhist.h"

struct stat_list dist_stats;
struct Categories cats;
struct FPRange fp_range;
int is_fp;

char *map_name;
int color;
float size;
int style;
int type;
int is_fp;
int nodata;
int nsteps;
int cat_ranges;

int main(int argc, char **argv)
{
    int text_height;
    int text_width;
    struct Categories cats;
    struct Range range;
    struct Colors pcolors;
    char title[GNAME_MAX];
    double tt, tb, tl, tr;
    double t, b, l, r;
    struct GModule *module;
    struct Option *opt1;
    struct Option *opt2, *bg_opt;
    struct Option *opt4;
    struct Option *opt5;
    struct Flag *flag1;
    struct Flag *flag2;
    struct Flag *flag3;


    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("histogram"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Displays a histogram in the form of a pie or bar chart "
	  "for a user-specified raster map.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);
    opt1->description = _("Raster map for which histogram will be displayed");

    opt4 = G_define_option();
    opt4->key = "style";
    opt4->description = _("Indicate if a pie or bar chart is desired");
    opt4->type = TYPE_STRING;
    opt4->required = NO;
    opt4->options = "pie,bar";
    opt4->answer = "bar";

    /* The color option specifies the color for the labels, tic-marks,
     * and borders of the chart. */
    opt2 = G_define_standard_option(G_OPT_C);
    opt2->label = _("Color for text and axes");

    bg_opt = G_define_standard_option(G_OPT_CN);
    bg_opt->key = "bgcolor";
    bg_opt->label = _("Background color");
    bg_opt->answer = DEFAULT_BG_COLOR;

#ifdef CAN_DO_AREAS
    opt3 = G_define_option();
    opt3->key = "type";
    opt3->description =
	_("Indicate if cell counts or map areas should be displayed");
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->answer = "count";
    opt3->options = "count,area";
#endif

    opt5 = G_define_option();
    opt5->key = "nsteps";
    opt5->description =
	_("Number of steps to divide the data range into (fp maps only)");
    opt5->type = TYPE_INTEGER;
    opt5->required = NO;
    opt5->answer = "255";

    flag1 = G_define_flag();
    flag1->key = 'n';
    flag1->description = _("Display information for null cells");

    flag3 = G_define_flag();
    flag3->key = 'c';
    flag3->description =
	_("Report for ranges defined in cats file (fp maps only)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    map_name = opt1->answer;

    color = D_parse_color(opt2->answer, FALSE);

    type = COUNT;
#ifdef CAN_DO_AREAS
    if (strcmp(opt3->answer, "count") == 0)
	type = COUNT;
    else
	type = AREA;
#endif

    if (strcmp(opt4->answer, "bar") == 0)
	style = BAR;
    else
	style = PIE;

    if (sscanf(opt5->answer, "%d", &nsteps) != 1)
	G_fatal_error(_("Invalid number of steps: %s"), opt5->answer);

    cat_ranges = flag3->answer;

    if (cat_ranges && nsteps != 255)
	G_warning(_("When -C flag is set, the nsteps argument is ignored"));

    nodata = flag1->answer;

    if (Rast_read_colors(map_name, "", &pcolors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), map_name);

    if (Rast_read_cats(map_name, "", &cats) == -1)
	G_fatal_error(_("Category file for <%s> not available"), map_name);

    if (Rast_read_range(map_name, "", &range) == -1)
	G_fatal_error(_("Range information for <%s> not available"),
		      map_name);

    /* get the distribution statistics */

    get_stats(map_name, &dist_stats);

    /* set up the graphics driver and initialize its color-table */

    D_open_driver();
    
    D_setup_unity(0);			/* 0 = don't clear frame */
    D_get_src(&t, &b, &l, &r);

    /* clear the frame, if requested to do so */
    if (strcmp(bg_opt->answer, "none") != 0)
	D_erase(bg_opt->answer);

    /* draw a title for */
    sprintf(title, "%s", map_name);
    text_height = (b - t) * 0.05;
    text_width = (r - l) * 0.05 * 0.50;
    D_text_size(text_width, text_height);
    D_get_text_box(title, &tt, &tb, &tl, &tr);
    D_pos_abs(l + (r - l) / 2 - (tr - tl) / 2,
	      t + (b - t) * 0.07);
    D_use_color(color);
    D_text(title);

    /* plot the distributrion statistics */
    if (style == PIE)
	pie(&dist_stats, &pcolors);
    else
	bar(&dist_stats, &pcolors);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}
