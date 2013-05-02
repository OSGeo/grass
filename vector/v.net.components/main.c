
/****************************************************************
 *
 * MODULE:     v.net.components
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Computes strongly and weakly connected components
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

int insert_new_record(dbDriver * driver, struct field_info *Fi,
		      dbString * sql, int cat, int comp)
{
    char buf[2000];

    sprintf(buf, "insert into %s values (%d, %d)", Fi->table, cat, comp);
    db_set_string(sql, buf);
    G_debug(3, db_get_string(sql));

    if (db_execute_immediate(driver, sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Cannot insert new record: %s"), db_get_string(sql));
	return 0;
    };
    return 1;
}

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *method_opt, *afield_opt, *nfield_opt, *abcol,
                  *afcol, *ncol;
    struct Flag *add_f;
    int with_z;
    int afield, nfield, mask_type;
    dglGraph_s *graph;
    int *component, nnodes, type, i, nlines, components, j, max_cat;
    char buf[2000], *covered;
    char *desc;

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
    G_add_keyword(_("components"));
    module->description =
	_("Computes strongly and weakly connected components in the network.");

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

    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->multiple = NO;
    method_opt->options = "weak,strong";
    desc = NULL;
    G_asprintf(&desc,
	       "weak;%s;strong;%s",
	       _("Weakly connected components"),
	       _("Strongly connected components"));
    method_opt->descriptions = desc;
    method_opt->description = _("Type of components");

    add_f = G_define_flag();
    add_f->key = 'a';
    add_f->description = _("Add points on nodes");

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
    nnodes = Vect_get_num_nodes(&In);
    component = (int *)G_calloc(nnodes + 1, sizeof(int));
    covered = (char *)G_calloc(nnodes + 1, sizeof(char));
    if (!component || !covered) {
	G_fatal_error(_("Out of memory"));
	exit(EXIT_FAILURE);
    }
    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    sprintf(buf, "create table %s ( cat integer, comp integer)", Fi->table);

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

    if (method_opt->answer[0] == 'w')
	components = NetA_weakly_connected_components(graph, component);
    else
	components = NetA_strongly_connected_components(graph, component);

    G_debug(3, "Components: %d", components);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    nlines = Vect_get_num_lines(&In);
    for (i = 1; i <= nlines; i++) {
	int comp, cat;

	type = Vect_read_line(&In, Points, Cats, i);
	if (!Vect_cat_get(Cats, afield, &cat))
	    continue;
	if (type == GV_LINE || type == GV_BOUNDARY) {
	    int node1, node2;

	    Vect_get_line_nodes(&In, i, &node1, &node2);
	    if (component[node1] == component[node2]) {
		comp = component[node1];
	    }
	    else {
		continue;
	    }
	}
	else if (type == GV_POINT) {
	    int node;

	    /* Vect_get_line_nodes(&In, i, &node, NULL); */
	    node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	    comp = component[node];
	    covered[node] = 1;
	}
	else
	    continue;
	Vect_write_line(&Out, type, Points, Cats);
	insert_new_record(driver, Fi, &sql, cat, comp);
	/*        for(j=0;j<Cats->n_cats;j++)
	 * if(Cats->field[j] == layer)
	 * insert_new_record(driver, Fi, &sql, Cats->cat[j], comp);
	 */
    };
    /*add points on nodes not covered by any point in the network */
    /*find the maximum cat number */
    if (add_f->answer) {
	max_cat = 0;
	for (i = 1; i <= nlines; i++) {
	    Vect_read_line(&In, NULL, Cats, i);
	    for (j = 0; j < Cats->n_cats; j++)
		if (Cats->cat[j] > max_cat)
		    max_cat = Cats->cat[j];
	}
	max_cat++;
	for (i = 1; i <= nnodes; i++)
	    if (!covered[i]) {
		Vect_reset_cats(Cats);
		Vect_cat_set(Cats, 1, max_cat);
		NetA_add_point_on_node(&In, &Out, i, Cats);
		insert_new_record(driver, Fi, &sql, max_cat++, component[i]);
	    }
    }

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
