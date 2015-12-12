#include <stdio.h>

#include <grass/dbmi.h>
#include <grass/datetime.h>

#include "odbc.h"
#include "globals.h"
#include "proto.h"

int set_column_type(dbColumn * column, int otype);

int db__driver_describe_table(table_name, table)
     dbString *table_name;
     dbTable **table;
{
    char *name = NULL;
    SQLINTEGER err;
    SQLRETURN ret;
    cursor *c;
    char s[100];
    char msg[OD_MSG];

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    name = db_get_string(table_name);

    SQLSetStmtAttr(c->stmt, SQL_MAX_ROWS, (SQLPOINTER *) 1, 0);

    sprintf(s, "select * from %s", name);

    ret = SQLExecDirect(c->stmt, s, SQL_NTS);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	SQLGetDiagRec(SQL_HANDLE_STMT, c->stmt, 1, NULL, &err, msg,
		      sizeof(msg), NULL);
	db_d_append_error("SQLExecDirect():\n%s\n%s (%d)\n", s, msg,
			  (int)err);
	db_d_report_error();
	return DB_FAILED;
    }

    describe_table(c->stmt, table);

    free_cursor(c);

    /* set the table name */
    db_set_table_name(*table, name);

    /* set the table description */
    db_set_table_description(*table, "");

    /* 
       db_set_table_delete_priv_granted (*table);
       db_set_table_delete_priv_not_granted (*table);
       db_set_table_insert_priv_granted (*table);
       db_set_table_insert_priv_not_granted (*table);
     */

    return DB_OK;
}


int describe_table(stmt, table)
     SQLHSTMT stmt;
     dbTable **table;
{
    dbColumn *column;
    int col;
    SQLLEN intval;
    SQLUSMALLINT ncols;
    SQLRETURN ret;
    SQLCHAR charval[100];

    /* get the number of colummns */
    ret = SQLNumResultCols(stmt, &ncols);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	db_d_append_error("SQLNumResultCols()");
	db_d_report_error();
	return DB_FAILED;
    }

    /* allocate a table structure to correspond to our system descriptor */
    if (!(*table = db_alloc_table(ncols))) {
	return DB_FAILED;
    }

    /* Get col names and attributes */
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(*table, col);

	SQLColAttribute(stmt, col + 1, SQL_COLUMN_NAME, charval,
			sizeof(charval), NULL, NULL);
	db_set_column_name(column, charval);

	/* label(title) is not description, but I did not found better attribute and it can say something about column */
	SQLColAttribute(stmt, col + 1, SQL_COLUMN_LABEL, charval,
			sizeof(charval), NULL, NULL);
	db_set_column_description(column, charval);

	SQLColAttribute(stmt, col + 1, SQL_COLUMN_LENGTH, NULL, 0, NULL,
			&intval);
	db_set_column_length(column, intval);

	SQLColAttribute(stmt, col + 1, SQL_COLUMN_PRECISION, NULL, 0, NULL,
			&intval);
	db_set_column_precision(column, intval);

	SQLColAttribute(stmt, col + 1, SQL_COLUMN_SCALE, NULL, 0, NULL,
			&intval);
	db_set_column_scale(column, intval);

	SQLColAttribute(stmt, col + 1, SQL_COLUMN_NULLABLE, NULL, 0, NULL,
			&intval);
	if (intval == SQL_NULLABLE)
	    db_set_column_null_allowed(column);
	else
	    db_unset_column_null_allowed(column);

	/*
	   db_set_column_select_priv_not_granted (column);
	   db_set_column_select_priv_not_granted (column);
	   db_set_column_update_priv_granted (column);
	   db_set_column_update_priv_not_granted (column);
	 */

	/* because set_column_type() uses other attributes (length, precision,...) must be called at the end */
	SQLColAttribute(stmt, col + 1, SQL_COLUMN_TYPE, NULL, 0, NULL,
			&intval);
	set_column_type(column, intval);
	db_set_column_host_type(column, intval);

	/* set default value after we recognized type */
	/*
	   db_set_column_has_defined_default_value(column);
	   db_set_column_has_undefined_default_value(column);
	   db_set_column_use_default_value(column);
	   db_unset_column_use_default_value(column);
	 */
	/* and set column.defaultValue */
    }

    return DB_OK;
}

int set_column_type(column, otype)
     dbColumn *column;
     int otype;
{
    int dbtype;

    /* determine the DBMI datatype from ODBC type */
    switch (otype) {
	/* numbers */
    case SQL_INTEGER:
	dbtype = DB_SQL_TYPE_INTEGER;
	break;
    case SQL_SMALLINT:
	dbtype = DB_SQL_TYPE_SMALLINT;
	break;
    case SQL_REAL:
	dbtype = DB_SQL_TYPE_REAL;
	break;
    case SQL_DOUBLE:
	dbtype = DB_SQL_TYPE_DOUBLE_PRECISION;
	break;
    case SQL_FLOAT:
	if (db_get_column_precision(column) == 24)
	    dbtype = DB_SQL_TYPE_REAL;
	else
	    dbtype = DB_SQL_TYPE_DOUBLE_PRECISION;	/* precision == 53 */
	break;
    case SQL_DECIMAL:
	dbtype = DB_SQL_TYPE_DECIMAL;
	break;
    case SQL_NUMERIC:
	dbtype = DB_SQL_TYPE_NUMERIC;
	break;

	/* strings */
    case SQL_CHAR:
	dbtype = DB_SQL_TYPE_CHARACTER;
	break;
    case SQL_VARCHAR:
	dbtype = DB_SQL_TYPE_CHARACTER;
	break;
    case SQL_LONGVARCHAR:
	dbtype = DB_SQL_TYPE_CHARACTER;
	break;

	/* date & time */
    case SQL_DATE:
	dbtype = DB_SQL_TYPE_DATE;
	break;
    case SQL_TYPE_DATE:
	dbtype = DB_SQL_TYPE_DATE;
	break;
    case SQL_TIME:
	dbtype = DB_SQL_TYPE_TIME;
	break;
    case SQL_TYPE_TIME:
	dbtype = DB_SQL_TYPE_TIME;
	break;
    case SQL_TIMESTAMP:
	dbtype = DB_SQL_TYPE_TIMESTAMP;
	break;
    case SQL_TYPE_TIMESTAMP:
	dbtype = DB_SQL_TYPE_TIMESTAMP;
	break;

    default:
	dbtype = DB_SQL_TYPE_UNKNOWN;
	break;
    }

    db_set_column_sqltype(column, dbtype);
    return DB_OK;
}
