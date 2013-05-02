
/****************************************************************
 *
 * MODULE:     v.net.centrality
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    This module computes various centrality measures
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

/*Global variables */
struct Option *deg_opt, *close_opt, *betw_opt, *eigen_opt;
double *deg, *closeness, *betw, *eigen;

/* Attribute table */
dbString sql, tmp;
dbDriver *driver;
struct field_info *Fi;

void append_string(dbString * string, char *s)
{
    db_append_string(string, ", ");
    db_append_string(string, s);
    db_append_string(string, " double precision");
}

void append_double(dbString * string, double d)
{
    char buf[50];

    sprintf(buf, ",%f", d);
    db_append_string(string, buf);
}

void process_node(int node, int cat)
{
    char buf[2000];

    sprintf(buf, "INSERT INTO %s VALUES(%d", Fi->table, cat);
    db_set_string(&sql, buf);
    if (deg_opt->answer)
	append_double(&sql, deg[node]);
    if (close_opt->answer)
	append_double(&sql, closeness[node]);
    if (betw_opt->answer)
	append_double(&sql, betw[node]);
    if (eigen_opt->answer)
	append_double(&sql, eigen[node]);

    db_append_string(&sql, ")");
    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Cannot insert new record: %s"), db_get_string(&sql));
    }
}

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *cat_opt, *where_opt, *afield_opt, *nfield_opt, *abcol,
                  *afcol, *ncol;
    struct Option *iter_opt, *error_opt;
    struct Flag *geo_f, *add_f;
    int chcat, with_z;
    int afield, nfield, mask_type;
    struct varray *varray;
    dglGraph_s *graph;
    int i, geo, nnodes, nlines, j, max_cat;
    char buf[2000], *covered;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("centrality measures"));
    module->description =
	_("Computes degree, centrality, betweeness, closeness and eigenvector "
	 "centrality measures in the network.");

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

    cat_opt = G_define_standard_option(G_OPT_V_CATS);
    cat_opt->guisection = _("Selection");
    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

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

    deg_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    deg_opt->key = "degree";
    deg_opt->required = NO;
    deg_opt->description = _("Name of degree centrality column");
    deg_opt->guisection = _("Columns");

    close_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    close_opt->key = "closeness";
    close_opt->required = NO;
    close_opt->description = _("Name of closeness centrality column");
    close_opt->guisection = _("Columns");

    betw_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    betw_opt->key = "betweenness";
    betw_opt->required = NO;
    betw_opt->description = _("Name of betweenness centrality column");
    betw_opt->guisection = _("Columns");

    eigen_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    eigen_opt->key = "eigenvector";
    eigen_opt->required = NO;
    eigen_opt->description = _("Name of eigenvector centrality column");
    eigen_opt->guisection = _("Columns");

    iter_opt = G_define_option();
    iter_opt->key = "iterations";
    iter_opt->answer = "1000";
    iter_opt->type = TYPE_INTEGER;
    iter_opt->required = NO;
    iter_opt->description =
	_("Maximum number of iterations to compute eigenvector centrality");

    error_opt = G_define_option();
    error_opt->key = "error";
    error_opt->answer = "0.1";
    error_opt->type = TYPE_DOUBLE;
    error_opt->required = NO;
    error_opt->description =
	_("Cummulative error tolerance for eigenvector centrality");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

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


    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_warning(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

    /* parse filter option and select appropriate lines */
    afield = Vect_get_field_number(&In, afield_opt->answer);
    nfield = Vect_get_field_number(&In, nfield_opt->answer);

    if (where_opt->answer || cat_opt->answer) {
	chcat = (NetA_initialise_varray(&In, afield, GV_POINT,
	                                where_opt->answer,
					cat_opt->answer, &varray) > 0);
    }
    else
	chcat = 0;

    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    db_init_string(&tmp);
    if (deg_opt->answer)
	append_string(&tmp, deg_opt->answer);
    if (close_opt->answer)
	append_string(&tmp, close_opt->answer);
    if (betw_opt->answer)
	append_string(&tmp, betw_opt->answer);
    if (eigen_opt->answer)
	append_string(&tmp, eigen_opt->answer);
    sprintf(buf,
	    "create table %s(cat integer%s)", Fi->table, db_get_string(&tmp));

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

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    if (0 != Vect_net_build_graph(&In, mask_type, afield, nfield, afcol->answer,
                                  abcol->answer, ncol->answer, geo, 0))
        G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(&In));
    
    graph = Vect_net_get_graph(&In);
    nnodes = dglGet_NodeCount(graph);

    deg = closeness = betw = eigen = NULL;

    covered = (char *)G_calloc(nnodes + 1, sizeof(char));
    if (!covered)
	G_fatal_error(_("Out of memory"));

    if (deg_opt->answer) {
	deg = (double *)G_calloc(nnodes + 1, sizeof(double));
	if (!deg)
	    G_fatal_error(_("Out of memory"));
    }

    if (close_opt->answer) {
	closeness = (double *)G_calloc(nnodes + 1, sizeof(double));
	if (!closeness)
	    G_fatal_error(_("Out of memory"));
    }

    if (betw_opt->answer) {
	betw = (double *)G_calloc(nnodes + 1, sizeof(double));
	if (!betw)
	    G_fatal_error(_("Out of memory"));
    }

    if (eigen_opt->answer) {
	eigen = (double *)G_calloc(nnodes + 1, sizeof(double));
	if (!eigen)
	    G_fatal_error(_("Out of memory"));
    }


    if (deg_opt->answer) {
	G_message(_("Computing degree centrality measure"));
	NetA_degree_centrality(graph, deg);
    }
    if (betw_opt->answer || close_opt->answer) {
	G_message(_("Computing betweenness and/or closeness centrality measure"));
	NetA_betweenness_closeness(graph, betw, closeness);
	if (closeness)
	    for (i = 1; i <= nnodes; i++)
		closeness[i] /= (double)In.dgraph.cost_multip;
    }
    if (eigen_opt->answer) {
	G_message(_("Computing eigenvector centrality measure"));
	NetA_eigenvector_centrality(graph, atoi(iter_opt->answer),
				    atof(error_opt->answer), eigen);
    }


    nlines = Vect_get_num_lines(&In);
    G_message(_("Writing data into the table..."));
    G_percent_reset();
    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 1);
	int type = Vect_read_line(&In, Points, Cats, i);

	if (type == GV_POINT && (!chcat || varray->c[i])) {
	    int cat, node;

	    if (!Vect_cat_get(Cats, nfield, &cat))
		continue;
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 1, cat);
	    Vect_write_line(&Out, type, Points, Cats);
	    node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	    process_node(node, cat);
	    covered[node] = 1;
	}
    }

    if (add_f->answer && !chcat) {
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
		process_node(i, max_cat);
		max_cat++;
	    }

    }

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    G_free(covered);
    if (deg)
	G_free(deg);
    if (closeness)
	G_free(closeness);
    if (betw)
	G_free(betw);
    if (eigen)
	G_free(eigen);
    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
