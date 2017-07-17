#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(tlist, tcount, system)
     dbString **tlist;
     int *tcount;
     int system;
{
    cursor *c;
    dbString *list;
    int count = 0;
    SQLCHAR tableName[SQL_MAX_TABLE_NAME_LEN];
    SQLLEN indi, nrow = 0;
    SQLRETURN ret;
    char ttype[50];

    *tlist = NULL;
    *tcount = 0;

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    /* Execute SQL */
    if (system)
	sprintf(ttype, "SYSTEM TABLE");
    else
	sprintf(ttype, "TABLE, VIEW");

    ret = SQLTables(c->stmt, NULL, 0, NULL, 0, NULL, 0, ttype, sizeof(ttype));

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
	db_d_append_error("SQLTables()");
	db_d_report_error();
	return DB_FAILED;
    }

    SQLBindCol(c->stmt, 3, SQL_C_CHAR, tableName, sizeof(tableName), &indi);

    /* Get number of rows */
    /* WARNING: after SQLTables(), SQLRowCount() doesn't sets number of rows
     * to number of tables! ODBC developers said, this is correct. */
    nrow = 0;
    ret = SQLFetch(c->stmt);
    while (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
	nrow++;
	ret = SQLFetch(c->stmt);
    }

    list = db_alloc_string_array(nrow);
    if (list == NULL)
	return DB_FAILED;

    /* Get table names */
    /* ret = SQLFetch( c->stmt ); */
    ret = SQLFetchScroll(c->stmt, SQL_FETCH_FIRST, 0);
    while (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
	if (indi == SQL_NULL_DATA) {
	    if (db_set_string(&list[count], "Unknown") != DB_OK)
		return DB_FAILED;
	}
	else {
	    if (db_set_string(&list[count], (char *)tableName) != DB_OK)
		return DB_FAILED;
	}
	count++;
	ret = SQLFetch(c->stmt);
    }

    free_cursor(c);

    *tlist = list;
    *tcount = count;
    return DB_OK;
}
