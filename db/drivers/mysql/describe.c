
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_describe_table(dbString * table_name, dbTable ** table)
{
    dbString sql;
    MYSQL_RES *res;

    db_init_string(&sql);

    db_set_string(&sql, "select * from ");
    db_append_string(&sql, db_get_string(table_name));
    db_append_string(&sql, " where 1 = 0");

    if (mysql_query(connection, db_get_string(&sql)) != 0) {
	db_d_append_error("%s\n%s\n%s",
			  _("Unable to describe table:"),
			  db_get_string(&sql),
			  mysql_error(connection));
	db_d_report_error();
	return DB_FAILED;
    }

    res = mysql_store_result(connection);

    if (res == NULL) {
	db_d_append_error("%s\n%s",
			  db_get_string(&sql),
			  mysql_error(connection));
	db_d_report_error();
	return DB_FAILED;
    }

    if (describe_table(res, table, NULL) == DB_FAILED) {
	db_d_append_error(_("Unable to describe table"));
	db_d_report_error();
	mysql_free_result(res);
	return DB_FAILED;
    }

    mysql_free_result(res);

    db_set_table_name(*table, db_get_string(table_name));

    return DB_OK;
}

/* describe table, if c is not NULL cur->cols and cur->ncols is also set */
int describe_table(MYSQL_RES * res, dbTable ** table, cursor * c)
{
    int i, ncols, kcols;
    char *name;
    int sqltype, length;
    dbColumn *column;
    MYSQL_FIELD *fields;

    G_debug(3, "describe_table()");

    ncols = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    /* Count columns of known type */
    kcols = 0;
    for (i = 0; i < ncols; i++) {
	field_info(&(fields[i]), &sqltype, &length);

	if (sqltype == DB_SQL_TYPE_UNKNOWN)
	    continue;

	kcols++;		/* known types */
    }

    G_debug(3, "kcols = %d", kcols);

    if (!(*table = db_alloc_table(kcols))) {
	return DB_FAILED;
    }

    if (c) {
	c->ncols = kcols;
	c->cols = (int *)G_malloc(kcols * sizeof(int));
    }

    db_set_table_name(*table, "");
    db_set_table_description(*table, "");

    /* Currently not used in GRASS */
    /*
       db_set_table_delete_priv_granted (*table);
       db_set_table_insert_priv_granted (*table);
       db_set_table_delete_priv_not_granted (*table);
       db_set_table_insert_priv_not_granted (*table);
     */

    kcols = 0;
    for (i = 0; i < ncols; i++) {
	name = fields[i].name;
	field_info(&(fields[i]), &sqltype, &length);

	G_debug(3, "col: %s, kcols %d, sqltype %d", name, kcols, sqltype);

	G_debug(3, "flags = %d", fields[i].flags);

	if (sqltype == DB_SQL_TYPE_UNKNOWN) {
	    /* Print warning and continue */
	    G_warning(_("MySQL driver: column '%s', type %d "
			"is not supported"), name, fields[i].type);
	    continue;
	}

	if (fields[i].type == MYSQL_TYPE_LONGLONG)
	    G_warning(_("column '%s' : type BIGINT is stored as "
			"integer (4 bytes) some data may be damaged"), name);

	column = db_get_table_column(*table, kcols);

	db_set_column_name(column, name);
	db_set_column_length(column, length);
	db_set_column_host_type(column, (int)fields[i].type);
	db_set_column_sqltype(column, sqltype);

	db_set_column_precision(column, (int)fields[i].decimals);
	db_set_column_scale(column, 0);

	if (!(fields[i].flags & NOT_NULL_FLAG)) {
	    db_set_column_null_allowed(column);
	}
	db_set_column_has_undefined_default_value(column);
	db_unset_column_use_default_value(column);

	/* Currently not used in GRASS */
	/*
	   db_set_column_select_priv_granted (column);
	   db_set_column_update_priv_granted (column);
	   db_set_column_update_priv_not_granted (column); 
	 */

	if (c) {
	    c->cols[kcols] = i;
	}

	kcols++;
    }

    return DB_OK;
}

/* Get sqltype for field */
void field_info(MYSQL_FIELD * field, int *sqltype, int *length)
{
    *length = field->length;

    switch (field->type) {
    case MYSQL_TYPE_TINY:
	*sqltype = DB_SQL_TYPE_SMALLINT;
	break;

    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG:
	*sqltype = DB_SQL_TYPE_INTEGER;
	break;

    case MYSQL_TYPE_DECIMAL:
#ifdef MYSQL_TYPE_NEWDECIMAL
    case MYSQL_TYPE_NEWDECIMAL:
#endif
	*sqltype = DB_SQL_TYPE_DECIMAL;
	break;

    case MYSQL_TYPE_FLOAT:
	*sqltype = DB_SQL_TYPE_REAL;
	break;

    case MYSQL_TYPE_DOUBLE:
	*sqltype = DB_SQL_TYPE_DOUBLE_PRECISION;
	break;

    case MYSQL_TYPE_TIMESTAMP:
	*sqltype = DB_SQL_TYPE_TIMESTAMP;
	break;

    case MYSQL_TYPE_DATE:
	*sqltype = DB_SQL_TYPE_DATE;
	break;

    case MYSQL_TYPE_TIME:
	*sqltype = DB_SQL_TYPE_TIME;
	break;

    case MYSQL_TYPE_DATETIME:
	/* *sqltype = DB_SQL_TYPE_DATETIME; */
	/* *sqltype |= DB_DATETIME_MASK; */
	*sqltype = DB_SQL_TYPE_TIMESTAMP;
	break;

    case MYSQL_TYPE_YEAR:
	*sqltype = DB_SQL_TYPE_INTEGER;
	break;

    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_ENUM:
	*sqltype = DB_SQL_TYPE_CHARACTER;
	break;

    case MYSQL_TYPE_BLOB:
	if (field->flags & BINARY_FLAG) {
	    *sqltype = DB_SQL_TYPE_UNKNOWN;
	}
	else {
	    *sqltype = DB_SQL_TYPE_TEXT;
	}
	break;

    case MYSQL_TYPE_GEOMETRY:
    case MYSQL_TYPE_NULL:
	*sqltype = DB_SQL_TYPE_UNKNOWN;
	break;

    default:
	*sqltype = DB_SQL_TYPE_UNKNOWN;
    }

    return;
}
