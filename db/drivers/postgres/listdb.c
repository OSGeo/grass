/*!
  \file db/driver/postgres/listdb.c
  
  \brief DBMI - Low Level PostgreSQL database driver - list databases
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Updated for GRASS 7 by Martin Landa <landa.martin gmail.com>
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_databases(dbString * dbpath, int npaths,
			      dbHandle ** dblist, int *dbcount)
{
    int i;
    const char *user, *passwd, *host, *port;
    PGCONN pgconn;
    PGresult *res;
    int rec_num = 0;
    dbHandle *list;

    *dblist = NULL;
    *dbcount = 0;

    /* TODO: the solution below is not good as user usually does not
     * have permissions for "template1" */

    if (npaths < 1) {
        db_d_append_error(_("No path given"));
        db_d_report_error();
        return DB_FAILED;
    }

    if (parse_conn(db_get_string(dbpath), &pgconn) == DB_FAILED) {
        db_d_report_error();
        return DB_FAILED;
    }
    
    G_debug(1, "db = %s, user = %s, pass = %s, "
            "host = %s, port = %s, options = %s, tty = %s",
	    pgconn.dbname, pgconn.user, pgconn.password, pgconn.host,
            pgconn.port, pgconn.options, pgconn.tty);

    db_get_login2("pg", NULL, &user, &passwd, &host, &port);
    G_debug(1, "user = %s, passwd = %s", user, passwd ? "xxx" : "");

    if (user || passwd) {
        pg_conn = PQsetdbLogin(host, port, pgconn.options, pgconn.tty,
                               "template1", user, passwd);
    }
    else {
        pg_conn =
            PQsetdb(host, port, pgconn.options, pgconn.tty,
                    "template1");
    }

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	db_d_append_error("%s\n%s",
			  _("Unable to connect to Postgres:"),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    res = PQexec(pg_conn, "select datname from pg_database");

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	db_d_append_error("%s\n%s",
			  _("Unable to select from Postgres:"),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(res);
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    rec_num = PQntuples(res);

    list = db_alloc_handle_array(rec_num);
    if (list == NULL) {
	db_d_append_error(_("Out of memory"));
	db_d_report_error();
	return DB_FAILED;
    }

    for (i = 0; i < rec_num; i++) {
	db_init_handle(&list[i]);
	if (db_set_handle(&list[i], PQgetvalue(res, i, 0), NULL) != DB_OK) {
	    db_d_append_error(_("Unable to set handle"));
	    db_d_report_error();
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
