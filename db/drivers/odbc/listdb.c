#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int db__driver_list_databases(dbString *dbpath, int npaths, dbHandle **dblist, int *dbcount)
{
    int i, count = 0;
    dbHandle *list;
    char dsn[SQL_MAX_DSN_LENGTH], desc[100];
    SQLUSMALLINT next;

    *dblist = NULL;
    *dbcount = 0;

    if (open_connection() != DB_OK)
	return DB_FAILED;

    next = SQL_FETCH_FIRST;
    while (SQLDataSources(ODenvi, next, dsn, sizeof(dsn),
			  NULL, desc, sizeof(desc), NULL) == SQL_SUCCESS) {
	next = SQL_FETCH_NEXT;
	count++;
    }

    list = db_alloc_handle_array(count);
    if (list == NULL) {
	db_d_append_error(_("Unable to allocate handle."));
	db_d_report_error();
	return DB_FAILED;
    }

    i = 0;
    next = SQL_FETCH_FIRST;
    while (SQLDataSources(ODenvi, next, dsn, sizeof(dsn),
			  NULL, desc, sizeof(desc), NULL) == SQL_SUCCESS) {
	db_init_handle(&list[i]);
	if (db_set_handle(&list[i], dsn, desc) != DB_OK) {
	    db_d_append_error(_("Unable to set handle"));
	    db_d_report_error();
	    db_free_handle_array(list, count);
	    return DB_FAILED;
	}
	next = SQL_FETCH_NEXT;
	i++;
    }

    *dblist = list;
    *dbcount = count;
    close_connection();

    return DB_OK;
}
