#include <grass/config.h>
#if defined(HAVE_LIBPQ_FE_H)
#include <grass/gis.h>
#include "pg.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>

char *runPg(char *SQL_stmt)
{
    char buf[QRY_LENGTH];
    char chunk[QRY_LENGTH];
    static char long_str[2 * QRY_LENGTH];

    char sqlcmd[QRY_LENGTH];
    int i, j, nrows, nfields;

    PGconn *pg_conn;
    PGresult *res;
    char *pghost;
    int vrbs = 1;

    memset(long_str, '\0', sizeof(long_str));
    memset(sqlcmd, '\0', sizeof(sqlcmd));
    memset(buf, '\0', sizeof(buf));
    memset(chunk, '\0', sizeof(chunk));

    sprintf(sqlcmd, "%s", SQL_stmt);

    if (vrbs)
	fprintf(stderr, "\n\nExecuting\n%s\n---------------------\n", sqlcmd);

    pghost = G__getenv("PG_HOST");
    pg_conn = PQsetdb(pghost, NULL, NULL, NULL, G_getenv("PG_DBASE"));

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	fprintf(stderr, "Error: connect Postgres:%s\n",
		PQerrorMessage(pg_conn));
	PQfinish(pg_conn);
	return NULL;
    }

    res = PQexec(pg_conn, sqlcmd);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	fprintf(stderr, "Error: select Postgres:%s\n",
		PQerrorMessage(pg_conn));
	PQclear(res);
	PQfinish(pg_conn);
	return NULL;
    }

    nfields = PQnfields(res);
    nrows = PQntuples(res);

    if (nrows == 1 && vrbs) {
	for (j = 0; j < nfields; j++) {
	    strncpy(buf, PQgetvalue(res, 0, j), QRY_LENGTH);
	    sprintf(chunk, "%10s I %s\n", PQfname(res, j), buf);
	    strncat(long_str, chunk, QRY_LENGTH);
	}
    }
    else if (nrows) {

	fprintf(stderr, "%s", PQfname(res, 0));
	for (j = 1; j < nfields; j++) {
	    fprintf(stderr, ",%s", PQfname(res, j));
	}
	fprintf(stderr, "\n");

	for (i = 0; i < nrows; i++) {
	    for (j = 0; j < nfields; j++) {
		strncpy(buf, PQgetvalue(res, i, j), QRY_LENGTH);
		fprintf(stderr, "%s,", buf);
	    }
	    fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n%d rows selected\n", nrows);
    }

    if (vrbs) {
	sprintf(chunk, "\n%d rows selected\n", nrows);
	strncat(long_str, chunk, QRY_LENGTH);
    }


    PQclear(res);
    /* explicitly close select result to avoid memory leaks  */

    PQfinish(pg_conn);
    /* close connection to database */

    return long_str;
}

char *do_query(char *SQL_stmt, struct Sql *pts)
{
    char buf[QRY_LENGTH];
    char chunk[QRY_LENGTH];
    static char long_str[2 * QRY_LENGTH];

    char sqlcmd[QRY_LENGTH];
    int i, j, nrows, nfields;

    PGconn *pg_conn;
    PGresult *res;
    char *pghost;

    memset(long_str, '\0', sizeof(long_str));
    memset(sqlcmd, '\0', sizeof(sqlcmd));
    memset(buf, '\0', sizeof(buf));
    memset(chunk, '\0', sizeof(chunk));


    sprintf(sqlcmd,
	    "%s @ '(%f,%f,%f,%f)'::box", SQL_stmt,
	    pts->minX, pts->minY, pts->maxX, pts->maxY);

    fprintf(stderr,
	    "\n\nExecuting\n%s;\n clause  @ '( )'::box addded autonmatically.\n\n",
	    sqlcmd);
    pghost = G__getenv("PG_HOST");
    pg_conn = PQsetdb(pghost, NULL, NULL, NULL, G_getenv("PG_DBASE"));

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	fprintf(stderr, "Error: connect Postgres:%s\n",
		PQerrorMessage(pg_conn));
	PQfinish(pg_conn);
	return NULL;
    }

    res = PQexec(pg_conn, sqlcmd);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	fprintf(stderr, "Error: select Postgres:%s\n",
		PQerrorMessage(pg_conn));
	PQclear(res);
	PQfinish(pg_conn);
	return NULL;
    }

    nfields = PQnfields(res);
    nrows = PQntuples(res);

    if (nrows == 1) {
	for (j = 0; j < nfields; j++) {
	    strncpy(buf, PQgetvalue(res, 0, j), QRY_LENGTH);
	    sprintf(chunk, "%10s I %s\n", PQfname(res, j), buf);
	    strncat(long_str, chunk, QRY_LENGTH);
	}
    }
    else if (nrows) {

	fprintf(stderr, "%s", PQfname(res, 0));
	for (j = 1; j < nfields; j++) {
	    fprintf(stderr, ",%s", PQfname(res, j));
	}
	fprintf(stderr, "\n");

	for (i = 0; i < nrows; i++) {
	    for (j = 0; j < nfields; j++) {
		strncpy(buf, PQgetvalue(res, i, j), QRY_LENGTH);
		fprintf(stderr, "%s,", buf);
	    }
	    fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n%d rows selected\n", nrows);
    }


    sprintf(chunk, "\n%d rows selected\n", nrows);
    strncat(long_str, chunk, QRY_LENGTH);


    PQclear(res);
    /* explicitly close select result to avoid memory leaks  */

    PQfinish(pg_conn);
    /* close connection to database */

    return long_str;
}
#endif
