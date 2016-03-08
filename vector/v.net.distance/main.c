
/****************************************************************
 *
 * MODULE:     v.net.distance
 *
 * AUTHOR(S):  Daniel Bundala
 *             Markus Metz
 *
 * PURPOSE:    Computes shortest distance via the network between
 *             two given sets of features.
 *
 * COPYRIGHT:  (C) 2009-2010, 2012 by Daniel Bundala, and the GRASS Development Team
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
#include <grass/dbmi.h>
#include <grass/neta.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points, *PPoints;
    struct line_cats *Cats, *TCats;
    struct ilist *slist;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *catf_opt, *fieldf_opt, *wheref_opt;
    struct Option *catt_opt, *fieldt_opt, *wheret_opt, *typet_opt;
    struct Option *afield_opt, *nfield_opt, *abcol, *afcol, *ncol, *atype_opt;
    struct Flag *geo_f, *segments_f;
    int with_z, geo, segments;
    int atype, ttype;
    struct varray *varrayf, *varrayt;
    int flayer, tlayer;
    int afield, nfield;
    dglGraph_s *graph;
    struct ilist *nodest;
    int i, j, nnodes, nlines;
    int *dst, *nodes_to_features;
    int from_nr;			/* 'from' features not reachable */
    dglInt32_t **nxt;
    struct line_cats **on_path;
    char *segdir;
    char buf[2000];

    /* Attribute table */
    dbString sql;
    dbDriver *driver;
    struct field_info *Fi;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("shortest path"));
    module->label = _("Computes shortest distance via the network between "
		      "the given sets of features.");
    module->description =
	_("Finds the shortest paths from each 'from' point to the nearest 'to' feature "
	 "and various information about this relation are uploaded to the attribute table.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "arc_layer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");
    afield_opt->guisection = _("Cost");

    atype_opt = G_define_standard_option(G_OPT_V_TYPE);
    atype_opt->key = "arc_type";
    atype_opt->options = "line,boundary";
    atype_opt->answer = "line,boundary";
    atype_opt->label = _("Arc type");
    atype_opt->guisection = _("Cost");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "node_layer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");
    nfield_opt->guisection = _("Cost");

    fieldf_opt = G_define_standard_option(G_OPT_V_FIELD);
    fieldf_opt->key = "from_layer";
    fieldf_opt->label = _("From layer number or name");
    fieldf_opt->guisection = _("From");

    catf_opt = G_define_standard_option(G_OPT_V_CATS);
    catf_opt->key = "from_cats";
    catf_opt->label = _("From category values");
    catf_opt->guisection = _("From");

    wheref_opt = G_define_standard_option(G_OPT_DB_WHERE);
    wheref_opt->key = "from_where";
    wheref_opt->label =
	_("From WHERE conditions of SQL statement without 'where' keyword");
    wheref_opt->guisection = _("From");

    fieldt_opt = G_define_standard_option(G_OPT_V_FIELD);
    fieldt_opt->key = "to_layer";
    fieldt_opt->description = _("To layer number or name");
    fieldt_opt->guisection = _("To");

    typet_opt = G_define_standard_option(G_OPT_V_TYPE);
    typet_opt->key = "to_type";
    typet_opt->options = "point,line,boundary";
    typet_opt->answer = "point";
    typet_opt->description = _("To feature type");
    typet_opt->guisection = _("To");

    catt_opt = G_define_standard_option(G_OPT_V_CATS);
    catt_opt->key = "to_cats";
    catt_opt->label = _("To category values");
    catt_opt->guisection = _("To");

    wheret_opt = G_define_standard_option(G_OPT_DB_WHERE);
    wheret_opt->key = "to_where";
    wheret_opt->label =
	_("To WHERE conditions of SQL statement without 'where' keyword");
    wheret_opt->guisection = _("To");

    afcol = G_define_standard_option(G_OPT_DB_COLUMN);
    afcol->key = "arc_column";
    afcol->required = NO;
    afcol->description =
	_("Arc forward/both direction(s) cost column (number)");
    afcol->guisection = _("Cost");

    abcol = G_define_standard_option(G_OPT_DB_COLUMN);
    abcol->key = "arc_backward_column";
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column (number)");
    abcol->guisection = _("Cost");

    ncol = G_define_standard_option(G_OPT_DB_COLUMN);
    ncol->key = "node_column";
    ncol->required = NO;
    ncol->description = _("Node cost column (number)");
    ncol->guisection = _("Cost");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    segments_f = G_define_flag();
#if 0
    /* use this to sync with v.net.path */
    segments_f->key = 's';
    segments_f->description = _("Write output as original input segments, "
				"not each path as one line.");
#else
    segments_f->key = 'l';
    segments_f->description = _("Write each output path as one line, "
				"not as original input segments.");
#endif

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    atype = Vect_option_to_types(atype_opt);
    ttype = Vect_option_to_types(typet_opt);

    Points = Vect_new_line_struct();
    PPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    TCats = Vect_new_cats_struct();
    slist = G_new_ilist();

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


    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_warning(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

#if 0
    /* use this to sync with v.net.path */
    segments = segments_f->answer;
#else
    segments = !segments_f->answer;
#endif

    nnodes = Vect_get_num_nodes(&In);
    nlines = Vect_get_num_lines(&In);

    dst = (int *)G_calloc(nnodes + 1, sizeof(int));
    nxt = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    nodes_to_features = (int *)G_calloc(nnodes + 1, sizeof(int));
    on_path =
	(struct line_cats **)G_calloc(nlines + 1, sizeof(struct line_cats *));
    segdir = (char *)G_calloc(nlines + 1, sizeof(char));

    if (!dst || !nxt || !nodes_to_features || !on_path || !segdir)
	G_fatal_error(_("Out of memory"));

    for (i = 1; i <= nlines; i++) {
	on_path[i] = Vect_new_cats_struct();
	segdir[i] = 0;
    }

    /*initialise varrays and nodes list appropriatelly */
    afield = Vect_get_field_number(&In, afield_opt->answer);
    nfield = Vect_get_field_number(&In, nfield_opt->answer);

    flayer = atoi(fieldf_opt->answer);
    tlayer = atoi(fieldt_opt->answer);

    if (NetA_initialise_varray(&In, flayer, GV_POINT, wheref_opt->answer,
			   catf_opt->answer, &varrayf) <= 0) {
	G_fatal_error(_("No 'from' features selected. "
			"Please check options '%s', '%s', '%s'."),
			fieldf_opt->key, wheref_opt->key, catf_opt->key);
    }

    if (NetA_initialise_varray(&In, tlayer, ttype, wheret_opt->answer,
			   catt_opt->answer, &varrayt) <= 0) {
	G_fatal_error(_("No 'to' features selected. "
			"Please check options '%s', '%s', '%s'."),
			fieldt_opt->key, wheret_opt->key, catt_opt->key);
    }

    nodest = Vect_new_list();
    NetA_varray_to_nodes(&In, varrayt, nodest, nodes_to_features);
    
    if (nodest->n_values == 0)
	G_fatal_error(_("No 'to' features"));
    
    if (0 != Vect_net_build_graph(&In, atype, afield, nfield, afcol->answer, abcol->answer,
                                   ncol->answer, geo, 2))
        G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(&In));

    graph = Vect_net_get_graph(&In);

    G_message(_("Distances to 'to' features ..."));

    NetA_distance_to_points(graph, nodest, dst, nxt);

    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    db_set_error_handler_driver(driver);

    sprintf(buf,
	    "create table %s ( cat integer, tcat integer, dist double precision)",
	    Fi->table);

    db_set_string(&sql, buf);
    G_debug(2, "%s", db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }

    if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	G_warning(_("Cannot create index"));

    if (db_grant_on_table
	(driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    G_message(_("Tracing paths from 'from' features ..."));
    from_nr = 0;
    for (i = 1; i <= nlines; i++) {
	if (varrayf->c[i]) {
	    int type = Vect_read_line(&In, Points, Cats, i);
	    int node, tcat, cat;
	    double cost;
	    dglInt32_t *vertex, vertex_id;

	    if (!Vect_cat_get(Cats, flayer, &cat))
		continue;
		
	    if (type & GV_POINTS) {
		node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	    }
	    else {
		Vect_get_line_nodes(&In, i, &node, NULL);
	    }
	    if (node < 1)
		continue;
	    if (dst[node] < 0) {
		/* unreachable */
		from_nr++;
 		continue;
	    }
	    cost = dst[node] / (double)In.dgraph.cost_multip;
	    vertex = dglGetNode(graph, node);
	    vertex_id = node;
	    slist->n_values = 0;
	    while (nxt[vertex_id] != NULL) {
		int edge_id;

		edge_id = (int) dglEdgeGet_Id(graph, nxt[vertex_id]);
		if (segments) {
		    Vect_cat_set(on_path[abs(edge_id)], 1, cat);
		    if (edge_id < 0) {
			segdir[abs(edge_id)] = 1;
		    }
		}
		else
		    G_ilist_add(slist, edge_id);

		vertex = dglEdgeGet_Tail(graph, nxt[vertex_id]);
		vertex_id = dglNodeGet_Id(graph, vertex);
	    }
	    G_debug(3, "read line %d, vertex id %d", nodes_to_features[vertex_id], (int)vertex_id);
	    Vect_read_line(&In, NULL, TCats, nodes_to_features[vertex_id]);
	    if (!Vect_cat_get(TCats, tlayer, &tcat))
		continue;

	    Vect_write_line(&Out, type, Points, Cats);
	    sprintf(buf, "insert into %s values (%d, %d, %f)", Fi->table, cat,
		    tcat, cost);
	    db_set_string(&sql, buf);
	    G_debug(3, "%s", db_get_string(&sql));
	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		G_fatal_error(_("Cannot insert new record: %s"),
			      db_get_string(&sql));
	    };

	    if (!segments) {
		Vect_reset_line(PPoints);
		for (j = 0; j < slist->n_values; j++) {
		    Vect_read_line(&In, Points, NULL, abs(slist->value[j]));
		    if (slist->value[j] > 0)
			Vect_append_points(PPoints, Points,
					   GV_FORWARD);
		    else
			Vect_append_points(PPoints, Points,
					   GV_BACKWARD);
		    PPoints->n_points--;
		}
		PPoints->n_points++;
		Vect_reset_cats(Cats);
		Vect_cat_set(Cats, 1, cat);
		Vect_write_line(&Out, GV_LINE, PPoints, Cats);
	    }

	}
    }

    if (segments) {
	for (i = 1; i <= nlines; i++) {
	    if (on_path[i]->n_cats > 0) {
		int type; 
		
		if (segdir[i]) {
		    type = Vect_read_line(&In, PPoints, NULL, i);
		    Vect_reset_line(Points);
		    Vect_append_points(Points, PPoints, GV_BACKWARD);
		}
		else
		    type = Vect_read_line(&In, Points, NULL, i);

		Vect_write_line(&Out, type, Points, on_path[i]);
	    }
	}
    }

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    for (i = 1; i <= nlines; i++)
	Vect_destroy_cats_struct(on_path[i]);
    G_free(on_path);
    G_free(nodes_to_features);
    G_free(dst);
    G_free(nxt);
    G_free(segdir);

    if (from_nr)
	G_warning(n_("%d 'from' feature was not reachable",
                     "%d 'from' features were not reachable",
                     from_nr), from_nr);

    exit(EXIT_SUCCESS);
}
