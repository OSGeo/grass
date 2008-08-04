#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"
#include "proto.h"

int db__driver_open_database(dbHandle * handle)
{
    char buf[500];
    const char *name, *schema, *user, *password;
    dbConnection connection;
    PGCONN pgconn;
    PGresult *res;
    int row;

    init_error();
    db_get_connection(&connection);
    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0)
	name = connection.databaseName;

    G_debug(3,
	    "db_driver_open_database() driver=pg database definition = '%s'",
	    name);

    if (parse_conn(name, &pgconn) == DB_FAILED) {
	report_error();
	return DB_FAILED;
    }

    G_debug(3,
	    "host = %s, port = %s, options = %s, tty = %s, dbname = %s, user = %s, password = %s, "
	    "schema = %s", pgconn.host, pgconn.port, pgconn.options,
	    pgconn.tty, pgconn.dbname, pgconn.user, pgconn.password,
	    pgconn.schema);

    db_get_login("pg", name, &user, &password);

    pg_conn =
	PQsetdbLogin(pgconn.host, pgconn.port, pgconn.options, pgconn.tty,
		     pgconn.dbname, user, password);

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	append_error("Cannot connect to Postgres: ");
	append_error(PQerrorMessage(pg_conn));
	report_error();
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    /* Set schema */
    schema = db_get_handle_dbschema(handle);

    /* Cannot use default schema because link to table can point to different database */
    /*
       if ( schema ) 
       schema = connection.schemaName;
     */

    if (pgconn.schema) {
	schema = pgconn.schema;
    }

    if (schema && strlen(schema) > 0) {
	sprintf(buf, "set search_path to %s", schema);
	res = PQexec(pg_conn, buf);

	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	    append_error("Cannot set schema: ");
	    append_error(schema);
	    report_error();
	    PQclear(res);
	    return DB_FAILED;
	}
    }

    /* Read internal codes */
    res =
	PQexec(pg_conn,
	       "select oid, typname from pg_type where typname in ( "
	       "'bit', 'int2', 'int4', 'int8', 'serial', 'oid', "
	       "'float4', 'float8', 'numeric', "
	       "'char', 'bpchar', 'varchar', 'text', "
	       "'time', 'date', 'timestamp', "
	       "'bool', 'geometry' ) order by oid");

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	append_error("Cannot select data types");
	report_error();
	PQclear(res);
	return DB_FAILED;
    }

    pg_ntypes = PQntuples(res);
    pg_types = G_realloc(pg_types, 2 * pg_ntypes * sizeof(int));

    for (row = 0; row < pg_ntypes; row++) {
	int pgtype, type;

	pgtype = atoi(PQgetvalue(res, row, 0));

	pg_types[row][0] = pgtype;

	if (strcmp(PQgetvalue(res, row, 1), "bit") == 0)
	    type = PG_TYPE_BIT;
	else if (strcmp(PQgetvalue(res, row, 1), "int2") == 0)
	    type = PG_TYPE_INT2;
	else if (strcmp(PQgetvalue(res, row, 1), "int4") == 0)
	    type = PG_TYPE_INT4;
	else if (strcmp(PQgetvalue(res, row, 1), "int8") == 0)
	    type = PG_TYPE_INT8;
	else if (strcmp(PQgetvalue(res, row, 1), "serial") == 0)
	    type = PG_TYPE_SERIAL;
	else if (strcmp(PQgetvalue(res, row, 1), "oid") == 0)
	    type = PG_TYPE_OID;
	else if (strcmp(PQgetvalue(res, row, 1), "float4") == 0)
	    type = PG_TYPE_FLOAT4;
	else if (strcmp(PQgetvalue(res, row, 1), "float8") == 0)
	    type = PG_TYPE_FLOAT8;
	else if (strcmp(PQgetvalue(res, row, 1), "numeric") == 0)
	    type = PG_TYPE_NUMERIC;
	else if (strcmp(PQgetvalue(res, row, 1), "char") == 0)
	    type = PG_TYPE_CHAR;
	else if (strcmp(PQgetvalue(res, row, 1), "bpchar") == 0)
	    type = PG_TYPE_BPCHAR;
	else if (strcmp(PQgetvalue(res, row, 1), "varchar") == 0)
	    type = PG_TYPE_VARCHAR;
	else if (strcmp(PQgetvalue(res, row, 1), "text") == 0)
	    type = PG_TYPE_TEXT;
	else if (strcmp(PQgetvalue(res, row, 1), "date") == 0)
	    type = PG_TYPE_DATE;
	else if (strcmp(PQgetvalue(res, row, 1), "time") == 0)
	    type = PG_TYPE_TIME;
	else if (strcmp(PQgetvalue(res, row, 1), "timestamp") == 0)
	    type = PG_TYPE_TIMESTAMP;
	else if (strcmp(PQgetvalue(res, row, 1), "bool") == 0)
	    type = PG_TYPE_BOOL;
	else if (strcmp(PQgetvalue(res, row, 1), "geometry") == 0)
	    type = PG_TYPE_POSTGIS_GEOM;
	else
	    type = PG_TYPE_UNKNOWN;

	G_debug(3, "pgtype = %d, \tname = %s -> \ttype = %d", pgtype,
		PQgetvalue(res, row, 1), type);
	pg_types[row][1] = type;
    }

    PQclear(res);

    return DB_OK;
}

int db__driver_close_database()
{
    init_error();
    PQfinish(pg_conn);
    return DB_OK;
}
