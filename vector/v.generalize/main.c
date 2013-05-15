
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *             OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *             Markus Metz: preserve areas and area attributes
 *
 * PURPOSE:    Module for line simplification and smoothing
 *
 * COPYRIGHT:  (C) 2007-2010, 2012 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2).  Read the file COPYING that
 *             comes with GRASS for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "misc.h"
#include "operators.h"

#define DOUGLAS 0
#define LANG 1
#define VERTEX_REDUCTION 2
#define REUMANN 3
#define BOYLE 4
#define DISTANCE_WEIGHTING 5
#define CHAIKEN 6
#define HERMITE 7
#define SNAKES 8
#define DOUGLAS_REDUCTION 9
#define SLIDING_AVERAGING 10
#define NETWORK 100
#define DISPLACEMENT 101

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i, type, iter;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out, *thresh_opt, *method_opt,
	*look_ahead_opt;
    struct Option *iterations_opt, *cat_opt, *alpha_opt, *beta_opt, *type_opt;
    struct Option *field_opt, *where_opt, *reduction_opt, *slide_opt;
    struct Option *angle_thresh_opt, *degree_thresh_opt,
	*closeness_thresh_opt;
    struct Option *betweeness_thresh_opt;
    struct Flag *notab_flag, *loop_support_flag;
    int with_z;
    int total_input, total_output;	/* Number of points in the input/output map respectively */
    double thresh, alpha, beta, reduction, slide, angle_thresh;
    double degree_thresh, closeness_thresh, betweeness_thresh;
    int method;
    int look_ahead, iterations;
    int loop_support;
    int layer;
    int n_lines;
    int simplification, mask_type;
    struct cat_list *cat_list = NULL;
    char *s, *descriptions;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("generalization"));
    G_add_keyword(_("simplification"));
    G_add_keyword(_("smoothing"));
    G_add_keyword(_("displacement"));
    G_add_keyword(_("network generalization"));
    module->description = _("Performs vector based generalization.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary,area";
    type_opt->answer = "line,boundary,area";
    type_opt->guisection = _("Selection");
    
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);


    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->multiple = NO;
    method_opt->options =
	"douglas,douglas_reduction,lang,reduction,reumann,boyle,sliding_averaging,distance_weighting,chaiken,hermite,snakes,network,displacement";
    descriptions = NULL;
    G_asprintf(&descriptions,
               "douglas;%s;"
               "douglas_reduction;%s;"
               "lang;%s;"
               "reduction;%s;"
               "reumann;%s;"
               "boyle;%s;"
               "sliding_averaging;%s;"
               "distance_weighting;%s;"
               "chaiken;%s;"
               "hermite;%s;"
               "snakes;%s;"
               "network;%s;"
               "displacement;%s;",
               _("Douglas-Peucker Algorithm"),
               _("Douglas-Peucker Algorithm with reduction parameter"),
               _("Lang Simplification Algorithm"),
               _("Vertex Reduction Algorithm eliminates points close to each other"),
               _("Reumann-Witkam Algorithm"),
               _("Boyle's Forward-Looking Algorithm"),
               _("McMaster's Sliding Averaging Algorithm"),
               _("McMaster's Distance-Weighting Algorithm"),
               _("Chaiken's Algorithm"),
               _("Interpolation by Cubic Hermite Splines"),
               _("Snakes method for line smoothing"),
               _("Network generalization"),
               _("Displacement of lines close to each other"));
    method_opt->descriptions = G_store(descriptions);
    
    method_opt->description = _("Generalization algorithm");

    thresh_opt = G_define_option();
    thresh_opt->key = "threshold";
    thresh_opt->type = TYPE_DOUBLE;
    thresh_opt->required = YES;
    thresh_opt->options = "0-1000000000";
    thresh_opt->description = _("Maximal tolerance value");

    look_ahead_opt = G_define_option();
    look_ahead_opt->key = "look_ahead";
    look_ahead_opt->type = TYPE_INTEGER;
    look_ahead_opt->required = NO;
    look_ahead_opt->answer = "7";
    look_ahead_opt->description = _("Look-ahead parameter");

    reduction_opt = G_define_option();
    reduction_opt->key = "reduction";
    reduction_opt->type = TYPE_DOUBLE;
    reduction_opt->required = NO;
    reduction_opt->answer = "50";
    reduction_opt->options = "0-100";
    reduction_opt->description =
	_("Percentage of the points in the output of 'douglas_reduction' algorithm");
    
    slide_opt = G_define_option();
    slide_opt->key = "slide";
    slide_opt->type = TYPE_DOUBLE;
    slide_opt->required = NO;
    slide_opt->answer = "0.5";
    slide_opt->options = "0-1";
    slide_opt->description =
	_("Slide of computed point toward the original point");

    angle_thresh_opt = G_define_option();
    angle_thresh_opt->key = "angle_thresh";
    angle_thresh_opt->type = TYPE_DOUBLE;
    angle_thresh_opt->required = NO;
    angle_thresh_opt->answer = "3";
    angle_thresh_opt->options = "0-180";
    angle_thresh_opt->description =
	_("Minimum angle between two consecutive segments in Hermite method");

    degree_thresh_opt = G_define_option();
    degree_thresh_opt->key = "degree_thresh";
    degree_thresh_opt->type = TYPE_INTEGER;
    degree_thresh_opt->required = NO;
    degree_thresh_opt->answer = "0";
    degree_thresh_opt->description =
	_("Degree threshold in network generalization");

    closeness_thresh_opt = G_define_option();
    closeness_thresh_opt->key = "closeness_thresh";
    closeness_thresh_opt->type = TYPE_DOUBLE;
    closeness_thresh_opt->required = NO;
    closeness_thresh_opt->answer = "0";
    closeness_thresh_opt->options = "0-1";
    closeness_thresh_opt->description =
	_("Closeness threshold in network generalization");

    betweeness_thresh_opt = G_define_option();
    betweeness_thresh_opt->key = "betweeness_thresh";
    betweeness_thresh_opt->type = TYPE_DOUBLE;
    betweeness_thresh_opt->required = NO;
    betweeness_thresh_opt->answer = "0";
    betweeness_thresh_opt->description =
	_("Betweeness threshold in network generalization");

    alpha_opt = G_define_option();
    alpha_opt->key = "alpha";
    alpha_opt->type = TYPE_DOUBLE;
    alpha_opt->required = NO;
    alpha_opt->answer = "1.0";
    alpha_opt->description = _("Snakes alpha parameter");

    beta_opt = G_define_option();
    beta_opt->key = "beta";
    beta_opt->type = TYPE_DOUBLE;
    beta_opt->required = NO;
    beta_opt->answer = "1.0";
    beta_opt->description = _("Snakes beta parameter");

    iterations_opt = G_define_option();
    iterations_opt->key = "iterations";
    iterations_opt->type = TYPE_INTEGER;
    iterations_opt->required = NO;
    iterations_opt->answer = "1";
    iterations_opt->description = _("Number of iterations");

    cat_opt = G_define_standard_option(G_OPT_V_CATS);
    cat_opt->guisection = _("Selection");
    
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    loop_support_flag = G_define_flag();
    loop_support_flag->key = 'l';
    loop_support_flag->label = _("Loop support");
    loop_support_flag->description = _("Modify end points of lines forming a closed loop");

    notab_flag = G_define_standard_flag(G_FLG_V_TABLE);
    notab_flag->description = _("Do not copy attributes");
    notab_flag->guisection = _("Attributes");
    
    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    thresh = atof(thresh_opt->answer);
    look_ahead = atoi(look_ahead_opt->answer);
    alpha = atof(alpha_opt->answer);
    beta = atof(beta_opt->answer);
    reduction = atof(reduction_opt->answer);
    iterations = atoi(iterations_opt->answer);
    slide = atof(slide_opt->answer);
    angle_thresh = atof(angle_thresh_opt->answer);
    degree_thresh = atof(degree_thresh_opt->answer);
    closeness_thresh = atof(closeness_thresh_opt->answer);
    betweeness_thresh = atof(betweeness_thresh_opt->answer);

    mask_type = type_mask(type_opt);
    G_debug(3, "Method: %s", method_opt->answer);

    s = method_opt->answer;

    if (strcmp(s, "douglas") == 0)
	method = DOUGLAS;
    else if (strcmp(s, "lang") == 0)
	method = LANG;
    else if (strcmp(s, "reduction") == 0)
	method = VERTEX_REDUCTION;
    else if (strcmp(s, "reumann") == 0)
	method = REUMANN;
    else if (strcmp(s, "boyle") == 0)
	method = BOYLE;
    else if (strcmp(s, "distance_weighting") == 0)
	method = DISTANCE_WEIGHTING;
    else if (strcmp(s, "chaiken") == 0)
	method = CHAIKEN;
    else if (strcmp(s, "hermite") == 0)
	method = HERMITE;
    else if (strcmp(s, "snakes") == 0)
	method = SNAKES;
    else if (strcmp(s, "douglas_reduction") == 0)
	method = DOUGLAS_REDUCTION;
    else if (strcmp(s, "sliding_averaging") == 0)
	method = SLIDING_AVERAGING;
    else if (strcmp(s, "network") == 0)
	method = NETWORK;
    else if (strcmp(s, "displacement") == 0) {
	method = DISPLACEMENT;
	/* we can displace only the lines */
	mask_type = GV_LINE;
    }
    else {
	G_fatal_error(_("Unknown method"));
	exit(EXIT_FAILURE);
    }


    /* simplification or smoothing? */
    switch (method) {
    case DOUGLAS:
    case DOUGLAS_REDUCTION:
    case LANG:
    case VERTEX_REDUCTION:
    case REUMANN:
	simplification = 1;
	break;
    default:
	simplification = 0;
	break;
    }


    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(map_in->answer, map_out->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);

    if (Vect_open_old2(&In, map_in->answer, "", field_opt->answer) < 1)
	G_fatal_error(_("Unable to open vector map <%s>"), map_in->answer);

    with_z = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, map_out->answer, with_z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), map_out->answer);
    }


    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    total_input = total_output = 0;

    layer = Vect_get_field_number(&In, field_opt->answer);
    /* parse filter options */
    if (layer > 0)
	cat_list = Vect_cats_set_constraint(&In, layer, 
			      where_opt->answer, cat_opt->answer);

    if (method == DISPLACEMENT) {
	/* modifies only lines, all other features including boundaries are preserved */
	/* options where, cats, and layer are respected */
	G_message(_("Displacement..."));
	snakes_displacement(&In, &Out, thresh, alpha, beta, 1.0, 10.0,
			    iterations, cat_list, layer);
    }

    /* TODO: rearrange code below. It's really messy */
    if (method == NETWORK) {
	/* extracts lines of selected type, all other features are discarded */
	/* options where, cats, and layer are ignored */
	G_message(_("Network generalization..."));
	total_output =
	    graph_generalization(&In, &Out, mask_type, degree_thresh, 
	                         closeness_thresh, betweeness_thresh);
    }

    /* copy tables here because method == NETWORK is complete and 
     * tables for Out may be needed for parse_filter_options() below */
    if (!notab_flag->answer) {
	if (method == NETWORK)
	    copy_tables_by_cats(&In, &Out);
	else
	    Vect_copy_tables(&In, &Out, -1);
    }
    else if (where_opt->answer && method < NETWORK) {
	G_warning(_("Attributes are needed for 'where' option, copying table"));
	Vect_copy_tables(&In, &Out, -1);
    }

    /* smoothing/simplification */
    if (method < NETWORK) {
	/* modifies only lines of selected type, all other features are preserved */
	int not_modified_boundaries = 0, n_oversimplified = 0;
	struct line_pnts *APoints;  /* original Points */

	Vect_copy_map_lines(&In, &Out);
	Vect_build_partial(&Out, GV_BUILD_CENTROIDS);

	if ((mask_type & GV_AREA) && !(mask_type & GV_BOUNDARY))
	    mask_type |= GV_BOUNDARY;

	G_message("-----------------------------------------------------");
	G_message(_("Generalization (%s)..."), method_opt->answer);
	G_percent_reset();

	APoints = Vect_new_line_struct();

	n_lines = Vect_get_num_lines(&Out);
	for (i = 1; i <= n_lines; i++) {
	    int after = 0;

	    G_percent(i, n_lines, 1);

	    type = Vect_read_line(&Out, APoints, Cats, i);

	    if (!(type & GV_LINES) || !(mask_type & type))
		continue;

	    if (layer > 0) {
		if ((type & GV_LINE) &&
		    !Vect_cats_in_constraint(Cats, layer, cat_list))
		    continue;
		else if ((type & GV_BOUNDARY)) {
		    int do_line = 0;
		    int left, right;
		    
		    do_line = Vect_cats_in_constraint(Cats, layer, cat_list);

		    if (!do_line) {
			
			/* check if any of the centroids is selected */
			Vect_get_line_areas(&Out, i, &left, &right);
			if (left > 0) {
			    Vect_get_area_cats(&Out, left, Cats);
			    do_line = Vect_cats_in_constraint(Cats, layer, cat_list);
			}
			else if (left < 0) {
			    left = Vect_get_isle_area(&Out, abs(left));
			    if (left > 0) {
				Vect_get_area_cats(&Out, left, Cats);
				do_line = Vect_cats_in_constraint(Cats, layer, cat_list);
			    }
			}
			
			if (!do_line) {
			    if (right > 0) {
				Vect_get_area_cats(&Out, right, Cats);
				do_line = Vect_cats_in_constraint(Cats, layer, cat_list);
			    }
			    else if (right < 0) {
				right = Vect_get_isle_area(&Out, abs(right));
				if (right > 0) {
				    Vect_get_area_cats(&Out, right, Cats);
				    do_line = Vect_cats_in_constraint(Cats, layer, cat_list);
				}
			    }
			}
		    }
		    if (!do_line)
			continue;
		}
	    }

	    Vect_line_prune(APoints);

	    if (APoints->n_points < 2)
		/* Line of length zero, delete if boundary ? */
		continue;

	    total_input += APoints->n_points;

	    /* copy points */
	    Vect_reset_line(Points);
	    Vect_append_points(Points, APoints, GV_FORWARD);
	    
	    loop_support = 0;
	    if (loop_support_flag->answer) {
		int n1, n2;

		Vect_get_line_nodes(&Out, i, &n1, &n2);
		if (n1 == n2) {
		    if (Vect_get_node_n_lines(&Out, n1) == 2) {
			if (abs(Vect_get_node_line(&Out, n1, 0)) == i &&
			    abs(Vect_get_node_line(&Out, n1, 1)) == i)
			    loop_support = 1;
		    }
		}
	    }
		
	    for (iter = 0; iter < iterations; iter++) {
		switch (method) {
		case DOUGLAS:
		    douglas_peucker(Points, thresh, with_z);
		    break;
		case DOUGLAS_REDUCTION:
		    douglas_peucker_reduction(Points, thresh, reduction,
					      with_z);
		    break;
		case LANG:
		    lang(Points, thresh, look_ahead, with_z);
		    break;
		case VERTEX_REDUCTION:
		    vertex_reduction(Points, thresh, with_z);
		    break;
		case REUMANN:
		    reumann_witkam(Points, thresh, with_z);
		    break;
		case BOYLE:
		    boyle(Points, look_ahead, loop_support, with_z);
		    break;
		case SLIDING_AVERAGING:
		    sliding_averaging(Points, slide, look_ahead, loop_support, with_z);
		    break;
		case DISTANCE_WEIGHTING:
		    distance_weighting(Points, slide, look_ahead, loop_support, with_z);
		    break;
		case CHAIKEN:
		    chaiken(Points, thresh, loop_support, with_z);
		    break;
		case HERMITE:
		    hermite(Points, thresh, angle_thresh, loop_support, with_z);
		    break;
		case SNAKES:
		    snakes(Points, alpha, beta, loop_support, with_z);
		    break;
		}
	    }

	    if (loop_support == 0) { 
		/* safety check, BUG in method if not passed */
		if (APoints->x[0] != Points->x[0] || 
		    APoints->y[0] != Points->y[0] ||
		    APoints->z[0] != Points->z[0])
		    G_fatal_error(_("Method '%s' did not preserve first point"), method_opt->answer);
		    
		if (APoints->x[APoints->n_points - 1] != Points->x[Points->n_points - 1] || 
		    APoints->y[APoints->n_points - 1] != Points->y[Points->n_points - 1] ||
		    APoints->z[APoints->n_points - 1] != Points->z[Points->n_points - 1])
		    G_fatal_error(_("Method '%s' did not preserve last point"), method_opt->answer);
	    }
	    else {
		/* safety check, BUG in method if not passed */
		if (Points->x[0] != Points->x[Points->n_points - 1] || 
		    Points->y[0] != Points->y[Points->n_points - 1] ||
		    Points->z[0] != Points->z[Points->n_points - 1])
		    G_fatal_error(_("Method '%s' did not preserve loop"), method_opt->answer);
	    }

	    Vect_line_prune(Points);

	    /* oversimplified line */
	    if (Points->n_points < 2) {
		after = APoints->n_points;
		n_oversimplified++;
	    }
	    /* check for topology corruption */
	    else if (type == GV_BOUNDARY) {
		if (!check_topo(&Out, i, APoints, Points, Cats)) {
		    after = APoints->n_points;
		    not_modified_boundaries++;
		}
		else
		    after = Points->n_points;
	    }
	    else {
		/* type == GV_LINE */
		Vect_rewrite_line(&Out, i, type, Points, Cats);
		after = Points->n_points;
	    }

	    total_output += after;
	}
	if (not_modified_boundaries > 0)
	    G_message(_("%d boundaries were not modified because modification would damage topology"),
		      not_modified_boundaries);
	if (n_oversimplified > 0)
	    G_message(_("%d lines/boundaries were not modified due to over-simplification"),
		      n_oversimplified);
	G_message("-----------------------------------------------------");

	/* make sure that clean topo is built at the end */
	Vect_build_partial(&Out, GV_BUILD_NONE);
    }

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    G_message("-----------------------------------------------------");
    if (total_input != 0 && total_input != total_output)
	G_done_msg(_("Number of vertices for selected features %s from %d to %d (%d%%)."),
                   simplification ? _("reduced") : _("changed"), 
                   total_input, total_output,
                   (total_output * 100) / total_input);
    else
        G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
