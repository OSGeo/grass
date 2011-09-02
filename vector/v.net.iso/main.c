
/****************************************************************
 * 
 *  MODULE:       v.net.iso
 *  
 *  AUTHOR(S):    Radim Blazek
 *                
 *  PURPOSE:      Split net to bands between isolines.
 *                
 *  COPYRIGHT:    (C) 2001-2008 by the GRASS Development Team
 * 
 *                This program is free software under the 
 *                GNU General Public License (>=v2). 
 *                Read the file COPYING that comes with GRASS
 *                for details.
 * 
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

typedef struct
{
    int cat;			/* category number */
    int node;			/* node number */
} CENTER;

typedef struct
{
    int centre;			/* neares centre, initially -1 *//* currently not used */
    double cost;		/* costs from this centre, initially not undefined */
} NODE;

typedef struct
{				/* iso point along the line */
    int iso;			/* index of iso line in iso array of costs */
    double distance;		/* distance along the line from the beginning for both directions */
} ISOPOINT;

int main(int argc, char **argv)
{
    int i, ret, centre, line, centre1, centre2;
    int nlines, nnodes, type, ltype, afield, nfield, geo, cat;
    int node, node1, node2;
    double cost, e1cost, e2cost, n1cost, n2cost, s1cost, s2cost, l, l1;
    struct Option *map, *output;
    struct Option *afield_opt, *nfield_opt, *afcol, *abcol, *ncol, *type_opt,
	*term_opt, *cost_opt;
    struct Flag *geo_f;
    struct GModule *module;
    struct Map_info Map, Out;
    struct cat_list *catlist;
    CENTER *Centers = NULL;
    int acentres = 0, ncentres = 0;
    NODE *Nodes;
    struct line_cats *Cats;
    struct line_pnts *Points, *SPoints;
    int niso, aiso;
    double *iso;
    int npnts1, apnts1 = 0, npnts2, apnts2 = 0;
    ISOPOINT *pnts1 = NULL, *pnts2 = NULL;
    int next_iso;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->label = _("Splits net by cost isolines.");
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("isolines"));
    module->description =
	_("Splits net to bands between cost isolines (direction from centre). "
	 "Centre node must be opened (costs >= 0). "
	 "Costs of centre node are used in calculation.");

    map = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";
    type_opt->description = _("Arc type");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->label = _("Arc layer");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");

    afcol = G_define_standard_option(G_OPT_DB_COLUMN);
    afcol->key = "afcolumn";
    afcol->description =
	_("Arc forward/both direction(s) cost column (number)");

    abcol = G_define_standard_option(G_OPT_DB_COLUMN);
    abcol->key = "abcolumn";
    abcol->description = _("Arc backward direction cost column (number)");

    ncol = G_define_standard_option(G_OPT_DB_COLUMN);
    ncol->key = "ncolumn";
    ncol->description = _("Node cost column (number)");

    term_opt = G_define_standard_option(G_OPT_V_CATS);
    term_opt->key = "ccats";
    term_opt->required = YES;
    term_opt->description =
	_("Categories of centres (points on nodes) to which net "
	  "will be allocated. "
	  "Layer for this categories is given by nlayer option.");

    cost_opt = G_define_option();
    cost_opt->key = "costs";
    cost_opt->type = TYPE_INTEGER;
    cost_opt->multiple = YES;
    cost_opt->required = YES;
    cost_opt->description = _("Costs for isolines");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();
    SPoints = Vect_new_line_struct();

    type = Vect_option_to_types(type_opt);
    afield = atoi(afield_opt->answer);
    nfield = atoi(nfield_opt->answer);

    catlist = Vect_new_cat_list();
    Vect_str_to_cat_list(term_opt->answer, catlist);

    Vect_check_input_output_name(map->answer, output->answer, GV_FATAL_EXIT);

    /* Iso costs */
    aiso = 1;
    iso = (double *)G_malloc(aiso * sizeof(double));
    /* Set first iso to 0 */
    iso[0] = 0;
    niso = 1;
    i = 0;
    while (cost_opt->answers[i]) {
	if (niso == aiso) {
	    aiso += 1;
	    iso = (double *)G_realloc(iso, aiso * sizeof(double));
	}
	iso[niso] = atof(cost_opt->answers[i]);
	if (iso[niso] <= 0)
	    G_fatal_error(_("Wrong iso cost: %f"), iso[niso]);

	if (iso[niso] <= iso[niso - 1])
	    G_fatal_error(_("Iso cost: %f less than previous"), iso[niso]);

	G_verbose_message(_("Iso cost %d: %f"), niso, iso[niso]);
	niso++;
	i++;
    }

    /* Should not happen: */
    if (niso < 2)
	G_warning(_("Not enough costs, everything reachable falls to first band"));

    if (geo_f->answer)
	geo = 1;
    else
	geo = 0;

    Vect_set_open_level(2);
    Vect_open_old(&Map, map->answer, "");

    /* Build graph */
    Vect_net_build_graph(&Map, type, afield, nfield, afcol->answer,
			 abcol->answer, ncol->answer, geo, 0);

    nnodes = Vect_get_num_nodes(&Map);
    nlines = Vect_get_num_lines(&Map);

    /* Create list of centres based on list of categories */
    for (i = 1; i <= nlines; i++) {
	ltype = Vect_get_line_type(&Map, i);
	if (!(ltype & GV_POINT))
	    continue;

	Vect_read_line(&Map, Points, Cats, i);
	node = Vect_find_node(&Map, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	if (!node) {
	    G_warning(_("Point is not connected to the network"));
	    continue;
	}
	if (!(Vect_cat_get(Cats, nfield, &cat)))
	    continue;
	if (Vect_cat_in_cat_list(cat, catlist)) {
	    Vect_net_get_node_cost(&Map, node, &n1cost);
	    if (n1cost == -1) {	/* closed */
		G_warning(_("Centre at closed node (costs = -1) ignored"));
	    }
	    else {
		if (acentres == ncentres) {
		    acentres += 1;
		    Centers =
			(CENTER *) G_realloc(Centers,
					     acentres * sizeof(CENTER));
		}
		Centers[ncentres].cat = cat;
		Centers[ncentres].node = node;
		G_debug(2, "centre = %d node = %d cat = %d", ncentres,
			node, cat);
		ncentres++;
	    }
	}
    }

    G_message(_("Number of centres: %d (nlayer %d)"), ncentres, nfield);

    if (ncentres == 0)
	G_warning(_("Not enough centres for selected nlayer. Nothing will be allocated."));

    /* alloc and reset space for all nodes */
    Nodes = (NODE *) G_calloc((nnodes + 1), sizeof(NODE));
    for (i = 1; i <= nnodes; i++) {
	Nodes[i].centre = -1;
    }

    apnts1 = 1;
    pnts1 = (ISOPOINT *) G_malloc(apnts1 * sizeof(ISOPOINT));

    apnts2 = 1;
    pnts2 = (ISOPOINT *) G_malloc(apnts2 * sizeof(ISOPOINT));

    /* Fill Nodes by neares centre and costs from that centre */
    for (centre = 0; centre < ncentres; centre++) {
	node1 = Centers[centre].node;
	Vect_net_get_node_cost(&Map, node1, &n1cost);
	G_debug(2, "centre = %d node = %d cat = %d", centre, node1,
		Centers[centre].cat);
	G_message(_("Calculating costs from centre %d..."), centre + 1);
	for (node2 = 1; node2 <= nnodes; node2++) {
	    G_percent(node2, nnodes, 1);
	    G_debug(5, "  node1 = %d node2 = %d", node1, node2);
	    Vect_net_get_node_cost(&Map, node2, &n2cost);
	    if (n2cost == -1) {
		continue;
	    }			/* closed, left it as not attached */

	    ret = Vect_net_shortest_path(&Map, node1, node2, NULL, &cost);
	    if (ret == -1) {
		continue;
	    }			/* node unreachable */

	    /* We must add centre node costs (not calculated by Vect_net_shortest_path() ), but
	     *  only if centre and node are not identical, because at the end node cost is add later */
	    if (node1 != node2)
		cost += n1cost;
	    G_debug(5,
		    "Arc nodes: %d %d cost: %f (x old cent: %d old cost %f",
		    node1, node2, cost, Nodes[node2].centre,
		    Nodes[node2].cost);
	    if (Nodes[node2].centre == -1 || cost < Nodes[node2].cost) {
		Nodes[node2].cost = cost;
		Nodes[node2].centre = centre;
	    }
	}
    }

    /* Write arcs to new map */
    Vect_open_new(&Out, output->answer, Vect_is_3d(&Map));
    Vect_hist_command(&Out);

    G_message("Generating isolines...");
    nlines = Vect_get_num_lines(&Map);
    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 2);
	
	ltype = Vect_read_line(&Map, Points, NULL, line);
	if (!(ltype & type)) {
	    continue;
	}
	Vect_get_line_nodes(&Map, line, &node1, &node2);
	centre1 = Nodes[node1].centre;
	centre2 = Nodes[node2].centre;
	s1cost = Nodes[node1].cost;
	s2cost = Nodes[node2].cost;
	l = Vect_line_length(Points);

	if (l == 0)
	    continue;

	G_debug(3, "Line %d : length = %f", line, l);
	G_debug(3, "Arc centres: %d %d (nodes: %d %d)", centre1, centre2,
		node1, node2);

	Vect_net_get_node_cost(&Map, node1, &n1cost);
	Vect_net_get_node_cost(&Map, node2, &n2cost);

	Vect_net_get_line_cost(&Map, line, GV_FORWARD, &e1cost);
	Vect_net_get_line_cost(&Map, line, GV_BACKWARD, &e2cost);

	G_debug(3, "  s1cost = %f n1cost = %f e1cost = %f", s1cost, n1cost,
		e1cost);
	G_debug(3, "  s2cost = %f n2cost = %f e2cost = %f", s2cost, n2cost,
		e2cost);


	/* First check if arc is reachable from at least one side */
	if ((centre1 != -1 && n1cost != -1 && e1cost != -1) ||
	    (centre2 != -1 && n2cost != -1 && e2cost != -1)) {
	    /* Line is reachable at least from one side */
	    G_debug(3, "  -> arc is reachable");

	    /* Add costs of node to starting costs */
	    s1cost += n1cost;
	    s2cost += n2cost;

	    e1cost /= l;
	    e2cost /= l;

	    /* Find points on isolines along the line in both directions, add them to array,
	     *  first point is placed at the beginning/end of line */
	    /* Forward */
	    npnts1 = 0;		/* in case this direction is closed */
	    if (centre1 != -1 && n1cost != -1 && e1cost != -1) {
		/* Find iso for beginning of the line */
		next_iso = 0;
		for (i = niso - 1; i >= 0; i--) {
		    if (iso[i] <= s1cost) {
			next_iso = i;
			break;
		    }
		}
		/* Add first */
		pnts1[0].iso = next_iso;
		pnts1[0].distance = 0;
		npnts1++;
		next_iso++;

		/* Calculate distances for points along line */
		while (next_iso < niso) {
		    if (e1cost == 0)
			break;	/* Outside line */
		    l1 = (iso[next_iso] - s1cost) / e1cost;
		    if (l1 >= l)
			break;	/* Outside line */

		    if (npnts1 == apnts1) {
			apnts1 += 1;
			pnts1 =
			    (ISOPOINT *) G_realloc(pnts1,
						   apnts1 * sizeof(ISOPOINT));
		    }
		    pnts1[npnts1].iso = next_iso;
		    pnts1[npnts1].distance = l1;
		    G_debug(3,
			    "  forward %d : iso %d : distance %f : cost %f",
			    npnts1, next_iso, l1, iso[next_iso]);
		    npnts1++;
		    next_iso++;
		}
	    }
	    G_debug(3, "  npnts1 = %d", npnts1);

	    /* Backward */
	    npnts2 = 0;
	    if (centre2 != -1 && n2cost != -1 && e2cost != -1) {
		/* Find iso for beginning of the line */
		next_iso = 0;
		for (i = niso - 1; i >= 0; i--) {
		    if (iso[i] <= s2cost) {
			next_iso = i;
			break;
		    }
		}
		/* Add first */
		pnts2[0].iso = next_iso;
		pnts2[0].distance = l;
		npnts2++;
		next_iso++;

		/* Calculate distances for points along line */
		while (next_iso < niso) {
		    if (e2cost == 0)
			break;	/* Outside line */
		    l1 = (iso[next_iso] - s2cost) / e2cost;
		    if (l1 >= l)
			break;	/* Outside line */

		    if (npnts2 == apnts2) {
			apnts2 += 1;
			pnts2 =
			    (ISOPOINT *) G_realloc(pnts2,
						   apnts2 * sizeof(ISOPOINT));
		    }
		    pnts2[npnts2].iso = next_iso;
		    pnts2[npnts2].distance = l - l1;
		    G_debug(3,
			    "  backward %d : iso %d : distance %f : cost %f",
			    npnts2, next_iso, l - l1, iso[next_iso]);
		    npnts2++;
		    next_iso++;
		}
	    }
	    G_debug(3, "  npnts2 = %d", npnts2);

	    /* Limit number of points by maximum costs in reverse direction, this may remove
	     *  also the first point in one direction, but not in both */
	    /* Forward */
	    if (npnts2 > 0) {
		for (i = 0; i < npnts1; i++) {
		    G_debug(3,
			    "  pnt1 = %d dist1 = %f iso1 = %d max iso2 = %d",
			    i, pnts1[i].distance, pnts1[i].iso,
			    pnts2[npnts2 - 1].iso);
		    if (pnts2[npnts2 - 1].iso < pnts1[i].iso) {
			G_debug(3, "    -> cut here");
			npnts1 = i;
			break;
		    }
		}
	    }
	    G_debug(3, "  npnts1 cut = %d", npnts1);

	    /* Backward */
	    if (npnts1 > 0) {
		for (i = 0; i < npnts2; i++) {
		    G_debug(3,
			    "  pnt2 = %d dist2 = %f iso2 = %d max iso1 = %d",
			    i, pnts2[i].distance, pnts2[i].iso,
			    pnts1[npnts1 - 1].iso);
		    if (pnts1[npnts1 - 1].iso < pnts2[i].iso) {
			G_debug(3, "    -> cut here");
			npnts2 = i;
			break;
		    }
		}
	    }
	    G_debug(3, "  npnts2 cut = %d", npnts2);

	    /* Biggest cost shoud be equal if exist (npnts > 0). Cut out overlapping segments,
	     *  this can cut only points on line but not first points */
	    if (npnts1 > 1 && npnts2 > 1) {
		while (npnts1 > 1 && npnts2 > 1) {
		    if (pnts1[npnts1 - 1].distance >= pnts2[npnts2 - 1].distance) {	/* overlap */
			npnts1--;
			npnts2--;
		    }
		    else {
			break;
		    }
		}
	    }
	    G_debug(3, "  npnts1 2. cut = %d", npnts1);
	    G_debug(3, "  npnts2 2. cut = %d", npnts2);

	    /* Now we have points in both directions which may not overlap, npoints in one
	     *  direction may be 0 but not both */

	    /* Join both arrays, iso of point is for next segment (point is at the beginning) */
	    /* In case npnts1 == 0 add point at distance 0 */
	    if (npnts1 == 0) {
		G_debug(3,
			"  npnts1 = 0 -> add first at distance 0, cat = %d",
			pnts2[npnts2 - 1].iso);
		pnts1[0].iso = pnts2[npnts2 - 1].iso;	/* use last point iso in reverse direction */
		pnts1[0].distance = 0;
		npnts1++;
	    }
	    for (i = npnts2 - 1; i >= 0; i--) {
		/* Check if identical */
		if (pnts1[npnts1 - 1].distance == pnts2[i].distance)
		    continue;

		if (npnts1 == apnts1) {
		    apnts1 += 1;
		    pnts1 =
			(ISOPOINT *) G_realloc(pnts1,
					       apnts1 * sizeof(ISOPOINT));
		}
		pnts1[npnts1].iso = pnts2[i].iso - 1;	/* last may be -1, but it is not used */
		pnts1[npnts1].distance = pnts2[i].distance;
		npnts1++;
	    }
	    /* In case npnts2 == 0 add point at the end */
	    if (npnts2 == 0) {
		pnts1[npnts1].iso = 0;	/* not used */
		pnts1[npnts1].distance = l;
		npnts1++;
	    }

	    /* Create line segments. */
	    for (i = 1; i < npnts1; i++) {
		cat = pnts1[i - 1].iso + 1;
		G_debug(3, "  segment %f - %f cat %d", pnts1[i - 1].distance,
			pnts1[i].distance, cat);
		ret =
		    Vect_line_segment(Points, pnts1[i - 1].distance,
				      pnts1[i].distance, SPoints);
		if (ret == 0) {
		    G_warning(_("Cannot get line segment, segment out of line"));
		}
		else {
		    Vect_reset_cats(Cats);
		    Vect_cat_set(Cats, 1, cat);
		    Vect_write_line(&Out, ltype, SPoints, Cats);
		}
	    }
	}
	else {
	    /* arc is not reachable */
	    G_debug(3, "  -> arc is not reachable");
	    Vect_reset_cats(Cats);
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
