#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    int i, j, nrows, trows, vrows, ncols, tablecol, tschemacol, viewcol,
	vschemacol;
    dbString *list;
    PGresult *rest, *resv;
    char buf[1000];

    init_error();
    *tlist = NULL;
    *tcount = 0;


    /* Get table names */
    rest =
	PQexec(pg_conn,
	       "select * from pg_tables where tablename !~ 'pg_*' order by tablename");

    if (!rest || PQresultStatus(rest) != PGRES_TUPLES_OK) {
	append_error("Cannot select table names\n");
	append_error(PQerrorMessage(pg_conn));
	report_error();
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
    resv =
	PQexec(pg_conn,
	       "SELECT * FROM pg_views WHERE schemaname NOT IN ('pg_catalog','information_schema') AND viewname !~ '^pg_'");

    if (!resv || PQresultStatus(resv) != PGRES_TUPLES_OK) {
	append_error("Cannot select view names\n");
	append_error(PQerrorMessage(pg_conn));
	report_error();
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
	append_error("Cannot db_alloc_string_array()");
	report_error();
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
