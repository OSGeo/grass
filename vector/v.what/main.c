/*
 ****************************************************************************
 *
 * MODULE:       v.what
 *
 * AUTHOR(S):    Trevor Wiens - derived from d.what.vect - 15 Jan 2006
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               Multiple features by Huidae Cho <grass4u gmail.com>
 *
 * PURPOSE:      To select and report attribute information for objects at a
 *               user specified location. This replaces d.what.vect by removing
 *               the interactive component to enable its use with the new
 *               gis.m and future GUI.
 *
 * COPYRIGHT:    (C) 2006-2010, 2011, 2017 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/display.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "what.h"


int main(int argc, char **argv)
{
    struct {
	struct Flag *print, *topo, *shell, *json, *multiple;
    } flag;
    struct {
	struct Option *map, *field, *coords, *maxdist, *type;
    } opt;
    struct Cell_head window;
    struct GModule *module;

    char **vect;
    int nvects;
    struct Map_info *Map;

    char buf[2000];
    int i, level, ret, type;
    int *field;
    double xval, yval, xres, yres, maxd, x;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
    char nsres[30], ewres[30];
    char ch;
    int output;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("position"));
    module->description = _("Queries a vector map at given locations.");

    opt.map = G_define_standard_option(G_OPT_V_MAPS);

    opt.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.field->multiple = YES;

    opt.type = G_define_standard_option(G_OPT_V3_TYPE);
    opt.type->answer = "point,line,area,face";

    opt.coords = G_define_standard_option(G_OPT_M_COORDS);
    opt.coords->required = YES;
    opt.coords->label = _("Coordinates for query");
    opt.coords->description = _("'-' for standard input");

    opt.maxdist = G_define_option();
    opt.maxdist->type = TYPE_DOUBLE;
    opt.maxdist->key = "distance";
    opt.maxdist->answer = "0";
    opt.maxdist->multiple = NO;
    opt.maxdist->description = _("Query threshold distance");
    opt.maxdist->guisection = _("Threshold");

    flag.topo = G_define_flag();
    flag.topo->key = 'd';
    flag.topo->description = _("Print topological information (debugging)");
    flag.topo->guisection = _("Print");

    flag.print = G_define_flag();
    flag.print->key = 'a';
    flag.print->description = _("Print attribute information");
    flag.print->guisection = _("Print");

    flag.shell = G_define_flag();
    flag.shell->key = 'g';
    flag.shell->description = _("Print the stats in shell script style");
    flag.shell->guisection = _("Print");

    flag.json = G_define_flag();
    flag.json->key = 'j';
    flag.json->description = _("Print the stats in JSON");
    flag.json->guisection = _("Print");

    flag.multiple = G_define_flag();
    flag.multiple->key = 'm';
    flag.multiple->description = _("Print multiple features if overlapping features are found");
    flag.multiple->guisection = _("Print");

    G_option_exclusive(flag.shell, flag.json, NULL);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* initialize variables */
    vect = NULL;
    Map = NULL;
    nvects = 0;
    field = NULL;

    if (opt.map->answers && opt.map->answers[0])
	vect = opt.map->answers;
    else
	G_fatal_error(_("No input vector maps!"));

    maxd = atof(opt.maxdist->answer);
    type = Vect_option_to_types(opt.type);
    output = flag.shell->answer ? OUTPUT_SCRIPT :
	    (flag.json->answer ? OUTPUT_JSON : OUTPUT_TEXT);

    if (maxd == 0.0) {
	/* this code is a translation from d.what.vect which uses display
	 * resolution to figure out a querying distance
	 * display resolution is not available here
	 * using raster resolution instead to determine vector querying
	 * distance does not really make sense
	 * maxd = 0 can make sense */
	G_get_window(&window);
	x = window.proj;
	G_format_resolution(window.ew_res, ewres, x);
	G_format_resolution(window.ns_res, nsres, x);
	G_begin_distance_calculations();
	EW_DIST1 =
	    G_distance(window.east, window.north, window.west, window.north);
	/* EW Dist at South Edge */
	EW_DIST2 =
	    G_distance(window.east, window.south, window.west, window.south);
	/* NS Dist at East edge */
	NS_DIST1 =
	    G_distance(window.east, window.north, window.east, window.south);
	/* NS Dist at West edge */
	NS_DIST2 =
	    G_distance(window.west, window.north, window.west, window.south);
	xres = ((EW_DIST1 + EW_DIST2) / 2) / window.cols;
	yres = ((NS_DIST1 + NS_DIST2) / 2) / window.rows;
	if (xres > yres)
	    maxd = xres;
	else
	    maxd = yres;
    }

    /* Look at maps given on command line */
    if (vect) {
	for (i = 0; vect[i]; i++)
	    ;
	nvects = i;

	for (i = 0; opt.field->answers[i]; i++)
	    ;

	if (nvects != i)
	    G_fatal_error(_("Number of given vector maps (%d) differs from number of layers (%d)"),
			  nvects, i);

	Map = (struct Map_info *) G_malloc(nvects * sizeof(struct Map_info));
	field = (int *) G_malloc(nvects * sizeof(int));

	for (i = 0; i < nvects; i++) {
	    level = Vect_open_old2(&Map[i], vect[i], "", opt.field->answers[i]);
	    if (level < 2)
		G_fatal_error(_("You must build topology on vector map <%s>"),
			      vect[i]);
	    field[i] = Vect_get_field_number(&Map[i], opt.field->answers[i]);
	}
    }

    if (strcmp(opt.coords->answer, "-") == 0) {
	/* read them from stdin */
	setvbuf(stdin, NULL, _IOLBF, 0);
	setvbuf(stdout, NULL, _IOLBF, 0);
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
	    ret = sscanf(buf, "%lf%c%lf", &xval, &ch, &yval);
	    if (ret == 3 && (ch == ',' || ch == ' ' || ch == '\t')) {
		what(Map, nvects, vect, xval, yval, maxd, type,
		     flag.topo->answer, flag.print->answer, output,
		     flag.multiple->answer, field);
	    }
	    else {
		G_warning(_("Unknown input format, skipping: '%s'"), buf);
		continue;
	    }
	}
    }
    else {
	/* use coords given on command line */
	for (i = 0; opt.coords->answers[i] != NULL; i += 2) {
	    xval = atof(opt.coords->answers[i]);
	    yval = atof(opt.coords->answers[i + 1]);
	    what(Map, nvects, vect, xval, yval, maxd, type, flag.topo->answer,
		 flag.print->answer, output, flag.multiple->answer, field);
	}
    }

    for (i = 0; i < nvects; i++)
	Vect_close(&Map[i]);

    exit(EXIT_SUCCESS);
}
