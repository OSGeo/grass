
/*-s.delaunay
**
** Author: James Darrell McCauley (mccauley@ecn.purdue.edu)
**         USDA Fellow
**         Department of Agricultural Engineering
**         Purdue University
**         West Lafayette, Indiana 47907-1146 USA
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted. This
** software is provided "as is" without express or implied warranty.
**
** Modification History:
** 06 Feb 93 - James Darrell McCauley <mccauley@ecn.purdue.edu> pieced
**             this together from stuff he found on netlib (see the manpage).
**
** 4 2008: Benjamin Ducke - 3D support + better memory management
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "sw_defs.h"
#include "defs.h"

int main(int argc, char **argv)
{
    char *mapset;
    struct Flag *reg_flag, *line_flag;
    struct Option *in_opt, *out_opt;
    struct GModule *module;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int nareas, area;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector");
    module->description = _("Creates a Delaunay triangulation from an input "
			    "vector map containing points or centroids.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    reg_flag = G_define_flag();
    reg_flag->key = 'r';
    reg_flag->description = _("Use only points in current region");

    line_flag = G_define_flag();
    line_flag->key = 'l';
    line_flag->description =
	_("Output triangulation as a graph (lines), not areas");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (line_flag->answer)
	Type = GV_LINE;
    else
	Type = GV_BOUNDARY;

    All = reg_flag->answer ? 0 : 1;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open files */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL) {
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);
    }

    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, mapset);

    /* check if we have a 3D input points map */
    mode3d = 0;
    if (Vect_is_3d(&In)) {
	mode3d = 1;
    }


    if (mode3d) {
	if (0 > Vect_open_new(&Out, out_opt->answer, 1)) {
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  out_opt->answer);
	}
    }
    else {
	if (0 > Vect_open_new(&Out, out_opt->answer, 0)) {
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  out_opt->answer);
	}

    }

    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Vect_build_partial(&Out, GV_BUILD_BASE, NULL);

    /* initialize working region */
    G_get_window(&Window);
    G_percent(0, 100, 1);
    Vect_region_box(&Window, &Box);

    freeinit(&sfl, sizeof *sites);

    readsites();

    siteidx = 0;
    geominit();

    triangulate = 1;
    plot = 0;
    debug = 0;
    voronoi(triangulate, nextone);

    Vect_close(&In);

    Vect_build_partial(&Out, GV_BUILD_ATTACH_ISLES, NULL);

    nareas = Vect_get_num_areas(&Out);
    G_debug(3, "nareas = %d", nareas);
    for (area = 1; area <= nareas; area++) {
	double x, y, z, angle, slope;
	int ret;

	G_percent(area, nareas, 2);
	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	ret = Vect_get_point_in_area(&Out, area, &x, &y);
	if (ret < 0) {
	    G_warning(_("Cannot calculate area centroid"));
	    continue;
	}

	ret = Vect_tin_get_z(&Out, x, y, &z, &angle, &slope);
	G_debug(3, "area centroid z: %f", z);
	if (ret < 0) {
	    G_warning(_("Cannot calculate area centroid z coordinate"));
	    continue;
	}

	Vect_append_point(Points, x, y, z);
	Vect_cat_set(Cats, 1, area);

	Vect_write_line(&Out, GV_CENTROID, Points, Cats);
    }

    Vect_build_partial(&Out, GV_BUILD_NONE, NULL);
    Vect_build(&Out, stderr);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
