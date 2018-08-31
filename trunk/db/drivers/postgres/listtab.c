/*!
  \file db/driver/postgres/listdb.c
  
  \brief DBMI - Low Level PostgreSQL database driver - list tables
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */
#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    int i, j, nrows, trows, vrows, ncols, tablecol, tschemacol, viewcol,
	vschemacol;
    dbString *list;
    PGresult *rest, *resv;
    char buf[DB_SQL_MAX];

    *tlist = NULL;
    *tcount = 0;


    /* Get table names */
    sprintf(buf, "SELECT * FROM pg_tables WHERE schemaname %s "
            " ('pg_catalog', 'information_schema') ORDER BY tablename", system ? "IN" : "NOT IN");
    G_debug(2, "SQL: %s", buf);
    
    rest = PQexec(pg_conn, buf);
    if (!rest || PQresultStatus(rest) != PGRES_TUPLES_OK) {
	db_d_append_error("%s\n%s",
			  _("Unable to select table names."),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(rest);
	return DB_FAILED;
    }

    /* Find table and schema col */
    ncols = PQnfields(rest);
    tschemacol = -1;
    for (i = 0; i < ncols; i++) {
	if (strcmp(PQfname(rest, i), "tablename") == 0)
	    tablecol = i;

	if (strcmp(PQfname(rest, i), "schemaname") == 0)
	    tschemacol = i;
    }


    /* Get view names */
    sprintf(buf, "SELECT * FROM pg_views WHERE schemaname %s "
            " ('pg_catalog', 'information_schema') ORDER BY viewname", system ? "IN" : "NOT IN");
    G_debug(2, "SQL: %s", buf);
    
    resv = PQexec(pg_conn, buf);
    if (!resv || PQresultStatus(resv) != PGRES_TUPLES_OK) {
	db_d_append_error("%s\n%s",
			  _("Unable to select view names."),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(resv);
	return DB_FAILED;
    }

    /* Find viewname and schema col */
    ncols = PQnfields(resv);
    vschemacol = -1;
    for (i = 0; i < ncols; i++) {
	if (strcmp(PQfname(resv, i), "viewname") == 0)
	    viewcol = i;

	if (strcmp(PQfname(resv, i), "schemaname") == 0)
	    vschemacol = i;
    }



    trows = PQntuples(rest);
    vrows = PQntuples(resv);
    nrows = trows + vrows;

    list = db_alloc_string_array(nrows);

    if (list == NULL) {
	db_d_append_error(_("Out of memory"));
	db_d_report_error();
	return DB_FAILED;
    }

    for (i = 0; i < trows; i++) {
	if (tschemacol >= 0) {
	    sprintf(buf, "%s.%s", (char *)PQgetvalue(rest, i, tschemacol),
		    (char *)PQgetvalue(rest, i, tablecol));
	}
	else {
	    sprintf(buf, "%s", (char *)PQgetvalue(rest, i, tablecol));
	}
	db_set_string(&list[i], buf);
    }

    PQclear(rest);


    for (j = 0; j < vrows; j++) {
	if (vschemacol >= 0) {
	    sprintf(buf, "%s.%s", (char *)PQgetvalue(resv, j, vschemacol),
		    (char *)PQgetvalue(resv, j, viewcol));
	}
	else {
	    sprintf(buf, "%s", (char *)PQgetvalue(resv, j, viewcol));
	}
	db_set_string(&list[i], buf);
	i++;
    }

    PQclear(resv);


    *tlist = list;
    *tcount = nrows;
    return DB_OK;
}
