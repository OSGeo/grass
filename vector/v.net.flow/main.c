
/****************************************************************
 *
 * MODULE:     v.net.flow
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Max flow and min cut between two sets of nodes
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
#include <grass/dbmi.h>
#include <grass/neta.h>


int main(int argc, char *argv[])
{
    struct Map_info In, Out, cut_map;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out, *cut_out;
    struct Option *afield_opt, *nfield_opt, *abcol, *afcol, *ncol;
    struct Option *catsource_opt, *wheresource_opt;
    struct Option *catsink_opt, *wheresink_opt;
    int with_z;
    int afield, nfield, mask_type;
    struct varray *varray_source, *varray_sink;
    dglGraph_s *graph;
    int i, nlines, *flow, total_flow;
    struct ilist *source_list, *sink_list, *cut;
    int find_cut;

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
    G_add_keyword(_("flow"));
    module->description =
	_("Computes the maximum flow between two sets of nodes in the network.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);

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

    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    cut_out = G_define_standard_option(G_OPT_V_OUTPUT);
    cut_out->key = "cut";
    cut_out->description =
	_("Name for output vector map containing a minimum cut");

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

    ncol = G_define_standard_option(G_OPT_DB_COLUMN);
    ncol->key = "ncolumn";
    ncol->required = NO;
    ncol->description = _("Node cost column (number)");
    ncol->guisection = _("Cost");

    catsource_opt = G_define_standard_option(G_OPT_V_CATS);
    catsource_opt->key = "source_cats";
    catsource_opt->label = _("Source category values");
    catsource_opt->guisection = _("Source");

    wheresource_opt = G_define_standard_option(G_OPT_DB_WHERE);
    wheresource_opt->key = "source_where";
    wheresource_opt->label =
	_("Source WHERE conditions of SQL statement without 'where' keyword");
    wheresource_opt->guisection = _("Source");

    catsink_opt = G_define_standard_option(G_OPT_V_CATS);
    catsink_opt->key = "sink_cats";
    catsink_opt->label = _("Sink category values");
    catsink_opt->guisection = _("Sink");

    wheresink_opt = G_define_standard_option(G_OPT_DB_WHERE);
    wheresink_opt->key = "sink_where";
    wheresink_opt->label =
	_("Sink WHERE conditions of SQL statement without 'where' keyword");
    wheresink_opt->guisection = _("Sink");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    find_cut = (cut_out->answer[0]);
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

    if (find_cut && 0 > Vect_open_new(&cut_map, cut_out->answer, with_z)) {
	Vect_close(&In);
	Vect_close(&Out);
	G_fatal_error(_("Unable to create vector map <%s>"), cut_out->answer);
    }

    /* parse filter option and select appropriate lines */
    afield = Vect_get_field_number(&In, afield_opt->answer);
    nfield = Vect_get_field_number(&In, nfield_opt->answer);

    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    sprintf(buf, "create table %s (cat integer, flow double precision)",
	    Fi->table);

    db_set_string(&sql, buf);
    G_debug(2, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }

    if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	G_warning(_("Cannot create index"));

    if (db_grant_on_table
	(driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);

    source_list = Vect_new_list();
    sink_list = Vect_new_list();

    if (NetA_initialise_varray
	(&In, nfield, GV_POINT,
	 wheresource_opt->answer, catsource_opt->answer, &varray_source) <= 0) {
	G_fatal_error(_("No source features selected. "
			"Please check options '%s', '%s'."),
			catsource_opt->key, wheresource_opt->key);
    }
    if (NetA_initialise_varray
	(&In, nfield, GV_POINT, wheresink_opt->answer,
	 catsink_opt->answer, &varray_sink) <= 0) {
	G_fatal_error(_("No sink features selected. "
			"Please check options '%s', '%s'."),
			catsink_opt->key, wheresink_opt->key);
    }

    NetA_varray_to_nodes(&In, varray_source, source_list, NULL);
    NetA_varray_to_nodes(&In, varray_sink, sink_list, NULL);

    if (source_list->n_values == 0)
	G_fatal_error(_("No sources"));

    if (sink_list->n_values == 0)
	G_fatal_error(_("No sinks"));

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    if (0 != Vect_net_build_graph(&In, mask_type, afield, nfield, afcol->answer, abcol->answer,
                                  ncol->answer, 0, 0))
        G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(&In));
    
    graph = Vect_net_get_graph(&In);
    nlines = Vect_get_num_lines(&In);
    flow = (int *)G_calloc(nlines + 1, sizeof(int));
    if (!flow)
	G_fatal_error(_("Out of memory"));

    total_flow = NetA_flow(graph, source_list, sink_list, flow);
    G_debug(3, "Max flow: %d", total_flow);
    if (find_cut) {
	cut = Vect_new_list();
	total_flow = NetA_min_cut(graph, source_list, sink_list, flow, cut);
	G_debug(3, "Min cut: %d", total_flow);
    }

    G_message(_("Writing the output..."));
    G_percent_reset();
    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 1);
	int type = Vect_read_line(&In, Points, Cats, i);

	Vect_write_line(&Out, type, Points, Cats);
	if (type == GV_LINE) {
	    int cat;

	    Vect_cat_get(Cats, afield, &cat);
	    if (cat == -1)
		continue;	/*TODO: warning? */
	    sprintf(buf, "insert into %s values (%d, %f)", Fi->table, cat,
		    flow[i] / (double)In.dgraph.cost_multip);
	    db_set_string(&sql, buf);
	    G_debug(3, db_get_string(&sql));

	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		db_close_database_shutdown_driver(driver);
		G_fatal_error(_("Cannot insert new record: %s"),
			      db_get_string(&sql));
	    };
	}
    }

    if (find_cut) {
	for (i = 0; i < cut->n_values; i++) {
	    int type = Vect_read_line(&In, Points, Cats, cut->value[i]);

	    Vect_write_line(&cut_map, type, Points, Cats);
	}
	Vect_destroy_list(cut);

	Vect_build(&cut_map);
	Vect_close(&cut_map);
    }

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    G_free(flow);
    Vect_destroy_list(source_list);
    Vect_destroy_list(sink_list);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
