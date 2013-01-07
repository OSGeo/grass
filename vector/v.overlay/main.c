
/****************************************************************************
 *
 * MODULE:       v.overlay
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>,
 *               Markus Neteler <neteler itc.it>,
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               Markus Metz
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local.h"

int main(int argc, char *argv[])
{
    int i, input, line, nlines, operator;
    int type[2], field[2], ofield[3];
    double snap_thresh;
    struct GModule *module;
    struct Option *in_opt[2], *out_opt, *type_opt[2], *field_opt[2],
	*ofield_opt, *operator_opt, *snap_opt;
    struct Flag *table_flag;
    struct Map_info In[2], Out, Tmp;
    char *tmpname;
    struct line_pnts *Points, *Points2;
    struct line_cats *Cats;
    struct ilist *BList;
    char *desc;
    int verbose;

    struct field_info *Fi = NULL;
    char buf[1000];
    dbString stmt;
    dbString sql, value_string, col_defs;
    dbDriver *driver;
    ATTRIBUTES attr[2];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("spatial query"));
    module->description = _("Overlays two vector maps.");

    in_opt[0] = G_define_standard_option(G_OPT_V_INPUT);
    in_opt[0]->label = _("Name of input vector map (A)");
    in_opt[0]->key = "ainput";

    field_opt[0] = G_define_standard_option(G_OPT_V_FIELD);
    field_opt[0]->label = _("Layer number or name (vector map A)");
    field_opt[0]->key = "alayer";

    type_opt[0] = G_define_standard_option(G_OPT_V_TYPE);
    type_opt[0]->label = _("Feature type (vector map A)");
    type_opt[0]->key = "atype";
    type_opt[0]->options = "line,area";
    type_opt[0]->answer = "area";

    in_opt[1] = G_define_standard_option(G_OPT_V_INPUT);
    in_opt[1]->label = _("Name of input vector map (B)");
    in_opt[1]->key = "binput";

    field_opt[1] = G_define_standard_option(G_OPT_V_FIELD);
    field_opt[1]->label = _("Layer number or name (vector map B)");
    field_opt[1]->key = "blayer";

    type_opt[1] = G_define_standard_option(G_OPT_V_TYPE);
    type_opt[1]->label = _("Feature type (vector map B)");
    type_opt[1]->key = "btype";
    type_opt[1]->options = "area";
    type_opt[1]->answer = "area";

    operator_opt = G_define_option();
    operator_opt->key = "operator";
    operator_opt->type = TYPE_STRING;
    operator_opt->required = YES;
    operator_opt->multiple = NO;
    operator_opt->options = "and,or,not,xor";
    operator_opt->label = _("Operator defines features written to "
			    "output vector map");
    operator_opt->description =
	_("Feature is written to output if the result "
	  "of operation 'ainput operator binput' is true. "
	  "Input feature is considered to be true, if "
	  "category of given layer is defined.");
    desc = NULL;
    G_asprintf(&desc,
	       "and;%s;or;%s;not;%s;xor;%s",
	       _("also known as 'intersection' in GIS"),
	       _("also known as 'union' in GIS (only for atype=area)"),
	       _("features from ainput not overlayed by features from binput"),
	       _("features from either ainput or binput but "
		 "not those from ainput overlayed by binput (only "
		 "for atype=area)"));
    operator_opt->descriptions = desc;

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    ofield_opt = G_define_standard_option(G_OPT_V_FIELD);
    ofield_opt->key = "olayer";
    ofield_opt->multiple = YES;
    ofield_opt->answer = "1,0,0";
    ofield_opt->label = _("Output layer for new category, ainput and binput");
    ofield_opt->description = _("If 0 or not given, "
				"the category is not written");
    ofield_opt->required = NO;
    ofield_opt->guisection = _("Attributes");
    
    snap_opt = G_define_option();
    snap_opt->key = "snap";
    snap_opt->label = _("Snapping threshold for boundaries");
    snap_opt->description = _("Disable snapping with snap <= 0");
    snap_opt->type = TYPE_DOUBLE;
    snap_opt->answer = "1e-8";

    table_flag = G_define_standard_flag(G_FLG_V_TABLE);
    table_flag->guisection = _("Attributes");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    for (input = 0; input < 2; input++) {
	type[input] = Vect_option_to_types(type_opt[input]);
    }
    if (type[0] & GV_AREA)
	type[0] = GV_AREA;

    ofield[0] = ofield[1] = ofield[2] = 0;
    i = 0;
    while (ofield_opt->answers[i] && i < 3) {
	ofield[i] = atoi(ofield_opt->answers[i]);
	i++;
    }

    if (operator_opt->answer[0] == 'a')
	operator = OP_AND;
    else if (operator_opt->answer[0] == 'o')
	operator = OP_OR;
    else if (operator_opt->answer[0] == 'n')
	operator = OP_NOT;
    else if (operator_opt->answer[0] == 'x')
	operator = OP_XOR;
    else
	G_fatal_error(_("Unknown operator '%s'"), operator_opt->answer);

    /* OP_OR, OP_XOR is not supported for lines,
       mostly because I'am not sure if they make enouhg sense */
    if (type[0] == GV_LINE && (operator == OP_OR || operator == OP_XOR))
	G_fatal_error(_("Operator '%s' is not supported for type line"),
		      operator_opt->answer);

    Vect_check_input_output_name(in_opt[0]->answer, out_opt->answer,
				 G_FATAL_EXIT);
    Vect_check_input_output_name(in_opt[1]->answer, out_opt->answer,
				 G_FATAL_EXIT);

    snap_thresh = atof(snap_opt->answer);

    Points = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* Open output */
    Vect_open_new(&Out, out_opt->answer, WITHOUT_Z);
    Vect_set_map_name(&Out, "Output from v.overlay");
    Vect_set_person(&Out, G_whoami());
    Vect_hist_command(&Out);
    
    G_asprintf(&tmpname, "%s_tmp_%d", out_opt->answer, getpid());
    Vect_open_new(&Tmp, tmpname, WITHOUT_Z);

    /* Create dblinks */
    if (ofield[0] > 0) {
	Fi = Vect_default_field_info(&Out, ofield[0], NULL, GV_1TABLE);
    }

    db_init_string(&sql);
    db_init_string(&value_string);
    db_init_string(&col_defs);

    /* Open database */
    if (ofield[0] > 0 && !(table_flag->answer)) {
	db_init_string(&stmt);
	driver =
	    db_start_driver_open_database(Fi->driver,
					  Vect_subst_var(Fi->database, &Out));
	if (driver == NULL) {
	    Vect_close(&Out);
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	}
	db_begin_transaction(driver);
    }
    else {
	driver = NULL;
    }

    /* Copy lines to output */
    BList = Vect_new_list();
    verbose = G_verbose();
    G_set_verbose(0);
    Vect_build_partial(&Tmp, GV_BUILD_BASE);
    G_set_verbose(verbose);
    for (input = 0; input < 2; input++) {
	int ncats, index, nlines_out, newline;

	Vect_set_open_level(2);
	Vect_open_old2(&(In[input]), in_opt[input]->answer, "", field_opt[input]->answer);
	field[input] = Vect_get_field_number(&(In[input]), field_opt[input]->answer);

	G_message(_("Copying vector features from <%s>..."),
		  Vect_get_full_name(&(In[input])));

	nlines = Vect_get_num_lines(&(In[input]));

	nlines_out = 0;
	for (line = 1; line <= nlines; line++) {
	    int ltype;
	    int vertices = 100; /* max number of vertices per line */

	    G_percent(line, nlines, 1);	/* must be before any continue */

	    ltype = Vect_read_line(&(In[input]), Points, Cats, line);

	    if (type[input] == GV_AREA) {
		if (!(ltype & GV_BOUNDARY))
		    continue;
	    }
	    else {		/* GV_LINE */
		if (!(ltype & type[input]))
		    continue;
	    }
	    
	    /* lines and boundaries must have at least 2 distinct vertices */
	    Vect_line_prune(Points);
	    if (Points->n_points < 2)
		continue;

	    /* TODO: figure out a reasonable threshold */
	    if (Points->n_points > vertices) {
		int start = 0;	/* number of coordinates written */
		
		vertices = Points->n_points / (Points->n_points / vertices + 1);
		
		/* split */
		while (start < Points->n_points - 1) {
		    int v = 0;

		    Vect_reset_line(Points2);
		    for (i = 0; i < vertices; i++) {
			v = start + i;
			if (v == Points->n_points)
			    break;

			Vect_append_point(Points2, Points->x[v], Points->y[v],
					  Points->z[v]);
		    }

		    newline = Vect_write_line(&Tmp, ltype, Points2, Cats);
		    if (input == 1)
			G_ilist_add(BList, newline);

		    start = v;
		}
	    }
	    else {
		newline = Vect_write_line(&Tmp, ltype, Points, Cats);
		if (input == 1)
		    G_ilist_add(BList, newline);
	    }
	    nlines_out++;
	}
	if (nlines_out == 0) {
	    Vect_close(&Tmp);
	    Vect_delete(tmpname);
	    Vect_close(&Out);
	    Vect_delete(out_opt->answer);
	    G_fatal_error(_("No %s features found in vector map <%s>. Verify '%s' parameter."),
		      type_opt[input]->answer, Vect_get_full_name(&(In[input])),
		      type_opt[input]->key);
	}
	
	/* Allocate attributes */
	attr[input].n = 0;
	/* this may be more than necessary */
	attr[input].attr =
	    (ATTR *)
	    G_calloc(Vect_cidx_get_type_count
		     (&(In[input]), field[input], type[input]), sizeof(ATTR));

	index = Vect_cidx_get_field_index(&(In[input]), field[input]);

	if (index >= 0) {
	    ncats = Vect_cidx_get_num_cats_by_index(&(In[input]), index);
	    for (i = 0; i < ncats; i++) {
		int cat, ctype, id;

		Vect_cidx_get_cat_by_index(&(In[input]), index, i, &cat,
					   &ctype, &id);
		if (!(ctype & type[input]))
		    continue;

		if (attr[input].n == 0 ||
		    cat != attr[input].attr[attr[input].n - 1].cat) {
		    attr[input].attr[attr[input].n].cat = cat;
		    attr[input].n++;
		}
	    }
	}

	G_debug(3, "%d cats read from index", attr[input].n);

	attr[input].null_values = NULL;
	attr[input].columns = NULL;

	/* Attributes */
	if (driver) {
	    int ncol, more;
	    struct field_info *inFi;
	    dbDriver *in_driver;
	    dbCursor cursor;
	    dbTable *Table;
	    dbColumn *Column;
	    dbValue *Value;
	    int sqltype, ctype;

	    G_verbose_message(_("Collecting input attributes..."));

	    inFi = Vect_get_field(&(In[input]), field[input]);
	    if (!inFi) {
		G_warning(_("Database connection not defined for layer %d"),
			  field[input]);
		continue;
	    }

	    in_driver =
		db_start_driver_open_database(inFi->driver, inFi->database);
	    if (in_driver == NULL) {
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      inFi->database, inFi->driver);
	    }

	    sprintf(buf, "select * from %s", inFi->table);
	    db_set_string(&sql, buf);

	    if (db_open_select_cursor(in_driver, &sql, &cursor, DB_SEQUENTIAL)
		!= DB_OK)
		G_fatal_error(_("Unable to select attributes"));

	    Table = db_get_cursor_table(&cursor);
	    ncol = db_get_table_number_of_columns(Table);
	    G_debug(3, "ncol = %d", ncol);

	    db_set_string(&sql, "");
	    db_set_string(&col_defs, "");
	    for (i = 0; i < ncol; i++) {
		db_append_string(&sql, ", null");

		Column = db_get_table_column(Table, i);
		sqltype = db_get_column_sqltype(Column);
		ctype = db_sqltype_to_Ctype(sqltype);

		if (input == 0)
		    db_append_string(&col_defs, ", a_");
		else
		    db_append_string(&col_defs, ", b_");

		db_append_string(&col_defs, db_get_column_name(Column));
		db_append_string(&col_defs, " ");
		switch (sqltype) {
		case DB_SQL_TYPE_CHARACTER:
		    sprintf(buf, "varchar(%d)", db_get_column_length(Column));
		    db_append_string(&col_defs, buf);
		    break;
		case DB_SQL_TYPE_TEXT:
		    db_append_string(&col_defs, "varchar(250)");
		    break;
		case DB_SQL_TYPE_SMALLINT:
		case DB_SQL_TYPE_INTEGER:
		    db_append_string(&col_defs, "integer");
		    break;
		case DB_SQL_TYPE_REAL:
		case DB_SQL_TYPE_DOUBLE_PRECISION:
		case DB_SQL_TYPE_DECIMAL:
		case DB_SQL_TYPE_NUMERIC:
		case DB_SQL_TYPE_INTERVAL:
		    db_append_string(&col_defs, "double precision");
		    break;
		case DB_SQL_TYPE_DATE:
		    db_append_string(&col_defs, "date");
		    break;
		case DB_SQL_TYPE_TIME:
		    db_append_string(&col_defs, "time");
		    break;
		case DB_SQL_TYPE_TIMESTAMP:
		    db_append_string(&col_defs, "datetime");
		    break;
		default:
		    G_warning(_("Unknown column type '%s' of column '%s'"),
			      db_sqltype_name(sqltype), db_get_column_name(Column));
		    sprintf(buf, "varchar(250)");
		}
	    }
	    attr[input].null_values = G_store(db_get_string(&sql));
	    attr[input].columns = G_store(db_get_string(&col_defs));

	    while (1) {
		int cat = -1;
		ATTR *at;

		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		    G_fatal_error(_("Unable to fetch data from table"));

		if (!more)
		    break;

		db_set_string(&sql, "");

		for (i = 0; i < ncol; i++) {

		    Column = db_get_table_column(Table, i);
		    sqltype = db_get_column_sqltype(Column);
		    ctype = db_sqltype_to_Ctype(sqltype);
		    Value = db_get_column_value(Column);


		    if (G_strcasecmp(db_get_column_name(Column), inFi->key) ==
			0) {
			cat = db_get_value_int(Value);
			G_debug(3, "cat = %d", cat);
		    }

		    db_append_string(&sql, ", ");

		    db_convert_value_to_string(Value, sqltype, &value_string);

		    G_debug(3, "%d: %s : %s", i, db_get_column_name(Column),
			    db_get_string(&value_string));

		    switch (ctype) {
		    case DB_C_TYPE_STRING:
		    case DB_C_TYPE_DATETIME:
			if (db_test_value_isnull(Value)) {
			    db_append_string(&sql, "null");
			}
			else {
			    db_double_quote_string(&value_string);
			    sprintf(buf, "'%s'",
				    db_get_string(&value_string));
			    db_append_string(&sql, buf);
			}
			break;
		    case DB_C_TYPE_INT:
		    case DB_C_TYPE_DOUBLE:
			if (db_test_value_isnull(Value)) {
			    db_append_string(&sql, "null");
			}
			else {
			    db_append_string(&sql,
					     db_get_string(&value_string));
			}
			break;
		    default:
			G_warning(_("Unknown column type '%s' of column '%s', values lost"),
			      db_sqltype_name(sqltype), db_get_column_name(Column));
			db_append_string(&sql, "null");
		    }
		}

		at = find_attr(&(attr[input]), cat);
		if (!at)
		    continue;

		/* if ( !at->used ) continue; *//* We don't know yet */

		at->values = G_store(db_get_string(&sql));
		G_debug(3, "values: %s", at->values);
	    }

	    db_table_to_sql(Table, &sql);

	    db_close_database_shutdown_driver(in_driver);
	}
    }

    if (driver) {
	sprintf(buf, "create table %s (cat integer ", Fi->table);
	db_set_string(&stmt, buf);

	if (attr[0].columns)
	    db_append_string(&stmt, attr[0].columns);
	else {
	    sprintf(buf, ", a_cat integer");
	    db_append_string(&stmt, buf);
	}

	if (attr[1].columns)
	    db_append_string(&stmt, attr[1].columns);
	else {
	    sprintf(buf, ", b_cat integer");
	    db_append_string(&stmt, buf);
	}

	db_append_string(&stmt, " )");

	G_debug(3, db_get_string(&stmt));

	if (db_execute_immediate(driver, &stmt) != DB_OK) {
	    Vect_close(&Out);
	    db_close_database_shutdown_driver(driver);
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&stmt));
	}
	
	if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	    G_warning(_("Unable to create index"));

	if (db_grant_on_table
	    (driver, Fi->table, DB_PRIV_SELECT,
	     DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);

	/* Table created, now we can write dblink */
	Vect_map_add_dblink(&Out, ofield[0], NULL, Fi->table, GV_KEY_COLUMN,
			    Fi->database, Fi->driver);
    }

    /* AREA x AREA */
    if (type[0] == GV_AREA) {
	area_area(In, field, &Tmp, &Out, Fi, driver, operator, ofield, attr, BList, snap_thresh);
    }
    else {			/* LINE x AREA */
	line_area(In, field, &Tmp, &Out, Fi, driver, operator, ofield, attr, BList);
    }
    
    Vect_close(&Tmp);
    Vect_delete(tmpname);

    /* Build topology to show the final result and prepare for Vect_close() */
    G_message(_("Building topology..."));
    Vect_build(&Out);

    if (driver) {
	/* Close table */
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }
    if (ofield[0] < 1 && !table_flag->answer) {
	int otype;
	
	if (type[0] == GV_AREA)
	    otype = GV_CENTROID;
	else
	    otype = GV_LINE;
	
	/* copy attributes from ainput */
	if (ofield[1] > 0 && field[0] > 0) {
	    Vect_copy_table(&In[0], &Out, field[0], ofield[1], NULL, otype);
	}
	/* copy attributes from binput */
	if (ofield[2] > 0 && field[1] > 0 && ofield[1] != ofield[2]) {
	    Vect_copy_table(&In[1], &Out, field[1], ofield[2], NULL, otype);
	}
    }

    Vect_close(&(In[0]));
    Vect_close(&(In[1]));
    Vect_close(&Out);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
