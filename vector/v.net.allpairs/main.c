
/****************************************************************
 *
 * MODULE:     v.net.allpairs
 *
 * AUTHOR(S):  Daniel Bundala
 *             Markus Metz
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

int remove_duplicates(struct Map_info *);
int check_duplicate(const struct line_pnts *, const struct line_pnts *);

struct _spnode {
    int cat, node;
};

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points, *aPoints;
    struct line_cats *Cats;
    struct ilist *List;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *cat_opt, *afield_opt, *nfield_opt, *where_opt, *abcol,
                  *afcol, *ncol;
    struct Flag *geo_f;
    int afield, nfield;
    int chcat, with_z;
    int mask_type;
    struct varray *varray;
    struct _spnode *spnode;
    int i, j, k, geo, nnodes, line, nlines, cat;
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
    module->description =
	_("Computes the shortest path between all pairs of nodes in the network.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");
    afield_opt->guisection = _("Selection");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");
    nfield_opt->guisection = _("Selection");

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
    aPoints = Vect_new_line_struct();
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
	chcat = (NetA_initialise_varray(&In, nfield, GV_POINT,
	                                where_opt->answer,
					cat_opt->answer, &varray) > 0);
    }
    else
	chcat = 0;

    /* Create table */
    Fi = Vect_default_field_info(&Out, afield, NULL, GV_MTABLE);
    Vect_map_add_dblink(&Out, afield, NULL, Fi->table, GV_KEY_COLUMN,
                        Fi->database, Fi->driver);
    db_init_string(&sql);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    sprintf(buf,
	    "create table %s ( cat integer, from_cat integer, to_cat integer, cost double precision)",
	    Fi->table);

    db_set_string(&sql, buf);
    G_debug(2, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }
    if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK) {
	if (strcmp(Fi->driver, "dbf"))
	    G_warning(_("Cannot create index"));
    }
    if (db_grant_on_table
	(driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);


    Vect_net_build_graph(&In, mask_type, afield, nfield,
			 afcol->answer, abcol->answer, ncol->answer, geo, 0);

    nnodes = Vect_get_num_primitives(&In, GV_POINT);

    G_debug(1, "%d nodes", nnodes);
    spnode = (struct _spnode *)G_calloc(nnodes, sizeof(struct _spnode));

    if (!spnode)
	G_fatal_error(_("Out of memory"));

    for (i = 0; i < nnodes; i++) {
	spnode[i].cat = -1;
	spnode[i].node = -1;
    }

    G_message(_("Writing node points..."));
    nlines = Vect_get_num_lines(&In);
    nnodes = 0;
    for (i = 1; i <= nlines; i++) {
	int node;

	if (Vect_get_line_type(&In, i) != GV_POINT)
	    continue;
	
	Vect_read_line(&In, Points, Cats, i);

	node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	if (node) {
	    Vect_cat_get(Cats, nfield, &cat);
	    if (cat != -1) {
		if (!chcat || varray->c[i]) {
		    Vect_write_line(&Out, GV_POINT, Points, Cats);
		    spnode[nnodes].cat = cat;
		    spnode[nnodes].node = node;
		    nnodes++;
		}
	    }
	}
    }
    /* copy node table */
    if (Vect_get_field(&In, nfield))
	Vect_copy_table(&In, &Out, nfield, nfield, NULL, GV_POINT);

    G_message(_("Writing shortest paths..."));
    G_percent_reset();
    cat = 1;
    List = Vect_new_list();
    for (i = 0; i < nnodes; i++) {
	G_percent(i, nnodes, 1);

	for (j = 0; j < nnodes; j++) {
	    double cost;
	    int ret;
	    
	    if (i == j)
		continue;
	    
	    ret = Vect_net_shortest_path(&In, spnode[i].node,
	                                 spnode[j].node, List, &cost);
	    
	    if (ret == -1) {
		/* unreachable */
		continue;
	    }

	    sprintf(buf, "insert into %s values (%d, %d, %d, %f)",
		    Fi->table, cat, spnode[i].cat, spnode[j].cat, cost);
	    db_set_string(&sql, buf);
	    G_debug(3, db_get_string(&sql));

	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		db_close_database_shutdown_driver(driver);
		G_fatal_error(_("Cannot insert new record: %s"),
			      db_get_string(&sql));
	    }
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, afield, cat);
	    cat++;

	    Vect_reset_line(aPoints);

	    for (k = 0; k < List->n_values; k++) {
		line = List->value[k];
		Vect_read_line(&In, Points, NULL, abs(line));
		if (line > 0)
		    Vect_append_points(aPoints, Points,
				       GV_FORWARD);
		else
		    Vect_append_points(aPoints, Points,
				       GV_BACKWARD);
		aPoints->n_points--;
	    }
	    aPoints->n_points++;
	    
	    Vect_line_prune(aPoints);

	    Vect_write_line(&Out, GV_LINE, aPoints, Cats);

	}
    }
    G_percent(1, 1, 1);

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    Vect_close(&In);

    Vect_build_partial(&Out, GV_BUILD_BASE);
    Vect_break_lines(&Out, GV_LINE, NULL);
    remove_duplicates(&Out);
    Vect_build_partial(&Out, GV_BUILD_NONE);
    
    Vect_build(&Out);

    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}


int remove_duplicates(struct Map_info *Map)
{
    struct line_pnts *APoints, *BPoints;
    struct line_cats *ACats, *BCats;
    int i, c, atype, btype, aline, bline;
    int nlines, nacats_orig, npoints;
    struct bound_box ABox;
    struct boxlist *List;
    int ndupl, is_dupl;


    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    nlines = Vect_get_num_lines(Map);

    G_debug(1, "nlines =  %d", nlines);
    /* Go through all lines in vector, for each line select lines which
     * overlap with the first vertex of this line and check if a 
     * selected line is identical. If yes, remove the selected line.
     * If the line vertices are identical with those of any other line, 
     * merge categories and rewrite the current line.
     */

    ndupl = 0;

    for (aline = 1; aline <= nlines; aline++) {
	G_percent(aline, nlines, 1);
	if (!Vect_line_alive(Map, aline))
	    continue;

	atype = Vect_read_line(Map, APoints, ACats, aline);
	if (!(atype & GV_LINE))
	    continue;

	npoints = APoints->n_points;
	Vect_line_prune(APoints);
	
	if (npoints != APoints->n_points) {
	    Vect_rewrite_line(Map, aline, atype, APoints, ACats);
	    nlines = Vect_get_num_lines(Map);
	    continue;
	}

	/* select potential duplicates */
	ABox.E = ABox.W = APoints->x[0];
	ABox.N = ABox.S = APoints->y[0];
	ABox.T = ABox.B = APoints->z[0];
	Vect_select_lines_by_box(Map, &ABox, GV_LINE, List);
	G_debug(3, "  %d lines selected by box", List->n_values);
	
	is_dupl = 0;

	for (i = 0; i < List->n_values; i++) {
	    bline = List->id[i];
	    G_debug(3, "  j = %d bline = %d", i, bline);

	    /* compare aline and bline only once */
	    if (aline <= bline)
		continue;

	    btype = Vect_read_line(Map, BPoints, BCats, bline);

	    if (!(btype & GV_LINE))
		continue;

	    Vect_line_prune(BPoints);

	    /* check for duplicate */
	    if (!check_duplicate(APoints, BPoints))
		continue;

	    /* bline is identical to aline */
	    is_dupl = 1;

	    Vect_delete_line(Map, bline);

	    /* merge categories */
	    nacats_orig = ACats->n_cats;

	    for (c = 0; c < BCats->n_cats; c++)
		Vect_cat_set(ACats, BCats->field[c], BCats->cat[c]);

	    if (ACats->n_cats > nacats_orig) {
		G_debug(4, "cats merged: n_cats %d -> %d", nacats_orig,
			ACats->n_cats);
	    }

	    ndupl++;
	}
	if (is_dupl) {
	    Vect_rewrite_line(Map, aline, atype, APoints, ACats);
	    nlines = Vect_get_num_lines(Map);
	    G_debug(3, "nlines =  %d\n", nlines);
	}
    }
    G_verbose_message("Removed duplicates: %d", ndupl);
    
    return ndupl;
}

int check_duplicate(const struct line_pnts *APoints,
		    const struct line_pnts *BPoints)
{
    int k;
    int npoints;

    if (APoints->n_points != BPoints->n_points)
	return 0;

    npoints = APoints->n_points;

    /* only forward */
    for (k = 0; k < npoints; k++) {
	if (APoints->x[k] != BPoints->x[k] ||
	    APoints->y[k] != BPoints->y[k] ||
	    APoints->z[k] != BPoints->z[k]) {
	    return 0;
	}
    }

    return 1;
}
