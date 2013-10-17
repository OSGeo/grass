
/****************************************************************************
 *
 * MODULE:       v.reclass
 * AUTHOR(S):    R.L. Glenn, USDA, SCS, NHQ-CGIS (original contributor)
 *               GRASS 6 update: Radim Blazek <radim.blazek gmail.com>
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jachym Cepicky <jachym les-ejk.cz>, Markus Neteler <neteler itc.it>
 *               OGR support by Martin Landa <landa.martin gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#define KEY(x) (G_strcasecmp(key,x)==0)

int inpt(FILE * rulefd, char *buf);
int key_data(char *buf, char **k, char **d);
int reclass(struct Map_info *In, struct Map_info *Out, int type,
	    int field, dbCatValArray * cvarr, int optiond);

static int cmpcat(const void *pa, const void *pb)
{
    dbCatVal *p1 = (dbCatVal *) pa;
    dbCatVal *p2 = (dbCatVal *) pb;

    if (p1->cat < p2->cat)
	return -1;
    if (p1->cat > p2->cat)
	return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    struct GModule *module;

    /*    struct Flag *d_flag; */
    struct Option *in_opt, *out_opt, *type_opt, *field_opt, *rules_opt,
	*col_opt;
    char *key, *data, buf[1024];
    int rclelem, type, field;
    struct Map_info In, Out;

    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;

    int i, n, ttype;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("reclassification"));
    G_add_keyword(_("attributes"));
    module->description =
	_("Changes vector category values for an existing vector map "
	  "according to results of SQL queries or a value in attribute table column.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->guisection = _("Selection");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,boundary,centroid";
    type_opt->answer = "point,line,boundary,centroid";
    type_opt->guisection = _("Selection");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    col_opt->label =
	_("The name of the column whose values are to be used as new categories");
    col_opt->description = _("The source for the new key column must be type integer or string");
    
    rules_opt = G_define_standard_option(G_OPT_F_INPUT);
    rules_opt->key = "rules";
    rules_opt->required = NO;
    rules_opt->description = _("Full path to the reclass rule file");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    type = Vect_option_to_types(type_opt);

    if ((!(rules_opt->answer) && !(col_opt->answer)) ||
	(rules_opt->answer && col_opt->answer)) {
	G_fatal_error(_("Either '%s' or '%s' must be specified"),
		      rules_opt->key, col_opt->key);
    }

    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", field_opt->answer);
    field = Vect_get_field_number(&In, field_opt->answer);

    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Read column values from database */
    db_CatValArray_init(&cvarr);

    Fi = Vect_get_field(&In, field);
    if (Fi == NULL)
      G_fatal_error(_("Database connection not defined for layer %s"),
		    field_opt->answer);

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    if (col_opt->answer) {
	int ctype;
	int nrec;

	/* Check column type */
	ctype = db_column_Ctype(Driver, Fi->table, col_opt->answer);

	if (ctype == -1) {
	    G_fatal_error(_("Column <%s> not found in table <%s>"),
			  col_opt->answer, Fi->table);
	}
	else if (ctype == DB_C_TYPE_INT) {
	    nrec =
		db_select_CatValArray(Driver, Fi->table, Fi->key,
				      col_opt->answer, NULL, &cvarr);
	    G_debug(3, "nrec = %d", nrec);
	}
	else if (ctype == DB_C_TYPE_STRING) {
	    int i, more, nrows, ncols, newval, len;
	    dbString stmt, stmt2;
	    dbString lastval;
	    dbCursor cursor;
	    dbColumn *column;
	    dbValue *value;
	    dbTable *table;
	    struct field_info *NewFi;
	    dbDriver *Driver2;
	    int foundnull;

	    db_init_string(&stmt);
	    db_init_string(&stmt2);
	    db_init_string(&lastval);

	    NewFi = Vect_default_field_info(&Out, field, NULL, GV_1TABLE);
	    Vect_map_add_dblink(&Out, field, NULL, NewFi->table, GV_KEY_COLUMN,
				NewFi->database, NewFi->driver);

	    Driver2 = db_start_driver_open_database(NewFi->driver,
						    Vect_subst_var(NewFi->
								   database,
								   &Out));

	    /* get string column length */
	    db_set_string(&stmt, Fi->table);
	    if (db_describe_table(Driver, &stmt, &table) != DB_OK) {
		G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
	    }

	    column = NULL;

	    ncols = db_get_table_number_of_columns(table);
	    G_debug(3, "ncol = %d", ncols);

	    len = 0;
	    for (i = 0; i < ncols; i++) {
		column = db_get_table_column(table, i);
		if (G_strcasecmp(db_get_column_name(column), col_opt->answer) == 0) {
		    /* String column length */
		    len = db_get_column_length(column);
		    break;
		}
	    }
	    db_free_table(table);

	    /* Create table */
	    sprintf(buf, "create table %s (cat integer, %s varchar(%d))",
		    NewFi->table, col_opt->answer, len);

	    db_set_string(&stmt2, buf);

	    if (db_execute_immediate(Driver2, &stmt2) != DB_OK) {
		Vect_close(&Out);
		db_close_database_shutdown_driver(Driver);
		db_close_database_shutdown_driver(Driver2);
		G_fatal_error("Unable to create table: '%s'",
			      db_get_string(&stmt2));
	    }
	    db_begin_transaction(Driver2);

	    /* select values */
	    sprintf(buf, "SELECT %s, %s FROM %s ORDER BY %s", Fi->key,
		    col_opt->answer, Fi->table, col_opt->answer);
	    db_set_string(&stmt, buf);

	    G_debug(3, "  SQL: %s", db_get_string(&stmt));

	    if (db_open_select_cursor(Driver, &stmt, &cursor, DB_SEQUENTIAL)
		!= DB_OK)
		G_fatal_error("Unable to open select cursor: '%s'",
			      db_get_string(&stmt));

	    nrows = db_get_num_rows(&cursor);

	    G_debug(3, "  %d rows selected", nrows);
	    if (nrows <= 0)
		G_fatal_error(_("No records selected from table <%s>"),
			      Fi->table);

	    db_CatValArray_alloc(&cvarr, nrows);

	    table = db_get_cursor_table(&cursor);

	    /* Check if key column is integer */
	    column = db_get_table_column(table, 0);
	    ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	    G_debug(3, "  key type = %d", type);

	    if (ctype != DB_C_TYPE_INT) {
		G_fatal_error(_("Key column type is not integer"));	/* shouldnot happen */
	    }

	    cvarr.ctype = DB_C_TYPE_INT;

	    newval = 0;
	    foundnull = 0;

	    /* fetch the data */
	    for (i = 0; i < nrows; i++) {
		int isnull;

		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
		    G_fatal_error(_("Unable to fetch data from table <%s>"),
				  Fi->table);
		}

		column = db_get_table_column(table, 1);
		value = db_get_column_value(column);
		isnull = db_test_value_isnull(value);

		if (i == 0 || (!foundnull && isnull) ||
		    (!isnull && strcmp(db_get_value_string(value),
				       db_get_string(&lastval)) != 0)) {

		    if (!foundnull && isnull) {
		        foundnull = 1;
			db_set_value_string(value, "");
		    }

		    newval++;
		    db_set_string(&lastval, db_get_value_string(value));
		    G_debug(3, "  newval = %d string = %s", newval,
			    db_get_value_string(value));

		    db_set_string(&stmt2, db_get_value_string(value));
		    db_double_quote_string(&stmt2);
		    sprintf(buf, "insert into %s values (%d, '%s')",
			    NewFi->table, newval, db_get_string(&stmt2));

		    db_set_string(&stmt2, buf);

		    if (db_execute_immediate(Driver2, &stmt2) != DB_OK) {
			Vect_close(&Out);
			db_close_database_shutdown_driver(Driver);
			db_close_database_shutdown_driver(Driver2);
			G_fatal_error(_("Cannot insert data: [%s]"),
				      db_get_string(&stmt2));
		    }
		}

		column = db_get_table_column(table, 0);	/* first column */
		value = db_get_column_value(column);
		cvarr.value[i].cat = db_get_value_int(value);

		cvarr.value[i].val.i = newval;

		G_debug(4, "  cat = %d newval = %d", cvarr.value[i].cat,
			newval);
	    }

	    cvarr.n_values = nrows;

	    db_close_cursor(&cursor);
	    db_free_string(&stmt);
	    db_free_string(&lastval);

	    db_commit_transaction(Driver2);

	    if (db_create_index2(Driver2, NewFi->table, NewFi->key) != DB_OK)
		G_warning(_("Unable to create index for table <%s>, key <%s>"),
			  NewFi->table, NewFi->key);

	    if (db_grant_on_table(Driver2, NewFi->table, DB_PRIV_SELECT,
				  DB_GROUP | DB_PUBLIC) != DB_OK) {
		G_fatal_error(_("Unable to grant privileges on table <%s>"),
			      NewFi->table);
	    }
	    db_close_database_shutdown_driver(Driver2);

	    qsort((void *)cvarr.value, nrows, sizeof(dbCatVal), cmpcat);
	}
	else {
	    G_fatal_error(_("Column type must be integer or string"));
	}

    }
    else {
	int cat;
	char *label, *where;
	FILE *rulefd;

	G_debug(2, "Reading rules");

	if ((rulefd = fopen(rules_opt->answer, "r")) == NULL)
	    G_fatal_error(_("Unable to open rule file <%s>"),
			  rules_opt->answer);

	db_CatValArray_alloc(&cvarr, Vect_get_num_lines(&In));

	cat = 0;
	where = label = NULL;
	while (inpt(rulefd, buf)) {
	    if (!key_data(buf, &key, &data))
		continue;

	    G_strip(data);
	    G_debug(3, "key = %s data = %s", key, data);

	    if (KEY("cat")) {
		if (cat > 0)
		    G_fatal_error(_("Category %d overwritten by '%s'"), cat,
				  data);
		cat = atoi(data);
		if (cat <= 0)
		    G_fatal_error(_("Category '%s' invalid"), data);
	    }
	    else if (KEY("label")) {
		if (label)
		    G_fatal_error(_("Label '%s' overwritten by '%s'"), label,
				  data);
		label = G_store(data);
	    }
	    else if (KEY("where")) {
		if (where)
		    G_fatal_error(_("Condition '%s' overwritten by '%s'"),
				  where, data);
		where = G_store(data);
	    }
	    else {
		G_fatal_error(_("Unknown rule option: '%s'"), key);
	    }

	    if (cat > 0 && where) {
		int i, over, *cats, ncats;
		dbCatVal *catval;

		G_debug(2, "cat = %d, where = '%s'", cat, where);
		if (!label)
		    label = where;

		ncats =
		    db_select_int(Driver, Fi->table, Fi->key, where, &cats);
		if (ncats == -1)
		    G_fatal_error(_("Cannot select values from database"));
		G_debug(3, "  ncats = %d", ncats);


		/* If the category already exists, overwrite it cvarr, set to 0 in cats
		 * and don't add second time */
		over = 0;
		for (i = 0; i < ncats; i++) {
		    if (db_CatValArray_get_value(&cvarr, cats[i], &catval) ==
			DB_OK) {
			catval->val.i = cat;
			cats[i] = 0;
			over++;
		    }
		}
		if (over > 0)
		    G_warning(_("%d previously set categories overwritten by new category %d"),
			      over, cat);

		for (i = 0; i < ncats; i++) {
		    if (cats[i] <= 0)
			continue;

		    if (cvarr.n_values == cvarr.alloc) {
			db_CatValArray_realloc(&cvarr,
					       (int)10 + cvarr.alloc / 3);
		    }
		    G_debug(3, "Add old cat %d", cats[i]);
		    cvarr.value[cvarr.n_values].cat = cats[i];
		    cvarr.value[cvarr.n_values].val.i = cat;
		    cvarr.n_values++;
		}

		db_CatValArray_sort(&cvarr);

		G_free(cats);
		cat = 0;
		where = label = NULL;
	    }
	}

	if (cat > 0 || where)
	    G_fatal_error(_("Incomplete rule"));
    }

    db_close_database_shutdown_driver(Driver);

    /* reclass vector map */
    rclelem = reclass(&In, &Out, type, field, &cvarr, 0);

    /* Copy tables */
    n = Vect_get_num_dblinks(&In);
    for (i = 0; i < Vect_get_num_dblinks(&In); i++) {
	Fi = Vect_get_dblink(&In, i);
	if (Fi->number == field) {
	    n--;
	    break;
	}
    }
    if (n > 1)
	ttype = GV_MTABLE;
    else
	ttype = GV_1TABLE;

    for (i = 0; i < Vect_get_num_dblinks(&In); i++) {
	Fi = Vect_get_dblink(&In, i);
	if (Fi->number == field) {
	    continue;
	}

	Vect_copy_table(&In, &Out, Fi->number, Fi->number, Fi->name, ttype);
    }

    Vect_close(&In);

    Vect_build(&Out);
    Vect_close(&Out);

    G_done_msg(_("%d features reclassed."), rclelem);

    exit(EXIT_SUCCESS);
}
