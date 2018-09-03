#include <stdio.h>
#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

/* Query database for qfield. */

int query(struct Map_info *Map)
{
    int i, j, idx, cat_no, nlines, type;
    register int line_num;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct field_info *Fi;
    dbString stmt, value_string;
    dbDriver *driver;

    /* Initialize the Point struct */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    G_message(_("Reading features..."));

    /* Cycle through all lines and make a list of categories of 'qfield' for each category given by 'field' */
    nlines = Vect_get_num_lines(Map);
    for (line_num = 1; line_num <= nlines; line_num++) {
	type = Vect_read_line(Map, Points, Cats, line_num);
	if (!(type & options.type))
	    continue;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == options.field) {

		cat_no = Cats->cat[i];

		idx = find_cat(cat_no, 1);

		for (j = 0; j < Cats->n_cats; j++) {
		    if (Cats->field[j] == options.qfield) {	/* Add to list */
			if (Values[idx].nqcats == Values[idx].aqcats) {	/* Alloc space */
			    Values[idx].aqcats += 2;
			    Values[idx].qcat =
				(int *)G_realloc(Values[idx].qcat,
						 Values[idx].aqcats *
						 sizeof(int));
			}
			Values[idx].qcat[Values[idx].nqcats] = Cats->cat[j];
			Values[idx].nqcats++;
		    }
		}
	    }
	}

	/* If there is no field cat add cat -1, values for cat -1 are reported at the end  */
	Vect_cat_get(Cats, options.field, &cat_no);

	if (cat_no == -1) {
	    idx = find_cat(cat_no, 1);

	    for (j = 0; j < Cats->n_cats; j++) {
		if (Cats->field[j] == options.qfield) {	/* Add to list */
		    if (Values[idx].nqcats == Values[idx].aqcats) {	/* Alloc space */
			Values[idx].aqcats += 2;
			Values[idx].qcat =
			    (int *)G_realloc(Values[idx].qcat,
					     Values[idx].aqcats *
					     sizeof(int));
		    }
		    Values[idx].qcat[Values[idx].nqcats] = Cats->cat[j];
		    Values[idx].nqcats++;
		}
	    }
	}

	G_percent(line_num, nlines, 2);
    }

    db_init_string(&stmt);
    db_init_string(&value_string);

    if ((Fi = Vect_get_field(Map, options.qfield)) == NULL)
	G_fatal_error(_("Database connection not defined for layer %d. Use v.db.connect first."),
		      options.qfield);

    /* Open driver */
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    /* Query the database for each category */
    G_message(_("Querying database... "));
    for (i = 0; i < vstat.rcat; i++) {
	int ctype, nrows, more;
	char buf[2000];
	dbCursor cursor;
	dbTable *table;
	dbColumn *column;
	dbValue *value;

	G_debug(3, "cat %d", Values[i].cat);
	G_percent(i + 1, vstat.rcat, 1);

	/* Skip if cat is zero and large number of query categories (many features without category).
	 * It would cause problems on server side and take long time. Postgres limit is 10000 */
	/* TODO: verify because no category is encoded as cat = -1, not cat = zero */
	if (Values[i].cat == 0 && Values[i].nqcats > 1000) {
	    G_warning(_("Query for category '0' (no category) was not executed because of too many "
		       "(%d) query categories. All later reported values for cat 0 are not valid."),
		      Values[i].nqcats);
	    continue;
	}

	if (Values[i].nqcats > 0) {
	    sprintf(buf, "SELECT %s FROM %s WHERE", options.qcol, Fi->table);
	    db_set_string(&stmt, buf);

	    for (j = 0; j < Values[i].nqcats; j++) {
		G_debug(4, "  qcat %d", Values[i].qcat[j]);

		if (j > 0)
		    db_append_string(&stmt, " OR");

		sprintf(buf, " %s = %d", Fi->key, Values[i].qcat[j]);
		db_append_string(&stmt, buf);
	    }
	    G_debug(4, "  SQL: '%s'", db_get_string(&stmt));

	    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL)
		!= DB_OK)
		G_fatal_error("Cannot open cursor: '%s'",
			      db_get_string(&stmt));

	    table = db_get_cursor_table(&cursor);
	    column = db_get_table_column(table, 0);	/* first column */
	    value = db_get_column_value(column);
	    ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
	    vstat.qtype = ctype;
	    nrows = db_get_num_rows(&cursor);

	    G_debug(4, "  nrows = %d, columnt type = %d", nrows, ctype);

	    if (nrows != 1) {
		if (nrows > 1) {
		    G_warning(_("Multiple query results, output value set to NULL (category [%d])"),
			      Values[i].cat);
		}
		Values[i].null = 1;
	    }
	    else {
		if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
		    G_fatal_error(_("Unable to fetch record"));

		db_convert_column_value_to_string(column, &stmt);
		G_debug(4, "  value = %s", db_get_string(&stmt));

		if (db_test_value_isnull(value)) {
		    Values[i].null = 1;
		}
		else {
		    switch (ctype) {
		    case (DB_C_TYPE_INT):
			Values[i].i1 = db_get_value_int(value);
			break;
		    case (DB_C_TYPE_DOUBLE):
			Values[i].d1 = db_get_value_double(value);
			break;
		    case (DB_C_TYPE_STRING):
			Values[i].str1 = G_store(db_get_value_string(value));
			break;
		    case (DB_C_TYPE_DATETIME):
			db_convert_column_value_to_string(column,
							  &value_string);
			Values[i].str1 =
			    G_store(db_get_string(&value_string));
		    }
		    Values[i].null = 0;
		}
	    }
	    db_close_cursor(&cursor);
	}
	else {			/* no qcats -> upload NULL */
	    Values[i].null = 1;
	}
    }

    db_close_database_shutdown_driver(driver);

    return 0;
}
