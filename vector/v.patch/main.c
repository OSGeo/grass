
/****************************************************************************
 *
 * MODULE:       v.patch
 * AUTHOR(S):    Dave Gerdes, U.S.Army Construction Engineering Research Laboratory
 *               (original contributor)
 *               Radim Blazek <radim.blazek gmail.com> (update to GRASS 6)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>,
 *               Martin Landa <landa.martin gmail.com> (bbox)
 * PURPOSE:      
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 **  v.patch  input=file1,file2,.... output=composite
 **
 **   patch 2 or more vector maps together creating composite
 **
 **
 **  no checking is done for overlapping lines.
 **  header information will have to be editted afterwards.
 */

/*
 **  Written by Dave Gerdes  8/1988, US Army Construction Engineering Research Lab
 **  Upgrade to 5.7 Radim Blazek
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int patch(struct Map_info *, struct Map_info *, int, int *,
	  struct Map_info *);
int copy_records(dbDriver * driver_in, dbString * table_name_in,
		 dbDriver * driver_out, dbString * table_name_out,
		 char *, int, int);
int max_cat(struct Map_info *Map, int layer);

int main(int argc, char *argv[])
{
    int i, ret;
    char *in_name, *out_name, *meta_name, *bbox_name;
    struct GModule *module;
    struct Option *old, *new, *bbox;
    struct Flag *append, *table_flag, *no_topo;
    struct Flag *no_input_topo_flag, *force_z_flag;
    struct Map_info InMap, OutMap, BBoxMap;
    int n_files;
    int do_table;
    struct field_info *fi_in, *fi_out;
    dbString sql, table_name_in, table_name_out;
    dbDriver *driver_in, *driver_out;
    dbTable *table_in, *table_out;
    char *key = NULL;
    int keycol = -1;
    int maxcat = 0;
    int out_is_3d = WITHOUT_Z;
    char colnames[4096];
    double snap = -1;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("level1"));

    module->description = _("Creates a new vector map "
			    "by combining other vector maps.");

    old = G_define_standard_option(G_OPT_V_INPUTS);

    new = G_define_standard_option(G_OPT_V_OUTPUT);

    bbox = G_define_standard_option(G_OPT_V_OUTPUT);
    bbox->required = NO;
    bbox->key = "bbox";
    bbox->description =
	_("Name for output vector map where bounding boxes of input vector maps are written to");

    no_input_topo_flag = G_define_flag();
    no_input_topo_flag->key = 'n';
    no_input_topo_flag->label = _("Do not expect input with topology");
    no_input_topo_flag->description =
        _("Applicable when input is points without topology");

    force_z_flag = G_define_flag();
    force_z_flag->key = 'z';
    force_z_flag->label = _("Expect z coordinate even when not using topology");
    force_z_flag->description =
        _("Applicable when input is points with z coordinate but without topology");

    table_flag = G_define_flag();
    table_flag->key = 'e';
    table_flag->label = _("Copy also attribute table");
    table_flag->description =
	_("Only the table of layer 1 is currently supported");

    append = G_define_flag();
    append->key = 'a';
    append->description = _("Append files to existing file "
			    "(overwriting existing files must be activated)");

    no_topo = G_define_standard_flag(G_FLG_V_TOPO);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    out_name = new->answer;
    bbox_name = bbox->answer;
    do_table = table_flag->answer;

    db_init_string(&table_name_in);
    db_init_string(&table_name_out);
    db_init_string(&sql);

    i = 0;
    while (old->answers[i]) {
	in_name = old->answers[i++];
	Vect_check_input_output_name(in_name, new->answer, G_FATAL_EXIT);

	Vect_set_open_level(no_input_topo_flag->answer ? 1 : 2);
	if (Vect_open_old_head(&InMap, in_name, "") < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"), in_name);

	if (out_is_3d != WITH_Z && Vect_is_3d(&InMap))
	    out_is_3d = WITH_Z;

	Vect_close(&InMap);
    }
    if (force_z_flag->answer)
        out_is_3d = WITH_Z;

    table_out = NULL;
    fi_in = NULL;
    fi_out = NULL;
    *colnames = '\0';
    /* Check input table structures */
    if (do_table) {
	if (append->answer) {
	    Vect_set_open_level(1);
	    if (Vect_open_old_head(&OutMap, out_name, G_mapset()) < 0)
		G_fatal_error(_("Unable to open vector map <%s>"), out_name);

	    fi_out = Vect_get_field(&OutMap, 1);
	    if (fi_out) {
		key = G_store(fi_out->key);
		driver_out =
		    db_start_driver_open_database(fi_out->driver,
						  fi_out->database);
		if (!driver_out) {
		    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				  fi_out->database, fi_out->driver);
		}
                db_set_error_handler_driver(driver_out);

		db_set_string(&table_name_out, fi_out->table);
		if (db_describe_table(driver_out, &table_name_out, &table_out)
		    != DB_OK) {
		    G_fatal_error(_("Unable to describe table <%s>"),
				  fi_out->table);
		}
		db_close_database_shutdown_driver(driver_out);
	    }
	    Vect_close(&OutMap);
	}

	/* Get map name the table columns are derived from */
	if (append->answer)
	    meta_name = out_name;
	else
	    meta_name = old->answers[0];

	i = 0;
	while (old->answers[i]) {
	    in_name = old->answers[i];
	    Vect_set_open_level(1);
	    if (Vect_open_old_head(&InMap, in_name, "") < 0)
		G_fatal_error(_("Unable to open vector map <%s>"), in_name);

	    fi_in = Vect_get_field(&InMap, 1);
	    table_in = NULL;
	    if (fi_in) {
		dbTable **table;

		driver_in =
		    db_start_driver_open_database(fi_in->driver,
						  fi_in->database);
		if (!driver_in) {
		    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				  fi_in->database, fi_in->driver);
		}
                db_set_error_handler_driver(driver_in);

		if (!append->answer && i == 0) {
		    table = &table_out;
		    key = G_store(fi_in->key);
		}
		else {
		    table = &table_in;
		}

		db_set_string(&table_name_in, fi_in->table);
		if (db_describe_table(driver_in, &table_name_in, table)
		    != DB_OK) {
		    G_fatal_error(_("Unable to describe table <%s>"),
				  fi_in->table);
		}
		db_close_database_shutdown_driver(driver_in);
	    }

	    /* Get the output table structure */
	    if (i == 0 ) {
		int ncols, col;

		ncols = db_get_table_number_of_columns(table_out);

		for (col = 0; col < ncols; col++) {
		    dbColumn *column_out;

		    column_out = db_get_table_column(table_out, col);
		    if (col == 0)
			strcpy(colnames, db_get_column_name(column_out));
		    else {
			char tmpbuf[4096];

			sprintf(tmpbuf, ",%s", db_get_column_name(column_out));
			strcat(colnames, tmpbuf);
		    }
		}
	    }

	    /* Check the table structure */
	    if (i > 0 || append->answer) {
		int ncols, col;

		if (!table_in ||
		    (table_out && !table_in) || (!table_out && table_in)) {
		    G_fatal_error(_("Missing table for <%s>"), in_name);
		}

		if (G_strcasecmp(fi_in->key, key) != 0) {
		    G_fatal_error(
			_("Key (category) column names differ:"
			  " <%s> from <%s> and <%s> from <%s>"),
			fi_in->key, in_name,
			key, meta_name);
		}

		ncols = db_get_table_number_of_columns(table_out);

		if (ncols != db_get_table_number_of_columns(table_in)) {
		    G_fatal_error(
			_("Number of columns differ:"
			  " %d in <%s> and %d in <%s>"),
			db_get_table_number_of_columns(table_in),
			in_name, ncols, meta_name);
		}

		for (col = 0; col < ncols; col++) {
		    int col2, colmatch;
		    dbColumn *column_out, *column_in;
		    int ctype_in, ctype_out;

		    column_out = db_get_table_column(table_out, col);
		    col2 = 0;
		    colmatch = -1;
		    column_in = NULL;
		    /* find column with same name */
		    while (colmatch < 0 && col2 < ncols) {
			column_in = db_get_table_column(table_in, col2);

			if (G_strcasecmp(db_get_column_name(column_in),
					 db_get_column_name(column_out)) == 0) {
			    colmatch = col2;
			}
			col2++;
		    }
		    if (colmatch < 0) {
			G_fatal_error(_("No column <%s> in input map <%s>"),
			              db_get_column_name(column_out),
				      in_name);
		    }

		    ctype_in =
			db_sqltype_to_Ctype(db_get_column_sqltype(column_in));
		    ctype_out =
			db_sqltype_to_Ctype(db_get_column_sqltype
					    (column_out));
		    if (ctype_in != ctype_out) {
			G_fatal_error(
			    _("Column types differ: "
			      " <%s> from <%s> is <%s> and"
			      " <%s> from <%s> is <%s>"),
			    db_get_column_name(column_in),
			    in_name,
			    db_sqltype_name(db_get_column_sqltype(column_in)),
			    db_get_column_name(column_out),
			    meta_name,
			    db_sqltype_name(db_get_column_sqltype(column_out)));
		    }
		    if (ctype_in == DB_C_TYPE_STRING &&
			db_get_column_length(column_in) !=
			db_get_column_length(column_out)) {
			G_fatal_error(_("Length of string columns differ"));
		    }
		    if (G_strcasecmp(key,
				     db_get_column_name(column_out)) == 0) {
			keycol = col;
		    }
		}
	    }

	    Vect_close(&InMap);
	    i++;
	}

	if (keycol == -1) {
	    G_fatal_error(_("Key column not found"));
	}
    }

    if (append->answer) {
	if (no_topo->answer)
	    Vect_set_open_level(1);

	if (Vect_open_update(&OutMap, out_name, G_mapset()) < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"), out_name);

	if (out_is_3d == WITH_Z && !Vect_is_3d(&OutMap)) {
	    G_warning(_("The output map is not 3D"));
	}
	maxcat = max_cat(&OutMap, 1);
    }
    else {
	if (Vect_open_new(&OutMap, out_name, out_is_3d) < 0)
	    G_fatal_error(_("Unable to create vector map <%s>"), out_name);
    }

    if (bbox_name) {
	if (Vect_open_new(&BBoxMap, bbox_name, out_is_3d) < 0)	/* TODO 3D */
	    G_fatal_error(_("Unable to create vector map <%s>"), bbox_name);
	Vect_hist_command(&BBoxMap);
    }

    Vect_hist_command(&OutMap);

    driver_out = NULL;
    if (do_table) {
	if (append->answer) {
	    fi_out = Vect_get_field(&OutMap, 1);
	}
	else {
	    fi_out = Vect_default_field_info(&OutMap, 1, NULL, GV_1TABLE);
	    fi_out->key = key;
	}
	if (fi_out) {
	    driver_out =
		db_start_driver_open_database(fi_out->driver,
					      Vect_subst_var(fi_out->database,
							     &OutMap));
	    if (!driver_out) {
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      fi_out->database, fi_out->driver);
	    }
            db_set_error_handler_driver(driver_out);

	    db_begin_transaction(driver_out);
	}

	db_set_string(&table_name_out, fi_out->table);
	db_set_table_name(table_out, fi_out->table);

	if (!append->answer) {
	    if (db_create_table(driver_out, table_out) != DB_OK) {
		G_fatal_error(_("Unable to create table <%s>"),
			      fi_out->table);
	    }

	    /* do not allow duplicate keys */
	    if (db_create_index2(driver_out, fi_out->table, fi_out->key) != DB_OK)
		G_warning(_("Unable to create index"));

	    if (db_grant_on_table
		(driver_out, fi_out->table, DB_PRIV_SELECT,
		 DB_GROUP | DB_PUBLIC) != DB_OK)
		G_fatal_error(_("Unable to grant privileges on table <%s>"),
			      fi_out->table);

	    Vect_map_add_dblink(&OutMap, 1, NULL, fi_out->table,
				fi_in->key, fi_out->database, fi_out->driver);

	    /* avoid Vect_subst_var() below */
	    fi_out = Vect_get_field(&OutMap, 1);
	}
    }

    i = 0;
    while (old->answers[i]) {
	int add_cat;

	in_name = old->answers[i++];
	G_important_message(_("Patching vector map <%s>..."), in_name);
	if (bbox_name)
	    Vect_set_open_level(2);	/* needed for Vect_map_box() */
	else
	    Vect_set_open_level(1);
	if (Vect_open_old(&InMap, in_name, "") < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"), in_name);

	/*first time around, copy first in head to out head */
	if (i == 1)
	    Vect_copy_head_data(&InMap, &OutMap);

	if (do_table) {
	    add_cat = maxcat + 1;
	}
	else {
	    add_cat = 0;
	}
	G_debug(2, "maxcat = %d add_cat = %d", maxcat, add_cat);

	ret =
	    patch(&InMap, &OutMap, add_cat, &maxcat,
		  bbox_name ? &BBoxMap : NULL);
	if (ret < 0)
	    G_warning(_("Error reading vector map <%s> - "
			"some data may not be correct"), in_name);

	if (do_table) {
	    fi_in = Vect_get_field(&InMap, 1);
	    if (fi_in) {

		/* SQLite does not like to have the same database opened twice */
		if (strcmp(fi_in->driver, fi_out->driver) == 0
		    && strcmp(fi_in->database, fi_out->database) == 0) {
		    G_debug(3, "Use the same driver");
		    driver_in = driver_out;
		}
		else {
		    driver_in =
			db_start_driver_open_database(fi_in->driver,
						      fi_in->database);
		    if (!driver_in) {
			G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
				      fi_in->database, fi_in->driver);
		    }
		    db_set_error_handler_driver(driver_in);
		}

		db_set_string(&table_name_in, fi_in->table);
		copy_records(driver_in, &table_name_in,
			     driver_out, &table_name_out,
			     colnames, keycol, add_cat);

		if (driver_in != driver_out)
		    db_close_database_shutdown_driver(driver_in);
	    }
	}

	Vect_close(&InMap);
    }
    n_files = i;

    if (driver_out) {
	db_commit_transaction(driver_out);
	db_close_database_shutdown_driver(driver_out);
    }

    Vect_set_map_name(&OutMap, "Output from v.patch");
    Vect_set_person(&OutMap, G_whoami());

    if (!no_topo->answer) {
	Vect_build_partial(&OutMap, GV_BUILD_BASE);

	if (Vect_get_num_primitives(&OutMap, GV_BOUNDARY) > 0) {
	    int nmodif;
	    struct bound_box box;
	    double xmax, ymax, min_snap, max_snap;
	    int exp;
	    char *separator = "-----------------------------------------------------";

	    Vect_get_map_box(&OutMap, &box);

	    if (fabs(box.E) > fabs(box.W))
		xmax = fabs(box.E);
	    else
		xmax = fabs(box.W);
	    if (fabs(box.N) > fabs(box.S))
		ymax = fabs(box.N);
	    else
		ymax = fabs(box.S);

	    if (xmax < ymax)
		xmax = ymax;

	    /* double precision ULP */
	    min_snap = frexp(xmax, &exp);
	    exp -= 52;
	    min_snap = ldexp(min_snap, exp);
	    /* human readable */
	    min_snap = log10(min_snap);
	    if (min_snap < 0)
		min_snap = (int)min_snap;
	    else
		min_snap = (int)min_snap + 1;

	    /* single precision ULP */
	    max_snap = frexp(xmax, &exp);
	    exp -= 23;
	    max_snap = ldexp(max_snap, exp);
	    /* human readable */
	    max_snap = log10(max_snap);
	    if (max_snap < 0)
		max_snap = (int)max_snap;
	    else
		max_snap = (int)max_snap + 1;

	    snap = (min_snap + max_snap) / 2 - 1.5;
	    snap = pow(10, snap);

	    if (snap >= 0) {
		G_message("%s", separator);
		G_message(_("Snapping boundaries (threshold = %.3e)..."), snap);
		Vect_snap_lines(&OutMap, GV_BOUNDARY, snap, NULL);
	    }

	    G_message("%s", separator);
	    G_message(_("Breaking polygons..."));
	    Vect_break_polygons(&OutMap, GV_BOUNDARY, NULL);

	    G_message("%s", separator);
	    G_message(_("Removing duplicates..."));
	    Vect_remove_duplicates(&OutMap, GV_BOUNDARY, NULL);

	    /* in non-pathological cases, the bulk of the cleaning is now done */

	    /* Vect_clean_small_angles_at_nodes() can change the geometry so that new intersections
	     * are created. We must call Vect_break_lines(), Vect_remove_duplicates()
	     * and Vect_clean_small_angles_at_nodes() until no more small angles are found */
	    do {
		G_message("%s", separator);
		G_message(_("Breaking boundaries..."));
		Vect_break_lines(&OutMap, GV_BOUNDARY, NULL);

		G_message("%s", separator);
		G_message(_("Removing duplicates..."));
		Vect_remove_duplicates(&OutMap, GV_BOUNDARY, NULL);

		G_message("%s", separator);
		G_message(_("Cleaning boundaries at nodes..."));
		nmodif =
		    Vect_clean_small_angles_at_nodes(&OutMap, GV_BOUNDARY, NULL);
	    } while (nmodif > 0);

	    /* merge boundaries */
	    G_message("%s", separator);
	    G_message(_("Merging boundaries..."));
	    Vect_merge_lines(&OutMap, GV_BOUNDARY, NULL, NULL);

	    G_message("%s", separator);
	    G_message(_("Removing dangles..."));
	    Vect_remove_dangles(&OutMap, GV_BOUNDARY, -1.0, NULL);

	    G_message("%s", separator);
	    Vect_build_partial(&OutMap, GV_BUILD_ALL);

	    G_message(_("Removing bridges..."));
	    Vect_remove_bridges(&OutMap, NULL, &nmodif, NULL);

	    /* Boundaries are hopefully clean, build areas */
	    G_message("%s", separator);
	}

	Vect_build_partial(&OutMap, GV_BUILD_NONE);
	Vect_build(&OutMap);
    }
    Vect_close(&OutMap);

    if (bbox_name) {
	Vect_set_map_name(&BBoxMap, "Output from v.patch (bounding boxes)");
	Vect_set_person(&BBoxMap, G_whoami());
	G_important_message(" ");
	G_important_message(_("Building topology for vector map <%s>..."),
			    bbox_name);
	Vect_build(&BBoxMap);
	Vect_close(&BBoxMap);
    }

    G_message(_("Intersections at borders will have to be snapped"));
    G_message(_("Lines common between files will have to be edited"));
    G_message(_("The header information also may have to be edited"));

    G_done_msg(_("%d vector maps patched"), n_files);

    exit(EXIT_SUCCESS);
}


int copy_records(dbDriver * driver_in, dbString * table_name_in,
		 dbDriver * driver_out, dbString * table_name_out,
		 char *colnames, int keycol, int add_cat)
{
    int ncols, col;
    dbCursor cursor;
    dbString value_str, sql;
    dbTable *table_in;
    char tmpbuf[4096];

    db_init_string(&value_str);
    db_init_string(&sql);

    if (colnames && *colnames)
	sprintf(tmpbuf, "select %s from ", colnames);
    else
	sprintf(tmpbuf, "select * from ");
    db_set_string(&sql, tmpbuf);
    db_append_string(&sql, db_get_string(table_name_in));

    if (db_open_select_cursor(driver_in, &sql, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_warning(_("Cannot open select cursor: '%s'"), db_get_string(&sql));
	return 0;
    }
    table_in = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table_in);

    while (1) {
	int more;
	char buf[DB_SQL_MAX];

	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
	    db_close_cursor(&cursor);
	    G_fatal_error(_("Cannot fetch row"));
	}
	if (!more)
	    break;

	sprintf(buf, "insert into %s values ( ",
		db_get_string(table_name_out));
	db_set_string(&sql, buf);

	for (col = 0; col < ncols; col++) {
	    int ctype, sqltype;
	    dbColumn *column;
	    dbValue *value;

	    column = db_get_table_column(table_in, col);

	    sqltype = db_get_column_sqltype(column);
	    ctype = db_sqltype_to_Ctype(sqltype);
	    value = db_get_column_value(column);

	    if (col > 0)
		db_append_string(&sql, ", ");

	    if (col == keycol) {
		db_set_value_int(value, db_get_value_int(value) + add_cat);
	    }
	    db_convert_value_to_string(value, sqltype, &value_str);

	    switch (ctype) {
	    case DB_C_TYPE_STRING:
	    case DB_C_TYPE_DATETIME:
		if (db_test_value_isnull(value)) {
		    db_append_string(&sql, "null");
		}
		else {
		    db_double_quote_string(&value_str);
		    sprintf(buf, "'%s'", db_get_string(&value_str));
		    db_append_string(&sql, buf);
		}
		break;
	    case DB_C_TYPE_INT:
	    case DB_C_TYPE_DOUBLE:
		if (db_test_value_isnull(value)) {
		    db_append_string(&sql, "null");
		}
		else {
		    db_append_string(&sql, db_get_string(&value_str));
		}
		break;
	    default:
		G_fatal_error(_("Unknown column type"));
	    }
	}
	db_append_string(&sql, ")");

	G_debug(2, "SQL: %s", db_get_string(&sql));

	if (db_execute_immediate(driver_out, &sql) != DB_OK) {
	    G_fatal_error(_("Cannot insert new record: '%s'"),
			  db_get_string(&sql));
	}
    }

    db_close_cursor(&cursor);

    return 1;
}

int patch(struct Map_info *InMap, struct Map_info *OutMap, int add_cat,
	  int *max_cat, struct Map_info *BBoxMap)
{
    int type;
    struct line_pnts *Points;
    struct line_cats *Cats;

    *max_cat = add_cat;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* TODO:    
       OutMap->head.orig_scale = GREATER (OutMap->head.orig_scale, InMap->head.orig_scale);
       OutMap->head.digit_thresh = 0;
       OutMap->head.map_thresh = GREATER (OutMap->head.map_thresh, InMap->head.map_thresh);
     */

    while ((type = Vect_read_next_line(InMap, Points, Cats)) > 0) {
	int i;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == 1) {
		Cats->cat[i] += add_cat;
		if (Cats->cat[i] > *max_cat)
		    *max_cat = Cats->cat[i];
	    }
	}

	Vect_write_line(OutMap, type, Points, Cats);
    }

    if (BBoxMap) {		/* inspired by v.in.region */
	struct bound_box box;
	double diff_long, mid_long;
	static int cat;

	Vect_get_map_box(InMap, &box);

	diff_long = box.E - box.W;
	mid_long = (box.W + box.E) / 2;

	/* rectangle */
	Vect_reset_cats(Cats);

	/* write each line, useful for snapping */
	Vect_reset_line(Points);
	Vect_append_point(Points, box.W, box.S, 0.0);
	if (Vect_get_proj(BBoxMap) == PROJECTION_LL && diff_long >= 179) {
	    Vect_append_point(Points, mid_long, box.S, 0.0);
	}
	Vect_append_point(Points, box.E, box.S, 0.0);
	Vect_write_line(BBoxMap, GV_BOUNDARY, Points, Cats);

	Vect_reset_line(Points);
	Vect_append_point(Points, box.E, box.S, 0.0);
	Vect_append_point(Points, box.E, box.N, 0.0);
	Vect_write_line(BBoxMap, GV_BOUNDARY, Points, Cats);

	Vect_reset_line(Points);
	Vect_append_point(Points, box.E, box.N, 0.0);
	if (Vect_get_proj(BBoxMap) == PROJECTION_LL && diff_long >= 179) {
	    Vect_append_point(Points, mid_long, box.N, 0.0);
	}
	Vect_append_point(Points, box.W, box.N, 0.0);
	Vect_write_line(BBoxMap, GV_BOUNDARY, Points, Cats);

	Vect_reset_line(Points);
	Vect_append_point(Points, box.W, box.N, 0.0);
	Vect_append_point(Points, box.W, box.S, 0.0);
	Vect_write_line(BBoxMap, GV_BOUNDARY, Points, Cats);

	/* centroid */
	Vect_reset_line(Points);
	Vect_cat_set(Cats, 1, ++cat);	/* first layer */
	Vect_append_point(Points, (box.W + box.E) / 2, (box.S + box.N) / 2,
			  0.0);

	Vect_write_line(BBoxMap, GV_CENTROID, Points, Cats);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    if (type != -2)
	return -1;

    return 0;
}

int max_cat(struct Map_info *Map, int layer)
{
    struct line_cats *Cats;
    int max = 0;

    Cats = Vect_new_cats_struct();

    while (Vect_read_next_line(Map, NULL, Cats) > 0) {
	int i;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == layer && Cats->cat[i] > max) {
		max = Cats->cat[i];
	    }
	}
    }
    return max;
}
