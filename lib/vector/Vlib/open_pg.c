/*!
   \file lib/vector/Vlib/open_pg.c

   \brief Vector library - Open PostGIS layer as vector map layer

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2011-2012 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <string.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

static char *get_key_column(struct Format_info_pg *pg_info)
{
    char *key_column;
    char stmt[DB_SQL_MAX];
    
    PGresult *res;
    
    sprintf(stmt,
	    "SELECT kcu.column_name "
	    "FROM    INFORMATION_SCHEMA.TABLES t "
	    "LEFT JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc "
	    "ON tc.table_catalog = t.table_catalog "
	    "AND tc.table_schema = t.table_schema "
	    "AND tc.table_name = t.table_name "
	    "AND tc.constraint_type = 'PRIMARY KEY' "
	    "LEFT JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu "
	    "ON kcu.table_catalog = tc.table_catalog "
	    "AND kcu.table_schema = tc.table_schema "
	    "AND kcu.table_name = tc.table_name "
	    "AND kcu.constraint_name = tc.constraint_name "
	    "WHERE t.table_name = '%s'", pg_info->table_name);

    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
	PQntuples(res) != 1) {
	G_warning(_("No key column detected."));
	if (res)
	    PQclear(res);
	return NULL;
    }
    key_column = G_store(PQgetvalue(res, 0, 0));
    
    PQclear(res);

    return key_column;
}
#endif

/*!
   \brief Open existing PostGIS feature table (level 1 - without topology)
   
   \todo Check database instead of geometry_columns
   
   \param[in,out] Map pointer to Map_info structure
   \param update TRUE for write mode, otherwise read-only
   
   \return 0 success
   \return -1 error
*/
int V1_open_old_pg(struct Map_info *Map, int update)
{
#ifdef HAVE_POSTGRES
    int found, ntables, i;
    
    dbString stmt;
    
    PGresult *res;
    
    struct Format_info_pg *pg_info;
    
    db_init_string(&stmt);
    
    pg_info = &(Map->fInfo.pg);
    if (!pg_info->conninfo) {
	G_warning(_("Connection string not defined"));
	return -1;
    }
    
    if (!pg_info->table_name) {
	G_warning(_("PostGIS feature table not defined"));
	return -1;
    }

    G_debug(1, "V1_open_old_pg(): conninfo='%s' table='%s'", pg_info->conninfo,
	    pg_info->table_name);

    /* connect database */
    pg_info->conn = PQconnectdb(pg_info->conninfo);
    G_debug(2, "   PQconnectdb(): %s", pg_info->conninfo);
    if (PQstatus(pg_info->conn) == CONNECTION_BAD)
	G_fatal_error("%s\n%s", _("Connection ton PostgreSQL database failed."), 
		      PQerrorMessage(pg_info->conn));

    /* get DB name */
    pg_info->db_name = G_store(PQdb(pg_info->conn));
    if (!pg_info->db_name) {
	G_warning(_("Unable to get database name"));
	return -1;
    }
    
    /* get fid and geometry column */
    db_set_string(&stmt, "SELECT f_table_name, f_geometry_column "
		  "FROM geometry_columns");
    G_debug(2, "SQL: %s", db_get_string(&stmt));
    
    res = PQexec(pg_info->conn, db_get_string(&stmt));
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
	G_fatal_error("%s\n%s", _("No feature tables found in database."),
		      PQresultErrorMessage(res));
		      
    ntables = PQntuples(res);
    G_debug(3, "\tnrows = %d", ntables);
    found = FALSE;
    for (i = 0; i < ntables; i++) {
	if (strcmp(PQgetvalue(res, i, 0), pg_info->table_name) == 0) {
	    pg_info->geom_column = G_store(PQgetvalue(res, i, 1));
	    G_debug(3, "\t-> table = %s column = %s", pg_info->table_name,
		    pg_info->geom_column);
	    pg_info->fid_column = get_key_column(pg_info);
	    found = TRUE;
	    break;
	}
    }

    /* no feature in cache */
    pg_info->cache.fid = -1;
    
    PQclear(res);
    
    db_free_string(&stmt);

    if (!found) {
	G_warning(_("Feature table <%s> not found in 'geometry_columns'"));
	return -1;
    }
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Open existing PostGIS layer (level 2 - feature index)

   \param[in,out] Map pointer to Map_info structure
   
   \return 0 success
   \return -1 error
*/
int V2_open_old_pg(struct Map_info *Map)
{
#ifdef HAVE_POSTGRES

    G_debug(3, "V2_open_old_pg(): name = %s mapset = %s", Map->name,
	    Map->mapset);

    if (Vect_open_fidx(Map, &(Map->fInfo.pg.offset)) != 0) {
	G_warning(_("Unable to open feature index file for vector map <%s>"),
		  Vect_get_full_name(Map));
	G_zero(&(Map->fInfo.pg.offset), sizeof(struct Format_info_offset));
    }
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Prepare PostGIS database for creating new feature table
   (level 1)

   \todo To implement
   
   \param[out] Map pointer to Map_info structure
   \param name name of PostGIS feature table to create
   \param with_z WITH_Z for 3D vector data otherwise WITHOUT_Z

   \return 0 success
   \return -1 error 
*/
int V1_open_new_pg(struct Map_info *Map, const char *name, int with_z)
{
    G_debug(1, "V1_open_new_pg(): name = %s with_z = %d", name, with_z);
#ifdef HAVE_POSTGRES
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}
