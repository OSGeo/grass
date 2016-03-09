
/****************************************************************
 * 
 * MODULE:       v.net.alloc
 *
 * AUTHOR(S):    Radim Blazek
 *               Stepan Turek <stepan.turek seznam.cz> (turns support)
 *               Markus Metz (costs from/to centers)
 *
 * PURPOSE:      Allocate subnets for nearest centers
 *               
 * COPYRIGHT:    (C) 2001, 2016 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "alloc.h"


int main(int argc, char **argv)
{
    int i, ret, line, center1, center2;
    int nlines, nnodes, type, ltype, afield, nfield, geo, cat, tfield,
	tucfield;
    int node1, node2;
    double e1cost, e2cost, n1cost, n2cost, s1cost, s2cost, l, l1, l2;
    struct Option *map, *output, *method_opt;
    struct Option *afield_opt, *nfield_opt, *afcol, *abcol, *ncol, *type_opt,
	*term_opt, *tfield_opt, *tucfield_opt;
    struct Flag *geo_f, *turntable_f;
    struct GModule *module;
    struct Map_info Map, Out;
    struct cat_list *catlist;
    CENTER *Centers = NULL;
    int acenters = 0, ncenters = 0;
    NODE *Nodes;
    struct line_cats *Cats;
    struct line_pnts *Points, *SPoints;
    int graph_version;
    int from_centers;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("cost allocation"));
    module->label =
	_("Allocates subnets for nearest centers (direction from center).");
    module->description =
	_("center node must be opened (costs >= 0). "
	  "Costs of center node are used in calculation");


    map = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->options = "from,to";
    method_opt->answer = "from";
    method_opt->description = _("Use costs from centers or costs to centers");
    method_opt->guisection = _("Cost");

    term_opt = G_define_standard_option(G_OPT_V_CATS);
    term_opt->key = "center_cats";
    term_opt->required = YES;
    term_opt->description =
	_("Categories of centers (points on nodes) to which net "
	  "will be allocated, "
	  "layer for this categories is given by nlayer option");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "arc_layer";
    afield_opt->answer = "1";
    afield_opt->required = YES;
    afield_opt->label = _("Arc layer");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->key = "arc_type";
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";
    type_opt->required = YES;
    type_opt->label = _("Arc type");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "node_layer";
    nfield_opt->answer = "2";
    nfield_opt->required = YES;
    nfield_opt->label = _("Node layer");

    afcol = G_define_option();
    afcol->key = "arc_column";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description = _("Arc forward/both direction(s) cost column (number)");
    afcol->guisection = _("Cost");

    abcol = G_define_option();
    abcol->key = "arc_backward_column";
    abcol->type = TYPE_STRING;
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column (number)");
    abcol->guisection = _("Cost");

    ncol = G_define_option();
    ncol->key = "node_column";
    ncol->type = TYPE_STRING;
    ncol->required = NO;
    ncol->description = _("Node cost column (number)");
    ncol->guisection = _("Cost");

    turntable_f = G_define_flag();
    turntable_f->key = 't';
    turntable_f->description = _("Use turntable");
    turntable_f->guisection = _("Turntable");

    tfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    tfield_opt->key = "turn_layer";
    tfield_opt->answer = "3";
    tfield_opt->label = _("Layer with turntable");
    tfield_opt->description =
	_("Relevant only with -t flag");
    tfield_opt->guisection = _("Turntable");

    tucfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    tucfield_opt->key = "turn_cat_layer";
    tucfield_opt->answer = "4";
    tucfield_opt->label = _("Layer with unique categories used in turntable");
    tucfield_opt->description =
	_("Relevant only with -t flag");
    tucfield_opt->guisection = _("Turntable");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Vect_check_input_output_name(map->answer, output->answer, G_FATAL_EXIT);

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();
    SPoints = Vect_new_line_struct();

    type = Vect_option_to_types(type_opt);

    catlist = Vect_new_cat_list();
    Vect_str_to_cat_list(term_opt->answer, catlist);

    if (geo_f->answer)
	geo = 1;
    else
	geo = 0;

    Vect_set_open_level(2);
    if (Vect_open_old(&Map, map->answer, "") < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), map->answer);

    afield = Vect_get_field_number(&Map, afield_opt->answer);
    nfield = Vect_get_field_number(&Map, nfield_opt->answer);
    tfield = Vect_get_field_number(&Map, tfield_opt->answer);
    tucfield = Vect_get_field_number(&Map, tucfield_opt->answer);

    /* Build graph */
    graph_version = 1;
    from_centers = 1;
    if (method_opt->answer[0] == 't' && !turntable_f->answer) {
	graph_version = 2;
	from_centers = 0;
    }
    if (turntable_f->answer)
	Vect_net_ttb_build_graph(&Map, type, afield, nfield, tfield, tucfield,
				 afcol->answer, abcol->answer, ncol->answer,
				 geo, 0);
    else
	Vect_net_build_graph(&Map, type, afield, nfield, afcol->answer,
			     abcol->answer, ncol->answer, geo, graph_version);

    nnodes = Vect_get_num_nodes(&Map);
    nlines = Vect_get_num_lines(&Map);

    /* Create list of centers based on list of categories */
    for (i = 1; i <= nlines; i++) {
	int node;

	ltype = Vect_get_line_type(&Map, i);
	if (!(ltype & GV_POINT))
	    continue;

	Vect_read_line(&Map, Points, Cats, i);
	node =
	    Vect_find_node(&Map, Points->x[0], Points->y[0], Points->z[0], 0,
			   0);
	if (!node) {
	    G_warning(_("Point is not connected to the network"));
	    continue;
	}
	if (!(Vect_cat_get(Cats, nfield, &cat)))
	    continue;
	if (Vect_cat_in_cat_list(cat, catlist)) {
	    Vect_net_get_node_cost(&Map, node, &n1cost);
	    if (n1cost == -1) {	/* closed */
		G_warning("Centre at closed node (costs = -1) ignored");
	    }
	    else {
		if (acenters == ncenters) {
		    acenters += 1;
		    Centers =
			(CENTER *) G_realloc(Centers,
					     acenters * sizeof(CENTER));
		}
		Centers[ncenters].cat = cat;
		Centers[ncenters].node = node;
		G_debug(2, "centre = %d node = %d cat = %d", ncenters,
			node, cat);
		ncenters++;
	    }
	}
    }

    G_message(_("Number of centers: [%d] (nlayer: [%d])"), ncenters, nfield);

    if (ncenters == 0)
	G_warning(_("Not enough centers for selected nlayer. "
		    "Nothing will be allocated."));

    /* alloc and reset space for all lines */
    if (turntable_f->answer) {
	/* if turntable is used we are looking for lines as destinations, not the intersections (nodes) */
	Nodes = (NODE *) G_calloc((nlines * 2 + 2), sizeof(NODE));
    }
    else {
	Nodes = (NODE *) G_calloc((nnodes + 1), sizeof(NODE));
    }

    /* Fill Nodes by nearest center and costs from that center */

    if (turntable_f->answer) {
	G_message(_("Calculating costs from centers ..."));
	alloc_from_centers_loop_tt(&Map, Nodes, Centers, ncenters,
				   tucfield);
    }
    else {
	if (from_centers) {
	    G_message(_("Calculating costs from centers ..."));
	    alloc_from_centers(Vect_net_get_graph(&Map), Nodes, Centers, ncenters);
	}
	else {
	    G_message(_("Calculating costs to centers ..."));
	    alloc_to_centers(Vect_net_get_graph(&Map), Nodes, Centers, ncenters);
	}
    }

    /* Write arcs to new map */
    if (Vect_open_new(&Out, output->answer, Vect_is_3d(&Map)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), output->answer);

    Vect_hist_command(&Out);

    nlines = Vect_get_num_lines(&Map);
    for (line = 1; line <= nlines; line++) {
	ltype = Vect_read_line(&Map, Points, NULL, line);
	if (!(ltype & type)) {
	    continue;
	}

	if (turntable_f->answer) {
	    center1 = Nodes[line * 2].center;
	    center2 = Nodes[line * 2 + 1].center;
	    s1cost = Nodes[line * 2].cost;
	    s2cost = Nodes[line * 2 + 1].cost;
	    n1cost = n2cost = 0;
	}
	else {
	    Vect_get_line_nodes(&Map, line, &node1, &node2);
	    center1 = Nodes[node1].center;
	    center2 = Nodes[node2].center;
	    s1cost = Nodes[node1].cost;
	    s2cost = Nodes[node2].cost;

	    Vect_net_get_node_cost(&Map, node1, &n1cost);
	    Vect_net_get_node_cost(&Map, node2, &n2cost);
	}

	if (from_centers) {
	    Vect_net_get_line_cost(&Map, line, GV_FORWARD, &e1cost);
	    Vect_net_get_line_cost(&Map, line, GV_BACKWARD, &e2cost);
	}
	else {
	    /* from node to center */
	    Vect_net_get_line_cost(&Map, line, GV_FORWARD, &e2cost);
	    Vect_net_get_line_cost(&Map, line, GV_BACKWARD, &e1cost);
	}

	G_debug(3, "Line %d:", line);
	G_debug(3, "Arc centers: %d %d (nodes: %d %d)", center1, center2,
		node1, node2);

	G_debug(3, "  s1cost = %f n1cost = %f e1cost = %f", s1cost, n1cost,
		e1cost);
	G_debug(3, "  s2cost = %f n2cost = %f e2cost = %f", s2cost, n2cost,
		e2cost);

	Vect_reset_cats(Cats);

	/* First check if arc is reachable from at least one side */
	if ((center1 != -1 && n1cost != -1 && e1cost != -1) ||
	    (center2 != -1 && n2cost != -1 && e2cost != -1)) {
	    /* Line is reachable at least from one side */
	    G_debug(3, "  -> arc is reachable");

	    if (center1 == center2) {	/* both nodes in one area -> whole arc in one area */
		if (center1 != -1)
		    cat = Centers[center1].cat;	/* line reachable */
		else
		    cat = Centers[center2].cat;
		Vect_cat_set(Cats, 1, cat);
		Vect_write_line(&Out, ltype, Points, Cats);
	    }
	    else {		/* each node in different area */
		/* Check if line is reachable from center */
		if (center1 == -1 || n1cost == -1 || e1cost == -1) {	/* closed from first node */
		    G_debug(3,
			    "    -> arc is not reachable from 1. node -> alloc to 2. node");
		    cat = Centers[center2].cat;
		    Vect_cat_set(Cats, 1, cat);
		    Vect_write_line(&Out, ltype, Points, Cats);
		    continue;
		}
		else if (center2 == -1 || n2cost == -1 || e2cost == -1) {	/* closed from second node */
		    G_debug(3,
			    "    -> arc is not reachable from 2. node -> alloc to 1. node");
		    cat = Centers[center1].cat;
		    Vect_cat_set(Cats, 1, cat);
		    Vect_write_line(&Out, ltype, Points, Cats);
		    continue;
		}
		/* Now we know that arc is reachable from both sides */

		/* Add costs of node to starting costs */
		s1cost += n1cost;
		s2cost += n2cost;

		/* Check if s1cost + e1cost <= s2cost or s2cost + e2cost <= s1cost !
		 * Note this check also possibility of (e1cost + e2cost) = 0 */
		if (s1cost + e1cost <= s2cost) {	/* whole arc reachable from node1 */
		    cat = Centers[center1].cat;
		    Vect_cat_set(Cats, 1, cat);
		    Vect_write_line(&Out, ltype, Points, Cats);
		}
		else if (s2cost + e2cost <= s1cost) {	/* whole arc reachable from node2 */
		    cat = Centers[center2].cat;
		    Vect_cat_set(Cats, 1, cat);
		    Vect_write_line(&Out, ltype, Points, Cats);
		}
		else {		/* split */
		    /* Calculate relative costs - we expect that costs along the line do not change */
		    l = Vect_line_length(Points);
		    e1cost /= l;
		    e2cost /= l;

		    G_debug(3, "  -> s1cost = %f e1cost = %f", s1cost,
			    e1cost);
		    G_debug(3, "  -> s2cost = %f e2cost = %f", s2cost,
			    e2cost);

		    /* Costs from both centers to the splitting point must be equal:
		     * s1cost + l1 * e1cost = s2cost + l2 * e2cost */
		    l1 = (l * e2cost - s1cost + s2cost) / (e1cost + e2cost);
		    l2 = l - l1;
		    G_debug(3, "l = %f l1 = %f l2 = %f", l, l1, l2);

		    /* First segment */
		    ret = Vect_line_segment(Points, 0, l1, SPoints);
		    if (ret == 0) {
			G_warning(_
				  ("Cannot get line segment, segment out of line"));
		    }
		    else {
			cat = Centers[center1].cat;
			Vect_cat_set(Cats, 1, cat);
			Vect_write_line(&Out, ltype, SPoints, Cats);
		    }

		    /* Second segment */
		    ret = Vect_line_segment(Points, l1, l, SPoints);
		    if (ret == 0) {
			G_warning(_
				  ("Cannot get line segment, segment out of line"));
		    }
		    else {
			Vect_reset_cats(Cats);
			cat = Centers[center2].cat;
			Vect_cat_set(Cats, 1, cat);
			Vect_write_line(&Out, ltype, SPoints, Cats);
		    }
		}
	    }
	}
	else {
	    /* arc is not reachable */
	    G_debug(3, "  -> arc is not reachable");
	    Vect_write_line(&Out, ltype, Points, Cats);
	}
    }

    Vect_build(&Out);

    /* Free, ... */
    G_free(Nodes);
    G_free(Centers);
    Vect_close(&Map);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
