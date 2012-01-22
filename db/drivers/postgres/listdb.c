/*!
  \file db/driver/postgres/listdb.c
  
  \brief DBMI - Low Level PostgreSQL database driver - list databases
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_databases(dbString * dbpath, int npaths,
			      dbHandle ** dblist, int *dbcount)
{
    int i;
    PGCONN pgconn;
    PGresult *res;
    int rec_num = 0;
    dbHandle *list;

    init_error();
    *dblist = NULL;
    *dbcount = 0;

    /* TODO: the solution below is not good as user usually does not have permissions for "template1" */
    append_error
	(_("db_driver_list_databases() is not implemented in pg driver"));
    report_error();
    return DB_FAILED;

    if (npaths > 0) {
	G_debug(3, "location: %s", db_get_string(dbpath));
	if (parse_conn(db_get_string(dbpath), &pgconn) == DB_FAILED) {
	    report_error();
	    return DB_FAILED;
	}
    }

    G_debug(3, "host = %s, port = %s, options = %s, tty = %s",
	    pgconn.host, pgconn.port, pgconn.options, pgconn.tty);

    pg_conn =
	PQsetdb(pgconn.host, pgconn.port, pgconn.options, pgconn.tty,
		"template1");

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	append_error(_("Unable to connect to Postgres:\n"));
	append_error(PQerrorMessage(pg_conn));
	report_error();
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    res = PQexec(pg_conn, "select datname from pg_database");

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	append_error(_("Unable to select from Postgres:\n"));
	append_error(PQerrorMessage(pg_conn));
	report_error();
	PQclear(res);
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    rec_num = PQntuples(res);

    list = db_alloc_handle_array(rec_num);
    if (list == NULL) {
	append_error(_("Out of memory"));
	report_error();
	return DB_FAILED;
    }

    for (i = 0; i < rec_num; i++) {
	db_init_handle(&list[i]);
	if (db_set_handle(&list[i], PQgetvalue(res, i, 0), NULL) != DB_OK) {
	    append_error("db_set_handle()");
	    report_error();
	    db_free_handle_array(list, rec_num);
	    return DB_FAILED;
	}
    }

    PQclear(res);
    PQfinish(pg_conn);

    *dblist = list;
    *dbcount = rec_num;

    return DB_OK;
}
