
/****************************************************************
 *
 * MODULE:     v.net.spanningtree
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Computes spanning tree in the network
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/neta.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *field_opt, *accol;
    struct Flag *geo_f;
    int with_z;
    int layer, mask_type;
    dglGraph_s *graph;
    int i, edges, geo;
    struct ilist *tree_list;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("spanning tree"));
    module->description = _("Computes minimum spanning tree for the network.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    accol = G_define_standard_option(G_OPT_DB_COLUMN);
    accol->key = "accol";
    accol->required = NO;
    accol->description = _("Name of Arc cost column");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    /* TODO: make an option for this */
    mask_type = GV_LINE | GV_BOUNDARY;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(map_in->answer, map_out->answer,
				 GV_FATAL_EXIT);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, map_in->answer, ""))
	G_fatal_error(_("Unable to open vector map <%s>"),
		      map_in->answer);

    with_z = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, map_out->answer, with_z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), map_out->answer);
    }

    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_warning(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

    /* parse filter option and select appropriate lines */
    layer = atoi(field_opt->answer);

    Vect_net_build_graph(&In, mask_type, layer, 0,
			 accol->answer, NULL, NULL, geo, 0);
    graph = &(In.graph);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    tree_list = Vect_new_list();
    edges = NetA_spanning_tree(graph, tree_list);
    G_debug(3, "Edges: %d", edges);
    for (i = 0; i < edges; i++) {
	int type = Vect_read_line(&In, Points, Cats, abs(tree_list->value[i]));
	Vect_write_line(&Out, type, Points, Cats);
    }
    Vect_destroy_list(tree_list);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
