/***************************************************************
 *
 * MODULE:       v.delaunay
 *
 * AUTHOR(S):    Martin Pavlovsky (Paul Kelly mentor)
 *
 * PURPOSE:      creates a Delaunay triangulation vector map
 *
 * COPYRIGHT:    (C) 2008 by the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "data_types.h"
#include "memory.h"
#include "geometry.h"
#include "edge.h"
#include "in_out.h"

int compare(const struct vertex **p1, const struct vertex **p2);

int main(int argc, char *argv[])
{

    /* GRASS related variables */
    const char *mapset;
    struct Map_info map_in, map_out;
    struct Cell_head Window;
    BOUND_BOX Box;
    struct GModule *module;
    struct Flag *reg_flag, *line_flag;
    struct Option *in_opt, *out_opt;

    struct line_pnts *Points;
    struct line_cats *Cats;
    int nareas, area;

    int Type;
    int complete_map;
    int mode3d;

    /* ---------------------- */

    unsigned int i;
    unsigned int n;
    struct edge *l_cw, *r_ccw;
    struct vertex **sites_sorted;

    /* GRASS related manipulations */
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

    complete_map = reg_flag->answer ? 0 : 1;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open files */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(2);
    Vect_open_old(&map_in, in_opt->answer, mapset);

    /* check if we have a 3D input points map */
    mode3d = Vect_is_3d(&map_in);

    if (mode3d) {
	if (0 > Vect_open_new(&map_out, out_opt->answer, 1))
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  out_opt->answer);
    }
    else if (0 > Vect_open_new(&map_out, out_opt->answer, 0))
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);

    Vect_hist_copy(&map_in, &map_out);
    Vect_hist_command(&map_out);

    /* initialize working region */
    G_get_window(&Window);
    G_percent(0, 100, 1);
    Vect_region_box(&Window, &Box);

    n = read_sites(mode3d, complete_map, map_in, Box);

    /* Sort. */
    sites_sorted =
	(struct vertex **)G_malloc((unsigned)n * sizeof(struct vertex *));
    if (sites_sorted == MY_NULL)
	G_fatal_error(_("Not enough memory."));
    for (i = 0; i < n; i++)
	sites_sorted[i] = sites + i;
    qsort(sites_sorted, n, sizeof(struct vertex *), (void *)compare);

    remove_duplicates(sites_sorted, &n);

    /* Triangulate. */
    divide(sites_sorted, 0, n - 1, &l_cw, &r_ccw);

    output_edges(sites_sorted, n, mode3d, Type, map_out);

    free((char *)sites_sorted);
    free_memory();

    Vect_close(&map_in);

    if (Type == GV_BOUNDARY) {
	Vect_build_partial(&map_out, GV_BUILD_AREAS);
	nareas = Vect_get_num_areas(&map_out);
	G_debug(3, "nareas = %d", nareas);
	/*  Assign centroid to each area */
	for (area = 1; area <= nareas; area++) {
	    double x, y, z, angle, slope;
	    int ret;

	    G_percent(area, nareas, 2);
	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);
	    ret = Vect_get_point_in_area(&map_out, area, &x, &y);
	    if (ret < 0) {
		G_warning(_("Unable to calculate area centroid"));
		continue;
	    }
	    ret = Vect_tin_get_z(&map_out, x, y, &z, &angle, &slope);
	    G_debug(3, "area centroid z: %f", z);
	    if (ret < 0) {
		G_warning(_("Unable to calculate area centroid z coordinate"));
		continue;
	    }
	    Vect_append_point(Points, x, y, z);
	    Vect_cat_set(Cats, 1, area);
	    Vect_write_line(&map_out, GV_CENTROID, Points, Cats);
	}
    }
    Vect_build(&map_out);
    Vect_close(&map_out);

    exit(EXIT_SUCCESS);
}

/* compare first according to x-coordinate, if equal then y-coordinate */
int compare(const struct vertex **p1, const struct vertex **p2)
{
    if ((*p1)->x == (*p2)->x)
	return ((*p1)->y < (*p2)->y);
    return ((*p1)->x < (*p2)->x);
}
