
/**
 * \file describe.c
 *
 * \brief Low level SQLite database driver.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <string.h>
#include <grass/dbmi.h>
#include <grass/datetime.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

/* function prototypes */
static int affinity_type(const char *);
static int parse_type(const char *, int *);
static void get_column_info(sqlite3_stmt * statement, int col,
			    int *litetype, int *sqltype, int *length);

/**
 * \fn int db__driver_describe_table (dbString *table_name, dbTable **table)
 *
 * \brief Low level SQLite describe database table.
 *
 * \param[in] table_name
 * \param[in] table
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_describe_table(dbString * table_name, dbTable ** table)
{
    dbString sql;
    sqlite3_stmt *statement;
    const char *rest;
    int ret;

    db_init_string(&sql);

    db_set_string(&sql, "select * from ");
    db_append_string(&sql, db_get_string(table_name));
    db_append_string(&sql, " where oid < 0");

    /* SQLITE bug?
     * If the database schema has changed, sqlite can prepare a statement,
     * but sqlite can not step, the statement needs to be prepared anew again */
    while (1) {
	ret = sqlite3_prepare(sqlite, db_get_string(&sql), -1, &statement, &rest);

	if (ret != SQLITE_OK) {
	    db_d_append_error("%s %s\n%s",
			      _("Error in sqlite3_prepare():"),
			      db_get_string(&sql),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    db_free_string(&sql);
	    return DB_FAILED;
	}

	ret = sqlite3_step(statement);
	/* get real result code */
	ret = sqlite3_reset(statement);

	if (ret == SQLITE_SCHEMA) {
	    sqlite3_finalize(statement);
	    /* try again */
	}
	else if (ret != SQLITE_OK) {
	    db_d_append_error("%s\n%s",
			      _("Error in sqlite3_step():"),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    sqlite3_finalize(statement);
	    return DB_FAILED;
	}
	else
	    break;
    }

    db_free_string(&sql);

    if (describe_table(statement, table, NULL) == DB_FAILED) {
	db_d_append_error("%s\n%s",
			  _("Unable to describe table:"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	sqlite3_finalize(statement);
	return DB_FAILED;
    }

    sqlite3_finalize(statement);

    return DB_OK;
}


/**
 * \fn int describe_table (sqlite3_stmt *statement, dbTable **table, cursor *c)
 *
 * \brief SQLite describe table.
 *
 * NOTE: If <b>c</b> is not NULL c->cols and c->ncols are also set.
 *
 * \param[in] statement
 * \param[in] table
 * \param[in] c SQLite cursor. See NOTE.
 * \return int DB_FAILED on error; DB_OK on success
 */

int describe_table(sqlite3_stmt * statement, dbTable ** table, cursor * c)
{
    int i, ncols, nkcols, ret;

    G_debug(3, "describe_table()");

    ncols = sqlite3_column_count(statement);

    /* Try to get first row */
    ret = sqlite3_step(statement);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
	/* get real result code */
	ret = sqlite3_reset(statement);
	db_d_append_error("%s\n%s",
			  _("Error in sqlite3_step():"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	return DB_FAILED;
    }

    /* Count columns of known type */
    nkcols = 0;

    for (i = 0; i < ncols; i++) {
	int litetype, sqltype, length;

	get_column_info(statement, i, &litetype, &sqltype, &length);

	if (sqltype == DB_SQL_TYPE_UNKNOWN)
	    continue;

	nkcols++;		/* known types */
    }

    G_debug(3, "nkcols = %d", nkcols);

    if (c) {
	c->kcols = (int *)G_malloc(nkcols * sizeof(int));
	c->nkcols = nkcols;
    }

    if (!(*table = db_alloc_table(nkcols))) {
	return DB_FAILED;
    }


    /* set the table name */
    /* TODO */
    db_set_table_name(*table, "");

    /* set the table description */
    db_set_table_description(*table, "");

    /* TODO */
    /*
       db_set_table_delete_priv_granted (*table);
       db_set_table_insert_priv_granted (*table);
       db_set_table_delete_priv_not_granted (*table);
       db_set_table_insert_priv_not_granted (*table);
     */

    nkcols = 0;
    for (i = 0; i < ncols; i++) {
	const char *fname;
	dbColumn *column;
	int litetype, sqltype, fsize, precision, scale;

	fname = sqlite3_column_name(statement, i);

	get_column_info(statement, i, &litetype, &sqltype, &fsize);

	G_debug(2, "col: %s, nkcols %d, litetype : %d, sqltype %d",
		fname, nkcols, litetype, sqltype);

	if (sqltype == DB_SQL_TYPE_UNKNOWN) {
	    /* Warn, ignore and continue */
	    G_warning(_("SQLite driver: column '%s', SQLite type %d  is not supported"),
		      fname, litetype);
	    continue;
	}

	switch (sqltype) {
	case DB_SQL_TYPE_SMALLINT:
	case DB_SQL_TYPE_INTEGER:
	case DB_SQL_TYPE_SERIAL:
	    fsize = 20;
	    break;

	case DB_SQL_TYPE_REAL:
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	case DB_SQL_TYPE_DECIMAL:
	case DB_SQL_TYPE_NUMERIC:
	    fsize = 20;
	    break;

	case DB_SQL_TYPE_DATE:
	case DB_SQL_TYPE_TIME:
	case DB_SQL_TYPE_TIMESTAMP:
	case DB_SQL_TYPE_INTERVAL:
	    fsize = 20;
	    break;

	case DB_SQL_TYPE_CHARACTER:
	    /* fsize is already correct */
	    break;

	case DB_SQL_TYPE_TEXT:
	    /* fudge for clients which don't understand variable-size fields */
	    fsize = 1000;
	    break;

	default:
	    G_warning("SQLite driver: unknown type: %d", sqltype);
	    fsize = 99999;	/* sqlite doesn't care, it must be long enough to
				   satisfy tests in GRASS */
	}

	column = db_get_table_column(*table, nkcols);

	db_set_column_name(column, fname);
	db_set_column_length(column, fsize);
	db_set_column_host_type(column, litetype);
	db_set_column_sqltype(column, sqltype);

	/* TODO */
	precision = 0;
	scale = 0;
	/*
	   db_set_column_precision (column, precision);
	   db_set_column_scale (column, scale);
	 */

	/* TODO */
	db_set_column_null_allowed(column);
	db_set_column_has_undefined_default_value(column);
	db_unset_column_use_default_value(column);

	/* TODO */
	/*
	   db_set_column_select_priv_granted (column);
	   db_set_column_update_priv_granted (column);
	   db_set_column_update_priv_not_granted (column); 
	 */

	if (c) {
	    c->kcols[nkcols] = i;
	}

	nkcols++;
    }

    sqlite3_reset(statement);

    return DB_OK;
}


static int dbmi_type(int litetype)
{
    switch (litetype) {
    case SQLITE_INTEGER:
	return DB_SQL_TYPE_INTEGER;
    case SQLITE_FLOAT:
	return DB_SQL_TYPE_DOUBLE_PRECISION;
    case SQLITE_TEXT:
	return DB_SQL_TYPE_TEXT;
    case SQLITE_NULL:
	return DB_SQL_TYPE_TEXT;	/* good choice? */
    default:
	return DB_SQL_TYPE_UNKNOWN;
    }
}

/**
 * \fn void get_column_info (sqlite3_stmt *statement, int col, int *litetype, int *sqltype)
 *
 * \brief Low level SQLite get column information.
 *
 * \param[in] statement
 * \param[in] col
 * \param[in,out] litetype
 * \param[in,out] sqltype
 */

static void get_column_info(sqlite3_stmt * statement, int col,
			    int *litetype, int *sqltype, int *length)
{
    const char *decltype;

    decltype = sqlite3_column_decltype(statement, col);
    if (decltype) {
	G_debug(4, "column: %s, decltype = %s",
		sqlite3_column_name(statement, col), decltype);
	*sqltype = parse_type(decltype, length);
	*litetype = affinity_type(decltype);
    }
    else {
	G_debug(4, "this is not a table column");

	/* If there are no results it gives 0 */
	*litetype = sqlite3_column_type(statement, col);
	*sqltype = dbmi_type(*litetype);
	*length = 0;
    }

    G_debug(3, "sqltype = %d", *sqltype);
    G_debug(3, "litetype = %d", *litetype);
}

/*  SQLite documentation:
 *
 *   The type affinity of a column is determined by the declared 
 *   type of the column, according to the following rules:
 *
 *   1. If the datatype contains the string "INT" 
 *      then it is assigned INTEGER affinity.
 *
 *   2. If the datatype of the column contains any of the strings 
 *      "CHAR", "CLOB", or "TEXT" then that column has TEXT affinity. 
 *      Notice that the type VARCHAR contains the string "CHAR" 
 *      and is thus assigned TEXT affinity.
 *
 *   3. If the datatype for a column contains the string "BLOB" 
 *      or if no datatype is specified then the column has affinity NONE.
 *
 *   4. Otherwise, the affinity is NUMERIC.
 */

static int affinity_type(const char *declared)
{
    char *lc;
    int aff = SQLITE_FLOAT;

    lc = G_store(declared);
    G_tolcase(lc);
    G_debug(4, "affinity_type: %s", lc);

    if (strstr(lc, "int")) {
	aff = SQLITE_INTEGER;
    }
    else if (strstr(lc, "char") || strstr(lc, "clob")
	     || strstr(lc, "text") || strstr(lc, "date")) {
	aff = SQLITE_TEXT;
    }
    else if (strstr(lc, "blob")) {
	aff = SQLITE_BLOB;
    }

    G_free(lc);

    return aff;
}

static int parse_type(const char *declared, int *length)
{
    char buf[256];
    char word[4][256];

    strncpy(buf, declared, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    G_chop(buf);
    G_tolcase(buf);

    *length = 1;

#define streq(a,b) (strcmp((a),(b)) == 0)

    if (streq(buf, "smallint") || streq(buf, "int2"))
	return DB_SQL_TYPE_SMALLINT;

    if (streq(buf, "integer") ||
	streq(buf, "int") ||
	streq(buf, "int4") || streq(buf, "bigint") || streq(buf, "int8"))
	return DB_SQL_TYPE_INTEGER;

    if (streq(buf, "real") || streq(buf, "float4"))
	return DB_SQL_TYPE_REAL;

    if (streq(buf, "double") || streq(buf, "float8"))
	return DB_SQL_TYPE_DOUBLE_PRECISION;

    if (streq(buf, "decimal"))
	return DB_SQL_TYPE_DECIMAL;

    if (streq(buf, "numeric"))
	return DB_SQL_TYPE_NUMERIC;

    if (streq(buf, "date"))
	return DB_SQL_TYPE_DATE;
    if (streq(buf, "time") || streq(buf, "timetz"))
	return DB_SQL_TYPE_TIME;

    if (streq(buf, "timestamp") || streq(buf, "timestamptz") || 
	streq(buf, "datetime"))
	return DB_SQL_TYPE_TIMESTAMP;

    if (streq(buf, "interval"))
	return DB_SQL_TYPE_INTERVAL;

    if (streq(buf, "text"))
	return DB_SQL_TYPE_TEXT;

    if (streq(buf, "serial") || streq(buf, "serial4"))
	return DB_SQL_TYPE_SERIAL;

    if (streq(buf, "character")
	|| streq(buf, "char")
	|| streq(buf, "varchar"))
	return DB_SQL_TYPE_TEXT;

    if (sscanf(buf, "%s %s", word[0], word[1]) == 2) {
	if (streq(word[0], "double") && streq(word[1], "precision"))
	    return DB_SQL_TYPE_DOUBLE_PRECISION;
	if (streq(word[0], "character") && streq(word[1], "varying"))
	    return DB_SQL_TYPE_TEXT;
    }

    if (sscanf(buf, "%s %s %s %s", word[0], word[1], word[2], word[3]) == 4 &&
	(streq(word[1], "with") || streq(word[1], "without")) &&
	streq(word[2], "time") && streq(word[3], "zone")) {
	if (streq(word[0], "time"))
	    return DB_SQL_TYPE_TIME;
	if (streq(word[0], "timestamp") || streq(word[0], "datetime"))
	    return DB_SQL_TYPE_TIMESTAMP;
    }

    if (sscanf(buf, "varchar ( %d )", length) == 1 ||
	sscanf(buf, "character varying ( %d )", length) == 1 ||
	sscanf(buf, "character ( %d )", length) == 1 ||
	sscanf(buf, "char ( %d )", length) == 1)
	return DB_SQL_TYPE_CHARACTER;

    if (sscanf(buf, "interval ( %d )", length) == 1)
	return DB_SQL_TYPE_INTERVAL;

    if (sscanf(buf, "numeric ( %d , %d )", length, length) == 2)
	return DB_SQL_TYPE_NUMERIC;

    if (sscanf(buf, "decimal ( %d , %d )", length, length) == 2)
	return DB_SQL_TYPE_DECIMAL;

    if (sscanf(buf, "time ( %d )", length) == 1 ||
	sscanf(buf, "timetz ( %d )", length) == 1)
	return DB_SQL_TYPE_TIME;

    if (sscanf(buf, "timestamp ( %d )", length) == 1 ||
	sscanf(buf, "timestamptz ( %d )", length) == 1 ||
	sscanf(buf, "datetime ( %d )", length) == 1 )
	return DB_SQL_TYPE_TIMESTAMP;

    if (sscanf
	(buf, "%s ( %d ) %s %s %s", word[0], length, word[1], word[2],
	 word[3]) == 5 && (streq(word[1], "with") ||
			   streq(word[1], "without")) &&
	streq(word[2], "time") && streq(word[3], "zone")) {
	if (streq(word[0], "time"))
	    return DB_SQL_TYPE_TIME;
	if (streq(word[0], "timestamp") || streq(word[0], "datetime"))
	    return DB_SQL_TYPE_TIMESTAMP;
    }

#undef streq

    G_warning("SQLite driver: unable to parse decltype: %s", declared);

    return DB_SQL_TYPE_UNKNOWN;
}
