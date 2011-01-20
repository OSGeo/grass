/*
 ****************************************************************************
 *
 * MODULE:       v.what
 *
 * AUTHOR(S):    Trevor Wiens - derived from d.what.vect - 15 Jan 2006
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      To select and report attribute information for objects at a
 *               user specified location. This replaces d.what.vect by removing
 *               the interactive component to enable its use with the new 
 *               gis.m and future GUI.
 *
 * COPYRIGHT:    (C) 2006-2010 by the GRASS Development Team
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

char **vect;
int nvects;
struct Map_info *Map;

int main(int argc, char **argv)
{
    struct Flag *printattributes, *topo_flag, *shell_flag;
    struct Option *map_opt, *field_opt, *coords_opt, *maxdistance;
    struct Cell_head window;
    struct GModule *module;
    char buf[2000];
    int i, level, ret;
    int *field;
    double xval, yval, xres, yres, maxd, x;
    double EW_DIST1, EW_DIST2, NS_DIST1, NS_DIST2;
    char nsres[30], ewres[30];
    char ch;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("querying"));
    module->description = _("Queries a vector map at given locations.");

    map_opt = G_define_standard_option(G_OPT_V_MAPS);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    
    coords_opt = G_define_option();
    coords_opt->key = "east_north";
    coords_opt->type = TYPE_DOUBLE;
    coords_opt->key_desc = "east,north";
    coords_opt->required = NO;
    coords_opt->multiple = YES;
    coords_opt->label = _("Coordinates for query");
    coords_opt->description = _("If not given read from standard input");

    maxdistance = G_define_option();
    maxdistance->type = TYPE_DOUBLE;
    maxdistance->key = "distance";
    maxdistance->answer = "0";
    maxdistance->multiple = NO;
    maxdistance->description = _("Query threshold distance");

    topo_flag = G_define_flag();
    topo_flag->key = 'd';
    topo_flag->description = _("Print topological information (debugging)");
    topo_flag->guisection = _("Print");

    printattributes = G_define_flag();
    printattributes->key = 'a';
    printattributes->description = _("Print attribute information");
    printattributes->guisection = _("Print");

    shell_flag = G_define_flag();
    shell_flag->key = 'g';
    shell_flag->description = _("Print the stats in shell script style");
    shell_flag->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (map_opt->answers && map_opt->answers[0])
	vect = map_opt->answers;

    maxd = atof(maxdistance->answer);

    /*  
     *  fprintf(stdout, maxdistance->answer);
     *  fprintf(stdout, "Maxd is %f", maxd);
     *  fprintf(stdout, xcoord->answer);
     *  fprintf(stdout, "xval is %f", xval);
     *  fprintf(stdout, ycoord->answer);
     *  fprintf(stdout, "yval is %f", yval);
     */

    if (maxd == 0.0) {
	G_get_window(&window);
	x = window.proj;
	G_format_resolution(window.ew_res, ewres, x);
	G_format_resolution(window.ns_res, nsres, x);
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

	for (i = 0; field_opt->answers[i]; i++)
	    ;
	
	if (nvects != i)
	    G_fatal_error(_("Number of given vector maps (%d) differs from number of layers (%d)"),
			  nvects, i);
	
	Map = (struct Map_info *) G_malloc(nvects * sizeof(struct Map_info));
	field = (int *) G_malloc(nvects * sizeof(int));
	
	for (i = 0; i < nvects; i++) {
	    level = Vect_open_old2(&Map[i], vect[i], "", field_opt->answers[i]);
	    if (level < 2)
		G_fatal_error(_("You must build topology on vector map <%s>"),
			      vect[i]);
	    field[i] = Vect_get_field_number(&Map[i], field_opt->answers[i]);
	}
    }

    if (!coords_opt->answer) {
	/* read them from stdin */
	setvbuf(stdin, NULL, _IOLBF, 0);
	setvbuf(stdout, NULL, _IOLBF, 0);
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
	    ret = sscanf(buf, "%lf%c%lf", &xval, &ch, &yval);
	    if (ret == 3 && (ch == ',' || ch == ' ' || ch == '\t')) {
		what(xval, yval, maxd, topo_flag->answer,
		     printattributes->answer, shell_flag->answer, field);
	    }
	    else {
		G_warning(_("Unknown input format, skipping: '%s'"), buf);
		continue;
	    }
	}
    }
    else {
	/* use coords given on command line */
	for (i = 0; coords_opt->answers[i] != NULL; i += 2) {
	    xval = atof(coords_opt->answers[i]);
	    yval = atof(coords_opt->answers[i + 1]);
	    what(xval, yval, maxd, topo_flag->answer,
		 printattributes->answer, shell_flag->answer, field);
	}
    }

    for (i = 0; i < nvects; i++)
	Vect_close(&Map[i]);

    exit(EXIT_SUCCESS);
}
