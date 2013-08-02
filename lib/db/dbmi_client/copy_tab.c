/*!
  \file db/dbmi_client/copy_tab.c
  
  \brief DBMI Library (client) - copy table
  
  (C) 1999-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
*/

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "macros.h"

static int cmp(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;

    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

/*!
  \brief Copy table, used by various db_copy_table* (internal use only)
  
  Use either 'where' or 'select' or 'selcol'+'ivals'+'nvals' but
  never more than one.

  Warning: driver opened as second must be closed as first, otherwise
  it hangs, not sure why.

  \param from_dvrname name of driver from table is copied
  \param from_dbname name of database from table is copied
  \param from_tbl_name name of table to be copied
  \param to_dvrname name of driver to - where table is copied to
  \param to_dbname name of database to - where table is copied to
  \param to_dbname name of copied table
  \param where WHERE SQL condition (without where key word) or NULL
  \param select full select statement
  \param selcol name of column used to select records by values in ivals or NULL
  \param ivals pointer to array of integer values or NULL
  \param nvals number of values in ivals

  \return DB_OK on success
  \return DB_FAILED on failure
 */
static int copy_table(const char *from_drvname, const char *from_dbname,
		      const char *from_tblname, const char *to_drvname,
		      const char *to_dbname, const char *to_tblname,
		      const char *where, const char *select, const char *selcol,
		      int *ivals, int nvals)
{
    int col, ncols, sqltype, ctype, more, selcol_found;
    char buf[1000];
    int *ivalues;
    dbHandle from_handle, to_handle;
    dbString tblname, sql;
    dbString value_string;
    dbString *tblnames;
    dbTable *table, *out_table;
    dbCursor cursor;
    dbColumn *column;
    dbValue *value;
    const char *colname;
    dbDriver *from_driver, *to_driver;
    int count, i;

    G_debug(3, "db_copy_table():\n  from driver = %s, db = %s, table = %s\n"
	    "  to driver = %s, db = %s, table = %s, where = %s, select = %s",
	    from_drvname, from_dbname, from_tblname, to_drvname, to_dbname,
	    to_tblname, where, select);

    db_init_handle(&from_handle);
    db_init_handle(&to_handle);
    db_init_string(&tblname);
    db_init_string(&sql);
    db_init_string(&value_string);

    if (selcol) {
	if (!ivals || (ivals && nvals == 0)) {
	    G_warning(_("Array of values to select from column <%s> is empty"), selcol);
	    return DB_FAILED;
	}
	/* Make a copy of input values and sort it */
	if (ivals) {
	    ivalues = (int *)G_malloc(nvals * sizeof(int));
	    memcpy(ivalues, ivals, nvals * sizeof(int));
	    qsort((void *)ivalues, nvals, sizeof(int), cmp);
	}
    }
    else
	ivalues = NULL;

    /* Open input driver and database */
    from_driver = db_start_driver(from_drvname);
    if (from_driver == NULL) {
	G_warning(_("Unable to start driver <%s>"), from_drvname);
	return DB_FAILED;
    }
    db_set_handle(&from_handle, from_dbname, NULL);
    if (db_open_database(from_driver, &from_handle) != DB_OK) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  from_dbname, from_drvname);
	db_close_database_shutdown_driver(from_driver);
	return DB_FAILED;
    }

    /* Open output driver and database */
    if (strcmp(from_drvname, to_drvname) == 0
	&& strcmp(from_dbname, to_dbname) == 0) {
	G_debug(3, "Use the same driver");
	to_driver = from_driver;
    }
    else {
	to_driver = db_start_driver(to_drvname);
	if (to_driver == NULL) {
	    G_warning(_("Unable to start driver <%s>"), to_drvname);
	    db_close_database_shutdown_driver(from_driver);
	    return DB_FAILED;
	}
	db_set_handle(&to_handle, to_dbname, NULL);
	if (db_open_database(to_driver, &to_handle) != DB_OK) {
	    G_warning(_("Unable to open database <%s> by driver <%s>"),
		      to_dbname, to_drvname);
	    db_close_database_shutdown_driver(to_driver);
	    if (from_driver != to_driver) {
		db_close_database_shutdown_driver(from_driver);
	    }
	    return DB_FAILED;
	}
    }

    db_begin_transaction(to_driver);

    /* Because in SQLite3 an opened cursor is no more valid
       if 'schema' is modified (create table), we have to open
       cursor twice */

    /* test if the table exists */
    if (db_list_tables(to_driver, &tblnames, &count, 0) != DB_OK) {
	G_warning(_("Unable to get list tables in database <%s>"),
		  to_dbname);
	db_close_database_shutdown_driver(to_driver);
	if (from_driver != to_driver)
	    db_close_database_shutdown_driver(from_driver);

	return DB_FAILED;
    }

    for (i = 0; i < count; i++) {
	const char *tblname = db_get_string(&tblnames[i]);
	int ret;
	
	ret = DB_FAILED;
	if (strcmp(to_tblname, tblname) == 0) {
	    if (G_get_overwrite()) {
		G_warning(_("Table <%s> already exists in database and will be overwritten"),
			  to_tblname);
		ret = db_drop_table(from_driver, &tblnames[i]);
	    }
	    else {
		G_warning(_("Table <%s> already exists in database <%s>"),
			  to_tblname, to_dbname);
	    }
	    
	    if (ret != DB_OK) {
		db_close_database_shutdown_driver(to_driver);
		if (from_driver != to_driver)
		    db_close_database_shutdown_driver(from_driver);
		
		return DB_FAILED;
	    }
	}
    }

    /* Create new table */
    /* Open cursor for data structure */
    if (select) {
	db_set_string(&sql, select);

	/* TODO!: cannot use this because it will not work if a query 
	 *         ends with 'group by' for example */
	/*
	   tmp = strdup ( select );
	   G_tolcase ( tmp );

	   if ( !strstr( tmp,"where") )
	   {
	   db_append_string ( &sql, " where 0 = 1");
	   }
	   else
	   {
	   db_append_string ( &sql, " and 0 = 1");
	   }

	   free (tmp);
	 */
    }
    else {
	db_set_string(&sql, "select * from ");
	db_append_string(&sql, from_tblname);
	db_append_string(&sql, " where 0 = 1");	/* to get no data */
    }

    G_debug(3, "db__copy_table: %s", db_get_string(&sql));
    if (db_open_select_cursor(from_driver, &sql, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_warning(_("Unable to open select cursor: '%s'"),
		  db_get_string(&sql));
	db_close_database_shutdown_driver(to_driver);
	if (from_driver != to_driver) {
	    db_close_database_shutdown_driver(from_driver);
	}
	return DB_FAILED;
    }
    G_debug(3, "Select cursor opened");

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);
    G_debug(3, "ncols = %d", ncols);

    out_table = db_alloc_table(ncols);
    db_set_table_name(out_table, to_tblname);

    selcol_found = 0;
    for (col = 0; col < ncols; col++) {
	dbColumn *out_column;

	column = db_get_table_column(table, col);
	colname = db_get_column_name(column);
	sqltype = db_get_column_sqltype(column);
	ctype = db_sqltype_to_Ctype(sqltype);

	G_debug(3, "%s (%s)", colname, db_sqltype_name(sqltype));

	out_column = db_get_table_column(out_table, col);

	if (selcol && G_strcasecmp(colname, selcol) == 0) {
	    if (ctype != DB_C_TYPE_INT)
		G_fatal_error(_("Column <%s> is not integer"),
			      colname);
	    selcol_found = 1;
	}

	db_set_column_name(out_column, db_get_column_name(column));
	db_set_column_description(out_column,
				  db_get_column_description(column));
	db_set_column_sqltype(out_column, db_get_column_sqltype(column));
	db_set_column_length(out_column, db_get_column_length(column));
	db_set_column_precision(out_column, db_get_column_precision(column));
	db_set_column_scale(out_column, db_get_column_scale(column));
    }

    db_close_cursor(&cursor);

    if (selcol && !selcol_found)
	G_fatal_error(_("Column <%s> not found"), selcol);

    if (db_create_table(to_driver, out_table) != DB_OK) {
	G_warning(_("Unable to create table <%s>"),
		  to_tblname);
	db_close_database_shutdown_driver(to_driver);
	if (from_driver != to_driver) {
	    db_close_database_shutdown_driver(from_driver);
	}
	return DB_FAILED;
    }

    /* Open cursor with data */
    if (select) {
	db_set_string(&sql, select);
    }
    else {
	db_set_string(&sql, "select * from ");
	db_append_string(&sql, from_tblname);
	if (where) {
	    db_append_string(&sql, " where ");
	    db_append_string(&sql, where);
	}
    }

    G_debug(3, "db__copy_table: %s", db_get_string(&sql));
    if (db_open_select_cursor(from_driver, &sql, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_warning(_("Unable to open select cursor: '%s'"),
		  db_get_string(&sql));
	db_close_database_shutdown_driver(to_driver);
	if (from_driver != to_driver) {
	    db_close_database_shutdown_driver(from_driver);
	}
	return DB_FAILED;
    }
    G_debug(3, "Select cursor opened");

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);
    G_debug(3, "ncols = %d", ncols);

    /* Copy all rows */
    while (1) {
	int select;

	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
	    G_warning(_("Unable to fetch data from table <%s>"),
		      from_tblname);
	    db_close_cursor(&cursor);
	    db_close_database_shutdown_driver(to_driver);
	    if (from_driver != to_driver) {
		db_close_database_shutdown_driver(from_driver);
	    }
	    return DB_FAILED;
	}
	if (!more)
	    break;

	sprintf(buf, "insert into %s values ( ", to_tblname);
	db_set_string(&sql, buf);
	select = 1;
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    colname = db_get_column_name(column);
	    sqltype = db_get_column_sqltype(column);
	    ctype = db_sqltype_to_Ctype(sqltype);
	    value = db_get_column_value(column);

	    if (selcol && G_strcasecmp(colname, selcol) == 0) {
		if (db_test_value_isnull(value))
		    continue;
		if (!bsearch(&(value->i), ivalues, nvals, sizeof(int), cmp)) {
		    select = 0;
		    break;
		}
	    }
	    if (col > 0)
		db_append_string(&sql, ", ");
	    db_convert_value_to_string(value, sqltype, &value_string);
	    switch (ctype) {
	    case DB_C_TYPE_STRING:
	    case DB_C_TYPE_DATETIME:
		if (db_test_value_isnull(value)) {
		    db_append_string(&sql, "null");
		}
		else {
		    db_double_quote_string(&value_string);
		    db_append_string(&sql, "'");
		    db_append_string(&sql, db_get_string(&value_string));
		    db_append_string(&sql, "'");
		}
		break;
	    case DB_C_TYPE_INT:
	    case DB_C_TYPE_DOUBLE:
		if (db_test_value_isnull(value)) {
		    db_append_string(&sql, "null");
		}
		else {
		    db_append_string(&sql, db_get_string(&value_string));
		}
		break;
	    default:
		G_warning(_("Unknown column type (column <%s>)"),
			  colname);
		db_close_cursor(&cursor);
		db_close_database_shutdown_driver(to_driver);
		if (from_driver != to_driver) {
		    db_close_database_shutdown_driver(from_driver);
		}
		return DB_FAILED;
	    }
	}
	if (!select)
	    continue;
	db_append_string(&sql, ")");
	G_debug(3, "db__copy_table: %s", db_get_string(&sql));
	if (db_execute_immediate(to_driver, &sql) != DB_OK) {
	    G_warning("Unable to insert new record: '%s'",
		      db_get_string(&sql));
	    db_close_cursor(&cursor);
	    db_close_database_shutdown_driver(to_driver);
	    if (from_driver != to_driver) {
		db_close_database_shutdown_driver(from_driver);
	    }
	    return DB_FAILED;
	}
    }
    if (selcol)
	G_free(ivalues);
    G_debug(3, "Table copy OK");

    db_close_cursor(&cursor);
    db_commit_transaction(to_driver);
    db_close_database_shutdown_driver(to_driver);
    if (from_driver != to_driver) {
	db_close_database_shutdown_driver(from_driver);
    }

    return DB_OK;
}

/*!
  \brief Copy a table
 
  \param from_drvname name of driver from table is copied
  \param from_dbname name of database from table is copied
  \param from_tblname name of table to be copied
  \param to_drvname name of driver to - where table is copied to
  \param to_dbname name of database to - where table is copied to
  \param to_tblname name of copied table

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_copy_table(const char *from_drvname, const char *from_dbname,
		  const char *from_tblname, const char *to_drvname,
		  const char *to_dbname, const char *to_tblname)
{
    return copy_table(from_drvname, from_dbname, from_tblname,
		      to_drvname, to_dbname, to_tblname,
		      NULL, NULL, NULL, NULL, 0);
}

/*!
  \brief Copy a table (by where statement)

  \param from_drvname name of driver from table is copied
  \param from_dbname name of database from table is copied
  \param from_tblname name of table to be copied
  \param to_drvname name of driver to - where table is copied to
  \param to_dbname name of database to - where table is copied to
  \param to_tblname name of copied table
  \param where WHERE SQL condition (without where key word)

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_copy_table_where(const char *from_drvname, const char *from_dbname,
			const char *from_tblname, const char *to_drvname,
			const char *to_dbname, const char *to_tblname,
			const char *where)
{
    return copy_table(from_drvname, from_dbname, from_tblname,
		      to_drvname, to_dbname, to_tblname,
		      where, NULL, NULL, NULL, 0);
}

/*!
  \brief Copy a table (by select statement)

  \param from_drvname name of driver from table is copied
  \param from_dbname name of database from table is copied
  \param from_dbname name of table to be copied
  \param to_drvname name of driver to - where table is copied to
  \param to_dbname name of database to - where table is copied to
  \param to_tblname name of copied table
  \param select full select statement

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_copy_table_select(const char *from_drvname, const char *from_dbname,
			 const char *from_tblname, const char *to_drvname,
			 const char *to_dbname, const char *to_tblname,
			 const char *select)
{
    return copy_table(from_drvname, from_dbname, from_tblname,
		      to_drvname, to_dbname, to_tblname,
		      NULL, select, NULL, NULL, 0);
}

/*!
  \brief Copy a table (by keys)

  \param from_drvname name of driver from table is copied
  \param from_dbname name of database from table is copied
  \param from_tblname name of table to be copied
  \param to_drvname name of driver to - where table is copied to
  \param to_dbname name of database to - where table is copied to
  \param to_tblname name of copied table
  \param selcol name of column used to select records by values in ivals or NULL
  \param ivals pointer to array of integer values or NULL
  \param nvals number of values in ivals

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db_copy_table_by_ints(const char *from_drvname, const char *from_dbname,
			  const char *from_tblname, const char *to_drvname,
			  const char *to_dbname, const char *to_tblname,
			  const char *selcol, int *ivals, int nvals)
{
    return copy_table(from_drvname, from_dbname, from_tblname,
		      to_drvname, to_dbname, to_tblname,
		      NULL, NULL, selcol, ivals, nvals);
}
