
/****************************************************************
 *
 * MODULE:     v.net.bridge
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Computes bridges and articulation points
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
    struct Option *afield_opt, *nfield_opt, *abcol, *afcol, *ncol,
                  *method_opt;
    char *desc;
    int with_z;
    int afield, nfield, mask_type;
    dglGraph_s *graph;
    int i, bridges, articulations;
    struct ilist *bridge_list, *articulation_list;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("articulation points"));
    module->description =
	_("Computes bridges and articulation points in the network.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");
    afield_opt->guisection = _("Cost");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");
    nfield_opt->guisection = _("Cost");

    afcol = G_define_standard_option(G_OPT_DB_COLUMN);
    afcol->key = "afcolumn";
    afcol->required = NO;
    afcol->description =
	_("Arc forward/both direction(s) cost column (number)");
    afcol->guisection = _("Cost");

    abcol = G_define_standard_option(G_OPT_DB_COLUMN);
    abcol->key = "abcolumn";
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column (number)");
    abcol->guisection = _("Cost");

    ncol = G_define_option();
    ncol->key = "ncolumn";
    ncol->type = TYPE_STRING;
    ncol->required = NO;
    ncol->description = _("Node cost column (number)");
    ncol->guisection = _("Cost");

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->multiple = NO;
    method_opt->options = "bridge,articulation";
    desc = NULL;
    G_asprintf(&desc,
	       "bridge;%s;articulation;%s",
	       _("Finds bridges"),
	       _("Finds articulation points"));
    method_opt->descriptions = desc;
    method_opt->description = _("Feature type");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    /* TODO: make an option for this */
    mask_type = GV_LINE | GV_BOUNDARY;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(map_in->answer, map_out->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, map_in->answer, ""))
	G_fatal_error(_("Unable to open vector map <%s>"), map_in->answer);

    with_z = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, map_out->answer, with_z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), map_out->answer);
    }


    /* parse filter option and select appropriate lines */
    afield = Vect_get_field_number(&In, afield_opt->answer);
    nfield = Vect_get_field_number(&In, nfield_opt->answer);

    if (0 != Vect_net_build_graph(&In, mask_type, afield, nfield, afcol->answer,
                                  abcol->answer, ncol->answer, 0, 0))
        G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(&In));
    
    graph = Vect_net_get_graph(&In);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    if (method_opt->answer[0] == 'b') {
	bridge_list = Vect_new_list();
	bridges = NetA_compute_bridges(graph, bridge_list);

	G_debug(3, "Bridges: %d", bridges);

	for (i = 0; i < bridges; i++) {
	    int type =
		Vect_read_line(&In, Points, Cats, abs(bridge_list->value[i]));
	    Vect_write_line(&Out, type, Points, Cats);
	}
	Vect_destroy_list(bridge_list);
    }
    else {
	articulation_list = Vect_new_list();
	articulations = NetA_articulation_points(graph, articulation_list);
	G_debug(3, "Articulation points: %d", articulations);

	for (i = 0; i < articulations; i++) {
	    double x, y, z;

	    Vect_get_node_coor(&In, articulation_list->value[i], &x, &y, &z);
	    Vect_reset_line(Points);
	    Vect_append_point(Points, x, y, z);
	    Vect_write_line(&Out, GV_POINT, Points, Cats);
	}

	Vect_destroy_list(articulation_list);
    }

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
