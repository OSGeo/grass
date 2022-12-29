#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int db__driver_fetch(cn, position, more)
     dbCursor *cn;
     int position;
     int *more;
{
    cursor *c;
    dbToken token;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    int col, ncols;
    SQLLEN len;
    int htype, sqltype, ctype;
    SQLRETURN ret;
    DATE_STRUCT date;
    TIME_STRUCT time;
    TIMESTAMP_STRUCT timestamp;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_error("cursor not found");
	return DB_FAILED;
    }

    /* fetch on position */
    switch (position) {
    case DB_NEXT:
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_NEXT, 0);
	break;
    case DB_CURRENT:
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_RELATIVE, 0);
	break;
    case DB_PREVIOUS:
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_PRIOR, 0);
	break;
    case DB_FIRST:
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_FIRST, 0);
	break;
    case DB_LAST:
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_LAST, 0);
	break;
    default:
	return DB_FAILED;
    };

    if (ret == SQL_NO_DATA) {
	*more = 0;
	return DB_OK;
    }
    if (!SQL_SUCCEEDED(ret)) {
	return DB_FAILED;
    }
    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);
    ncols = db_get_table_number_of_columns(table);
    for (col = 1; col <= ncols; col++) {
	column = db_get_table_column(table, col - 1);
	value = db_get_column_value(column);
	db_free_string(&value->s);

	/* Is null? */
	SQLGetData(c->stmt, col, SQL_C_CHAR, NULL, 0, &len);
	if (len == SQL_NULL_DATA) {
	    value->isNull = 1;
	    continue;
	}
	else
	    value->isNull = 0;

	sqltype = db_get_column_sqltype(column);
	ctype = db_sqltype_to_Ctype(sqltype);
	htype = db_get_column_host_type(column);

	switch (ctype) {
	case DB_C_TYPE_STRING:
	    if (htype == SQL_CHAR) {
		len = db_get_column_length(column);
		db_enlarge_string(&value->s, len + 1);
		ret =
		    SQLGetData(c->stmt, col, SQL_C_CHAR, value->s.string,
			       len + 1, NULL);
	    }
	    else if (htype == SQL_VARCHAR) {
		ret = SQLGetData(c->stmt, col, SQL_C_CHAR, NULL, 0, &len);
		db_enlarge_string(&value->s, len + 1);
		ret =
		    SQLGetData(c->stmt, col, SQL_C_CHAR, value->s.string,
			       len + 1, NULL);
	    }
	    else {		/* now the same as SQL_VARCHAR, could differ for other htype ?  */
		ret = SQLGetData(c->stmt, col, SQL_C_CHAR, NULL, 0, &len);
		db_enlarge_string(&value->s, len + 1);
		ret =
		    SQLGetData(c->stmt, col, SQL_C_CHAR, value->s.string,
			       len + 1, NULL);
	    }
	    break;
	case DB_C_TYPE_INT:
	    ret =
		SQLGetData(c->stmt, col, SQL_C_LONG, &value->i,
			   sizeof(value->i), NULL);
	    break;
	case DB_C_TYPE_DOUBLE:
	    ret =
		SQLGetData(c->stmt, col, SQL_C_DOUBLE, &value->d,
			   sizeof(value->d), NULL);
	    break;

	case DB_C_TYPE_DATETIME:
	    switch (sqltype) {
	    case DB_SQL_TYPE_DATE:
		ret =
		    SQLGetData(c->stmt, col, SQL_C_TYPE_DATE, &date,
			       sizeof(date), NULL);
		value->t.year = date.year;
		value->t.month = date.month;
		value->t.day = date.day;
		value->t.hour = 0;
		value->t.minute = 0;
		value->t.seconds = 0.0;
		break;
	    case DB_SQL_TYPE_TIME:
		ret =
		    SQLGetData(c->stmt, col, SQL_C_TYPE_TIME, &time,
			       sizeof(time), NULL);
		value->t.year = 0;
		value->t.month = 0;
		value->t.day = 0;
		value->t.hour = time.hour;
		value->t.minute = time.minute;
		value->t.seconds = time.second;
		break;
	    case DB_SQL_TYPE_TIMESTAMP:
		ret =
		    SQLGetData(c->stmt, col, SQL_C_TYPE_TIMESTAMP, &timestamp,
			       sizeof(timestamp), NULL);
		value->t.year = timestamp.year;
		value->t.month = timestamp.month;
		value->t.day = timestamp.day;
		value->t.hour = timestamp.hour;
		value->t.minute = timestamp.minute;
		value->t.seconds = timestamp.second;
		break;
		/*  
		   case DB_SQL_TYPE_INTERVAL:
		   break;
		   default: 
		   break;
		 */
	    }

	default:
	    len = db_get_column_length(column);
	    db_enlarge_string(&value->s, len + 1);
	    ret =
		SQLGetData(c->stmt, col, SQL_C_CHAR, value->s.string, len + 1,
			   NULL);
	    break;
	}
    }
    return DB_OK;
}

int db__driver_get_num_rows(cn)
     dbCursor *cn;
{
    cursor *c;
    dbToken token;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_error("cursor not found");
	return DB_FAILED;
    }

    return (c->nrows);
}
