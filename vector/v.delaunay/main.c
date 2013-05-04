/***************************************************************
 *
 * MODULE:       v.delaunay
 *
 * AUTHOR(S):    Martin Pavlovsky (Google SoC 2008, Paul Kelly mentor)
 *               Based on "dct" by Geoff Leach, Department of Computer 
 *               Science, RMIT.
 *
 * PURPOSE:      Creates a Delaunay triangulation vector map
 *
 * COPYRIGHT:    (C) RMIT 1993
 *               (C) 2008-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 * 
 * The following notices apply to portions of the code originally
 * derived from work by Geoff Leach of RMIT:
 *
 *   Author: Geoff Leach, Department of Computer Science, RMIT.
 *   email: gl@cs.rmit.edu.au
 *
 *   Date: 6/10/93
 *
 *   Version 1.0
 *   
 *   Copyright (c) RMIT 1993. All rights reserved.
 *
 *   License to copy and use this software purposes is granted provided 
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   License is also granted to make and use derivative works provided
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   RMIT makes no representations concerning either the merchantability 
 *   of this software or the suitability of this software for any particular 
 *   purpose.  It is provided "as is" without express or implied warranty 
 *   of any kind.
 *
 *   These notices must be retained in any copies of any part of this software.
 * 
 **************************************************************/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "defs.h"
#include "data_types.h"
#include "memory.h"
#include "geometry.h"
#include "edge.h"
#include "in_out.h"

int main(int argc, char *argv[])
{
    /* GRASS related variables */
    struct Map_info In, Out;
    struct Cell_head Window;
    struct bound_box Box;
    struct GModule *module;
    struct Flag *reg_flag, *line_flag;
    struct Option *in_opt, *out_opt, *field_opt;

    struct line_pnts *Points;
    struct line_cats *Cats;
    int nareas, area;

    int Type;
    int complete_map;
    int mode3d;
    
    unsigned int n;
    struct edge *l_cw, *r_ccw;

    /* GRASS related manipulations */
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("triangulation"));
    module->description = _("Creates a Delaunay triangulation from an input "
			    "vector map containing points or centroids.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    field_opt->answer = "-1";
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

    complete_map = reg_flag->answer ? 0 : 1;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", field_opt->answer);

    /* check if we have a 3D input points map */
    mode3d = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, out_opt->answer, mode3d))
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);

    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* initialize working region */
    G_get_window(&Window);
    Vect_region_box(&Window, &Box);

    n = read_sites(mode3d, complete_map, &In, Box,
		   Vect_get_field_number(&In, field_opt->answer));

    Vect_set_release_support(&In);
    Vect_close(&In);

    /* sort sites */
    qsort(sites, n, sizeof(struct vertex), cmp);

    G_verbose_message(_("Removing duplicates..."));
    remove_duplicates(&n);
    if (n < 3)
	G_fatal_error(_("no points to triangulate"));
	
    /* triangulate */
    G_verbose_message(_("Delaunay triangulation..."));
    divide(0, n - 1, &l_cw, &r_ccw);

    output_edges(n, mode3d, Type, &Out);

    free_memory();

    if (Type == GV_BOUNDARY) {
	Vect_build_partial(&Out, GV_BUILD_AREAS);
	nareas = Vect_get_num_areas(&Out);
	G_debug(3, "nareas = %d", nareas);
	/*  Assign centroid to each area */
	G_message(_("Calculating area centroids..."));
	for (area = 1; area <= nareas; area++) {
	    double x, y, z, angle, slope;
	    int ret;

	    G_percent(area, nareas, 2);
	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);
	    ret = Vect_get_point_in_area(&Out, area, &x, &y);
	    if (ret < 0) {
		G_warning(_("Unable to calculate area centroid"));
		continue;
	    }
	    ret = Vect_tin_get_z(&Out, x, y, &z, &angle, &slope);
	    G_debug(3, "area centroid z: %f", z);
	    if (ret < 0) {
		G_warning(_("Unable to calculate area centroid z coordinate"));
		continue;
	    }
	    Vect_append_point(Points, x, y, z);
	    Vect_cat_set(Cats, 1, area);
	    Vect_write_line(&Out, GV_CENTROID, Points, Cats);
	}
    }
    Vect_build_partial(&Out, GV_BUILD_NONE); /* build topo from scratch */
    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
