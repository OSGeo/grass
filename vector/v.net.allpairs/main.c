
/****************************************************************
 *
 * MODULE:     v.net.allpairs
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Shortest paths between all nodes
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
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *cat_opt, *field_opt, *where_opt, *abcol, *afcol;
    struct Flag *geo_f, *newpoints_f;
    int chcat, with_z;
    int layer, mask_type;
    struct varray *varray;
    dglGraph_s *graph;
    int i, j, geo, nnodes, nlines, max_cat, *cats;
    dglInt32_t **dist;
    char buf[2000], *output;

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
    module->description =
	_("Computes the shortest path between all pairs of nodes in the network.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->guisection = _("Selection");
    cat_opt = G_define_standard_option(G_OPT_V_CATS);
    cat_opt->guisection = _("Selection");
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    afcol = G_define_standard_option(G_OPT_DB_COLUMN);
    afcol->key = "afcolumn";
    afcol->required = NO;
    afcol->description =
	_("Name of arc forward/both direction(s) cost column");
    afcol->guisection = _("Cost");

    abcol = G_define_standard_option(G_OPT_DB_COLUMN);
    abcol->key = "abcolumn";
    abcol->required = NO;
    abcol->description = _("Name of arc backward direction cost column");
    abcol->guisection = _("Cost");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    newpoints_f = G_define_flag();
    newpoints_f->key = 'a';
    newpoints_f->description = _("Add points on nodes without points");

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

    /* parse filter option and select appropriate lines */
    layer = atoi(field_opt->answer);
    chcat =
	(NetA_initialise_varray
	 (&In, layer, GV_POINT, where_opt->answer, cat_opt->answer,
	  &varray) == 1);

    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    sprintf(buf,
	    "create table %s ( cat integer, to_cat integer, cost double precision)",
	    Fi->table);

    db_set_string(&sql, buf);
    G_debug(2, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }
    /*
     * if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
     * G_warning(_("Cannot create index"));
     */
    if (db_grant_on_table
	(driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);


    Vect_net_build_graph(&In, mask_type, atoi(field_opt->answer), 0,
			 afcol->answer, abcol->answer, NULL, geo, 0);
    graph = &(In.graph);
    nnodes = dglGet_NodeCount(graph);
    dist = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    cats = (int *)G_calloc(nnodes + 1, sizeof(int));	/*id of each node. -1 if not used */
    output = (char *)G_calloc(nnodes + 1, sizeof(char));

    if (!dist || !cats)
	G_fatal_error(_("Out of memory"));
    for (i = 0; i <= nnodes; i++) {
	dist[i] = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
	if (!dist[i])
	    G_fatal_error(_("Out of memory"));
    }
    NetA_allpairs(graph, dist);

    for (i = 1; i <= nnodes; i++) {
	cats[i] = -1;
	output[i] = 0;
    }

    nlines = Vect_get_num_lines(&In);
    max_cat = 0;
    for (i = 1; i <= nlines; i++) {
	int type = Vect_read_line(&In, Points, Cats, i);

	for (j = 0; j < Cats->n_cats; j++)
	    if (Cats->cat[j] > max_cat)
		max_cat = Cats->cat[j];
	if (type == GV_POINT) {
	    int node;

	    /* Vect_get_line_nodes(&In, i, &node, NULL); */
	    node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	    Vect_cat_get(Cats, layer, &cats[node]);
	    if (cats[node] != -1) {
		Vect_write_line(&Out, GV_POINT, Points, Cats);
		if (!chcat || varray->c[i])
		    output[node] = 'y';
	    }
	}

    }
    max_cat++;
    for (i = 1; i <= nnodes; i++)
	if (newpoints_f->answer && cats[i] == -1) {
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 1, max_cat);
	    cats[i] = max_cat++;
	    NetA_add_point_on_node(&In, &Out, i, Cats);
	}
    G_message(_("Writing data into the table..."));
    G_percent_reset();
    for (i = 1; i <= nnodes; i++) {
	G_percent(i, nnodes, 1);
	if (cats[i] != -1 && output[i])	/*Process only selected nodes */
	    for (j = 1; j <= nnodes; j++)
		if (cats[j] != -1) {
		    sprintf(buf, "insert into %s values (%d, %d, %f)",
			    Fi->table, cats[i], cats[j],
			    dist[i][j] / (double)In.cost_multip);
		    db_set_string(&sql, buf);
		    G_debug(3, db_get_string(&sql));

		    if (db_execute_immediate(driver, &sql) != DB_OK) {
			db_close_database_shutdown_driver(driver);
			G_fatal_error(_("Cannot insert new record: %s"),
				      db_get_string(&sql));
		    };
		}
    }
    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    for (i = 0; i <= nnodes; i++)
	G_free(dist[i]);
    G_free(dist);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
