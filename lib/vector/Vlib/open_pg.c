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
#include <stdlib.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

static char *get_key_column(struct Format_info_pg *);
static SF_FeatureType ftype_from_string(const char *);
static int drop_table(struct Format_info_pg *);
static int check_schema(const struct Format_info_pg*);
static int create_table(struct Format_info_pg *, const struct field_info *);
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
    int found;
    
    char stmt[DB_SQL_MAX];
    
    PGresult *res;
    
    struct Format_info_pg *pg_info;
    
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
    
    /* if schema not defined, use 'public' */
    if (!pg_info->schema_name) {
	pg_info->schema_name = G_store("public");
    }
	
    /* get fid and geometry column */
    sprintf(stmt, "SELECT f_geometry_column, coord_dimension, srid, type "
	    "FROM geometry_columns WHERE f_table_schema = '%s' AND "
	    "f_table_name = '%s'",
	    pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
	G_fatal_error("%s\n%s", _("No feature tables found in database."),
		      PQresultErrorMessage(res));
	
    found = PQntuples(res) > 0 ? TRUE : FALSE;
    if (found) {
	/* geometry column */
	pg_info->geom_column = G_store(PQgetvalue(res, 0, 0));
	G_debug(3, "\t-> table = %s column = %s", pg_info->table_name,
		pg_info->geom_column);
	/* fid column */
	pg_info->fid_column = get_key_column(pg_info);
	/* coordinates dimension */
	pg_info->coor_dim = atoi(PQgetvalue(res, 0, 1));
	/* SRS ID */
	pg_info->srid = atoi(PQgetvalue(res, 0, 2));
	/* feature type */
	pg_info->feature_type = ftype_from_string(PQgetvalue(res, 0, 3));
    }
    PQclear(res);
    
    /* no feature in cache */
    pg_info->cache.fid = -1;

    if (!found) {
	G_warning(_("Feature table <%s> not found in 'geometry_columns'"),
		  pg_info->table_name);
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
#ifdef HAVE_POSTGRES
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    
    PGresult *res;
    
    pg_info = &(Map->fInfo.pg);
    if (!pg_info->conninfo) {
	G_warning(_("Connection string not defined"));
	return -1;
    }
    
    if (!pg_info->table_name) {
	G_warning(_("PostGIS feature table not defined"));
	return -1;
    }

    G_debug(1, "V1_open_new_pg(): conninfo='%s' table='%s'", pg_info->conninfo,
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
    
    /* if schema not defined, use 'public' */
    if (!pg_info->schema_name) {
	pg_info->schema_name = G_store("public");
    }

    /* if fid_column not defined, use 'ogc_fid' */
    if (!pg_info->fid_column) {
	pg_info->fid_column = G_store("ogc_fid");
    }

    /* if geom_column not defined, use 'wkb_geometry' */
    if (!pg_info->geom_column) {
	pg_info->geom_column = G_store("wkb_geometry");
    }
    
    /* check if feature table already exists */
    sprintf(stmt, "SELECT * FROM pg_tables "
	    "WHERE schemaname = '%s' AND tablename = '%s'",
	    pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
	G_fatal_error("%s\n%s", _("No feature tables found in database."),
		      PQresultErrorMessage(res));

    if (PQntuples(res) > 0) {
	/* table found */
	if (G_get_overwrite()) {
	    G_warning(_("PostGIS layer <%s.%s> already exists and will be overwritten"),
		      pg_info->schema_name, pg_info->table_name);
	    if (drop_table(pg_info) == -1) {
		G_warning(_("Unable to delete PostGIS layer <%s>"),
			  pg_info->table_name);
		return -1;
	    }
	}
	else {
	    G_fatal_error(_("PostGIS layer <%s.%s> already exists in database '%s'"),
			  pg_info->schema_name, pg_info->table_name,
			  pg_info->db_name);
	    return -1;
	}
    }
    
    /* no feature in cache */
    pg_info->cache.fid = -1;

    /* unknown feature type */
    pg_info->feature_type = SF_UNKNOWN;
    
    PQclear(res);
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Create new PostGIS layer in given database (level 2)

   V1_open_new_pg() is required to be called before this function.

   List of currently supported types:
    - GV_POINT     (SF_POINT)
    - GV_LINE      (SF_LINESTRING)
    - GV_BOUNDARY  (SF_POLYGON)
   \param[in,out] Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)

   \return 0 success
   \return -1 error 
*/
int V2_open_new_pg(struct Map_info *Map, int type)
{
#ifdef HAVE_POSTGRES
    int ndblinks;
    
    struct Format_info_pg *pg_info;
    struct field_info *Fi;
    struct Key_Value *projinfo, *projunits;
    
    Fi = NULL;
    
    pg_info = &(Map->fInfo.pg);
    if (!pg_info->conninfo) {
	G_warning(_("Connection string not defined"));
	return -1;
    }

    if (!pg_info->table_name) {
	G_warning(_("PostGIS feature table not defined"));
	return -1;
    }

    G_debug(1, "V2_open_new_pg(): conninfo='%s' table='%s' -> type = %d",
	    pg_info->conninfo, pg_info->table_name, type);
    
    /* get spatial reference */
    projinfo  = G_get_projinfo();
    projunits = G_get_projunits();
    pg_info->srid = 0; /* TODO */
    // Ogr_spatial_ref = GPJ_grass_to_osr(projinfo, projunits);
    G_free_key_value(projinfo);
    G_free_key_value(projunits);

    /* determine geometry type */
    switch(type) {
    case GV_POINT:
	pg_info->feature_type = SF_POINT;
	break;
    case GV_LINE:
	pg_info->feature_type = SF_LINESTRING;
	break;
    case GV_BOUNDARY:
	pg_info->feature_type = SF_POLYGON;
	break;
    default:
	G_warning(_("Unsupported geometry type (%d)"), type);
	return -1;
    }

    /* coordinate dimension */
    pg_info->coor_dim = Vect_is_3d(Map) ? 3 : 2;
    
    /* create new PostGIS table */
    ndblinks = Vect_get_num_dblinks(Map);
    if (ndblinks > 0) {
	Fi = Vect_get_dblink(Map, 0);
	if (Fi) {
	    if (ndblinks > 1)
		G_warning(_("More layers defined, using driver <%s> and "
			    "database <%s>"), Fi->driver, Fi->database);
	}
	else {
	    G_warning(_("Database connection not defined. "
			"Unable to write attributes."));
	}
    }
    
    if (create_table(pg_info, Fi) == -1) {
	G_warning(_("Unable to create new PostGIS table"));
	return -1;
    }

    if (Fi) 
	G_free(Fi);
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
char *get_key_column(struct Format_info_pg *pg_info)
{
    char *key_column;
    char stmt[DB_SQL_MAX];
    
    PGresult *res;
    
    sprintf(stmt,
	    "SELECT kcu.column_name "
	    "FROM INFORMATION_SCHEMA.TABLES t "
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
	    "WHERE t.table_schema = '%s' AND t.table_name = '%s'",
	    pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
	PQntuples(res) != 1 || strlen(PQgetvalue(res, 0, 0)) < 1) {
	G_warning(_("No key column detected."));
	if (res)
	    PQclear(res);
	return NULL;
    }
    key_column = G_store(PQgetvalue(res, 0, 0));
    
    PQclear(res);

    return key_column;
}

SF_FeatureType ftype_from_string(const char *type)
{
    SF_FeatureType sf_type;

    if (G_strcasecmp(type, "POINT") == 0)
	return SF_POINT;
    else if (G_strcasecmp(type, "LINESTRING") == 0)
	return SF_LINESTRING;
    else if (G_strcasecmp(type, "POLYGON") == 0)
	return SF_POLYGON;
    else if (G_strcasecmp(type, "MULTIPOINT") == 0)
	return SF_MULTIPOINT;
    else if (G_strcasecmp(type, "MULTILINESTRING") == 0)
	return SF_MULTILINESTRING;
    else if (G_strcasecmp(type, "MULTIPOLYGON") == 0)
	return SF_MULTIPOLYGON;
    else if (G_strcasecmp(type, "GEOMETRYCOLLECTION") == 0)
	return SF_GEOMETRYCOLLECTION;
    
    return SF_UNKNOWN;
    
    G_debug(3, "ftype_from_string(): type='%s' -> %d", type, sf_type);
    
    return sf_type;
}

int drop_table(struct Format_info_pg *pg_info)
{
    char stmt[DB_SQL_MAX];

    sprintf(stmt, "DROP TABLE \"%s\".\"%s\"",
	    pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    if (execute(pg_info->conn, stmt) == -1) {
	return -1;
    }    
    return 0;
}

int check_schema(const struct Format_info_pg *pg_info)
{
    int i, found, nschema;
    char stmt[DB_SQL_MAX];
    
    PGresult *result;
    
    /* add geometry column */
    sprintf(stmt, "SELECT nspname FROM pg_namespace");
    G_debug(2, "SQL: %s", stmt);
    result = PQexec(pg_info->conn, stmt);
    
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
	PQclear(result);
	execute(pg_info->conn, "ROLLBACK");
	return -1;
    }
    
    found = FALSE;
    nschema = PQntuples(result);
    for (i = 0; i < nschema && !found; i++) {
	if (strcmp(pg_info->schema_name, PQgetvalue(result, i, 0)) == 0)
	    found = TRUE;
    }
    
    PQclear(result);
    
    if (!found) {
	sprintf(stmt, "CREATE SCHEMA %s", pg_info->schema_name);
	if (execute(pg_info->conn, stmt) == -1) {
	    execute(pg_info->conn, "ROLLBACK");
	    return -1;
	}
	G_warning(_("Schema <%s> doesn't exist, created"), pg_info->schema_name);
    }
    
    return 0;
}

int create_table(struct Format_info_pg *pg_info, const struct field_info *Fi)
{
    int spatial_index, primary_key;
    char stmt[DB_SQL_MAX], *geom_type;

    PGresult *result;

    /* by default create spatial index & add primary key */
    spatial_index = primary_key = TRUE;
    if (G_find_file2("", "PG", G_mapset())) {
	FILE *fp;
	const char *p;
	
	struct Key_Value *key_val;
	
	fp = G_fopen_old("", "PG", G_mapset());
	if (!fp) {
	    G_fatal_error(_("Unable to open PG file"));
	}
	key_val = G_fread_key_value(fp);
	fclose(fp);
	
	/* disable spatial index ? */
	p = G_find_key_value("spatial_index", key_val);
	if (p && G_strcasecmp(p, "off") == 0)
	    spatial_index = FALSE;
	/* disable primary key ? */
	p = G_find_key_value("primary_key", key_val);
	if (p && G_strcasecmp(p, "off") == 0)
	    primary_key = FALSE;
    }

    /* create schema if not exists */
    if (G_strcasecmp(pg_info->schema_name, "public") != 0) {
	if (check_schema(pg_info) != 0)
	    return -1;
    }
    
    /* prepare CREATE TABLE statement */
    sprintf(stmt, "CREATE TABLE \"%s\".\"%s\" (%s SERIAL",
	    pg_info->schema_name, pg_info->table_name,
	    pg_info->fid_column);
    
    if (Fi) {
	/* append attributes */
	int col, ncols, sqltype, length, ctype;
	char stmt_col[DB_SQL_MAX];
	const char *colname;
	
	dbString dbstmt;
	dbHandle  handle;
	dbDriver *driver;
	dbCursor  cursor;
	dbTable  *table;
	dbColumn *column;
	  
	db_init_string(&dbstmt);
	db_init_handle(&handle);
	
	pg_info->dbdriver = driver = db_start_driver(Fi->driver);
	if (!driver) {
	    G_warning(_("Unable to start driver <%s>"), Fi->driver);
	    return -1;
	}
	db_set_handle(&handle, Fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK) {
	    G_warning(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
	    db_close_database_shutdown_driver(driver);
	    pg_info->dbdriver = NULL;
	    return -1;
	}

	/* describe table */
	db_set_string(&dbstmt, "select * from ");
	db_append_string(&dbstmt, Fi->table);
	db_append_string(&dbstmt, " where 0 = 1");	
	
	if (db_open_select_cursor(driver, &dbstmt,
				  &cursor, DB_SEQUENTIAL) != DB_OK) {
	    G_warning(_("Unable to open select cursor: '%s'"),
		      db_get_string(&dbstmt));
	    db_close_database_shutdown_driver(driver);
	    pg_info->dbdriver = NULL;
	    return -1;
	}
	
	table = db_get_cursor_table(&cursor);
	ncols = db_get_table_number_of_columns(table);
	
	G_debug(3, "copying attributes: driver = %s database = %s table = %s cols = %d",
		Fi->driver, Fi->database, Fi->table, ncols);
		
	for (col = 0; col < ncols; col++) {
	    column = db_get_table_column(table, col);
	    colname = db_get_column_name(column);	
	    sqltype = db_get_column_sqltype(column);
	    ctype = db_sqltype_to_Ctype(sqltype);
	    length = db_get_column_length(column);
	    
	    G_debug(3, "\tcolumn = %d name = %s type = %d length = %d",
		    col, colname, sqltype, length);
	    
	    if (strcmp(pg_info->fid_column, colname) == 0) {
		/* skip fid column if exists */
		G_debug(3, "\t%s skipped", pg_info->fid_column);
		continue;
	    }
	    
	    /* append column */
	    sprintf(stmt_col, ",%s %s", colname, db_sqltype_name(sqltype));
	    strcat(stmt, stmt_col);
	    if (ctype == DB_C_TYPE_STRING) {
		/* length only for string columns */
		sprintf(stmt_col, "(%d)", length);
		strcat(stmt, stmt_col);
	    }
	}

	db_free_string(&dbstmt);
    }
    strcat(stmt, ")"); /* close CREATE TABLE statement */
    
    /* begin transaction (create table) */
    if (execute(pg_info->conn, "BEGIN") == -1) {
	return -1;
    }
    
    /* create table */
    G_debug(2, "SQL: %s", stmt);
    if (execute(pg_info->conn, stmt) == -1) {
	execute(pg_info->conn, "ROLLBACK");
	return -1;
    }
    
    /* add primary key ? */
    if (primary_key) {
	sprintf(stmt, "ALTER TABLE \"%s\".\"%s\" ADD PRIMARY KEY (%s)",
		pg_info->schema_name, pg_info->table_name,
		pg_info->fid_column);
	G_debug(2, "SQL: %s", stmt);
	if (execute(pg_info->conn, stmt) == -1) {
	    execute(pg_info->conn, "ROLLBACK");
	    return -1;
	}
    }
    
    /* determine geometry type (string) */
    switch(pg_info->feature_type) {
    case (SF_POINT):
	geom_type = "POINT";
	break;
    case (SF_LINESTRING):
	geom_type = "LINESTRING";
	break;
    case (SF_POLYGON):
	geom_type = "POLYGON";
	break;
    default:
	G_warning(_("Unsupported feature type %d"), pg_info->feature_type);
	execute(pg_info->conn, "ROLLBACK");
	return -1;
    }

    /* add geometry column */
    sprintf(stmt, "SELECT AddGeometryColumn('%s', '%s', "
	    "'%s', %d, '%s', %d)",
	    pg_info->schema_name, pg_info->table_name,
	    pg_info->geom_column, pg_info->srid,
	    geom_type, pg_info->coor_dim);
    G_debug(2, "SQL: %s", stmt);
    result = PQexec(pg_info->conn, stmt);
    
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
	PQclear(result);
	execute(pg_info->conn, "ROLLBACK");
	return -1;
    }
    
    /* create index ? */
    if (spatial_index) {
	sprintf(stmt, "CREATE INDEX %s_%s_idx ON \"%s\".\"%s\" USING GIST (%s)",
		pg_info->table_name, pg_info->geom_column,
		pg_info->schema_name, pg_info->table_name,
		pg_info->geom_column);
	G_debug(2, "SQL: %s", stmt);
	
	if (execute(pg_info->conn, stmt) == -1) {
	    execute(pg_info->conn, "ROLLBACK");
	    return -1;
	}
    }
    
    /* close transaction (create table) */
    if (execute(pg_info->conn, "COMMIT") == -1) {
	return -1;
    }
    
    return 0;
}
#endif
