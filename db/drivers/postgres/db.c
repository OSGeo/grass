/*!
  \file db/driver/postgres/db.c
  
  \brief DBMI - Low Level PostgreSQL database driver - open/close database.
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Create/drop database by Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

static void notice_processor(void *arg, const char *message)
{
    /* print notice messages only on verbose level */
    if (G_verbose() > G_verbose_std()) {
        fprintf(stderr, "%s", message);
    }
}

static int create_delete_db();

int db__driver_open_database(dbHandle * handle)
{
    char buf[500];
    const char *name, *schema, *user, *password, *host, *port;
    dbConnection connection;
    PGCONN pgconn;
    PGresult *res;
    int row;

    db_get_connection(&connection);
    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0)
	name = connection.databaseName;

    G_debug(3,
	    "db_driver_open_database(): driver=pg database definition = '%s'",
	    name);

    if (parse_conn(name, &pgconn) == DB_FAILED) {
	db_d_report_error();
	return DB_FAILED;
    }

    db_get_login2("pg", name, &user, &password, &host, &port);

    pg_conn = PQsetdbLogin(host, port, pgconn.options, pgconn.tty,
			   pgconn.dbname, user, password);

    G_debug(3,
	    "db_driver_open_database(): host = %s, port = %s, options = %s, tty = %s, "
	    "dbname = %s, user = %s, password = %s "
	    "schema = %s", host, port, pgconn.options,
	    pgconn.tty, pgconn.dbname, user, password,
	    pgconn.schema);

    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	db_d_append_error("%s\n%s",
			  _("Connection failed."),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    /* set schema */
    schema = db_get_handle_dbschema(handle);

    /* Cannot use default schema because link to table can point to
       different database */
    /*
       if ( schema ) 
       schema = connection.schemaName;
    */

    if (pgconn.schema) {
	schema = pgconn.schema;
    }

    /* set path to the schema */
    if (schema && strlen(schema) > 0) {
	sprintf(buf, "set search_path to %s", schema);
	res = PQexec(pg_conn, buf);

	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	    db_d_append_error("%s %s",
			      _("Unable to set schema:"),
			      schema);
	    db_d_report_error();
	    PQclear(res);
	    return DB_FAILED;
	}
    }

    /* read internal codes */
    res = PQexec(pg_conn,
		 "select oid, typname from pg_type where typname in ( "
		 "'bit', 'int2', 'int4', 'int8', 'serial', 'oid', "
		 "'float4', 'float8', 'numeric', "
		 "'char', 'bpchar', 'varchar', 'text', "
		 "'time', 'date', 'timestamp', "
		 "'bool', 'geometry', 'topogeometry') order by oid");
    
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	db_d_append_error(_("Unable to select data types"));
	db_d_report_error();
	PQclear(res);
	return DB_FAILED;
    }

    pg_ntypes = PQntuples(res);
    pg_types = G_realloc(pg_types, 2 * pg_ntypes * sizeof(int));

    for (row = 0; row < pg_ntypes; row++) {
	int pgtype, type;

	pgtype = atoi(PQgetvalue(res, row, 0));

	pg_types[row][0] = pgtype;

        G_debug(3, "row = %d value = %s", row, PQgetvalue(res, row, 1));
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
	else if (strcmp(PQgetvalue(res, row, 1), "topogeometry") == 0)
	    type = PG_TYPE_POSTGIS_TOPOGEOM;
	else
	    type = PG_TYPE_UNKNOWN;

	G_debug(3, "db_driver_open_database(): pgtype = %d, name = %s -> type = %d", pgtype,
		PQgetvalue(res, row, 1), type);
	pg_types[row][1] = type;
    }
    
    /* print notice messages only on verbose level */
    PQsetNoticeProcessor(pg_conn, notice_processor, NULL);
    
    PQclear(res);

    return DB_OK;
}

int db__driver_close_database()
{
    PQfinish(pg_conn);
    return DB_OK;
}

/*!
  \brief Create new empty PostgreSQL database.
  
  \param handle dbHandle
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_create_database(dbHandle *handle)
{
    return create_delete_db(handle, TRUE);
}

/*!
  \brief Drop existing PostgreSQL database.
  
  \param handle dbHandle
  
  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_delete_database(dbHandle *handle)
{
    return create_delete_db(handle, FALSE);
}

/* create or drop database */
int create_delete_db(dbHandle *handle, int create)
{
    dbString stmt;
    const char *template_db, *name, *user, *password, *host, *port;
    
    PGCONN pgconn;
    PGresult *res;
    
    db_init_string(&stmt);
    
    template_db = "template1";
    name = db_get_handle_dbname(handle); /* database to create */
    
    if (parse_conn(template_db, &pgconn) == DB_FAILED) {
	db_d_report_error();
	return DB_FAILED;
    }
    G_debug(3,
	    "db_driver_create_database(): host = %s, port = %s, options = %s, tty = %s, "
	    "dbname = %s, user = %s, password = %s, host = %s, port = %s"
	    "schema = %s", pgconn.host, pgconn.port, pgconn.options,
	    pgconn.tty, pgconn.dbname, pgconn.user, pgconn.password,
            pgconn.host, pgconn.port,
	    pgconn.schema);
    db_get_login2("pg", template_db, &user, &password, &host, &port);
    
    pg_conn = PQsetdbLogin(host, port, pgconn.options, pgconn.tty,
			   pgconn.dbname, user, password);
    if (PQstatus(pg_conn) == CONNECTION_BAD) {
	db_d_append_error(_("Connection failed."));
	db_d_append_error("\n");
	db_d_append_error("%s", PQerrorMessage(pg_conn));
	db_d_report_error();
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    /* create new database */
    if (create)
	db_set_string(&stmt, "CREATE DATABASE ");
    else
	db_set_string(&stmt, "DROP DATABASE ");
    db_append_string(&stmt, name);
    
    res = PQexec(pg_conn,
		 db_get_string(&stmt));
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	if (create)
	    db_d_append_error(_("Unable to create database <%s>"), name);
	else
	    db_d_append_error(_("Unable to drop database <%s>"), name);
	db_d_append_error("\n");
	db_d_append_error("%s", PQerrorMessage(pg_conn));
	db_d_report_error();
	
	PQclear(res);	
	PQfinish(pg_conn);
	return DB_FAILED;
    }

    PQclear(res);
    PQfinish(pg_conn);

    return DB_OK;
}
