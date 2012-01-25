#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"
#include <stdio.h>

int db__driver_open_select_cursor(dbString *sel, dbCursor *dbc, int mode)
{
    cursor *c;
    SQLRETURN ret;
    SQLINTEGER err;
    char *sql, msg[OD_MSG];
    dbTable *table;
    int nrows;

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    db_set_cursor_mode(dbc, mode);
    db_set_cursor_type_readonly(dbc);

    sql = db_get_string(sel);

    ret = SQLExecDirect(c->stmt, sql, SQL_NTS);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	SQLGetDiagRec(SQL_HANDLE_STMT, c->stmt, 1, NULL, &err, msg,
		      sizeof(msg), NULL);
	db_d_append_error("SQLExecDirect():\n%s\n%s (%d)", sql, msg, (int)err);
	db_d_report_error();
	return DB_FAILED;
    }

    describe_table(c->stmt, &table);


    db_set_cursor_table(dbc, table);

    /* record table with dbCursor */
    db_set_cursor_table(dbc, table);

    /* set dbCursor's token for my cursor */
    db_set_cursor_token(dbc, c->token);

    /* It seems that there is no function in ODBC to get number of selected rows.
     *  SQLRowCount() works for insert, update, delete. */
    nrows = 0;
    while (1) {
	ret = SQLFetchScroll(c->stmt, SQL_FETCH_NEXT, 0);
	if (ret == SQL_NO_DATA) {
	    break;
	}
	if (!SQL_SUCCEEDED(ret)) {
	    return DB_FAILED;
	}
	nrows++;
    }
    c->nrows = nrows;
    SQLFetchScroll(c->stmt, SQL_FETCH_FIRST, 0);
    SQLFetchScroll(c->stmt, SQL_FETCH_PRIOR, 0);

    return DB_OK;
}
