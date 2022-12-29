/*!
   \file lib/vector/Vlib/open_pg.c

   \brief Vector library - Open PostGIS layer as vector map layer

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2011-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
*/

#include <string.h>
#include <stdlib.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

struct line_data {
    int id;
    int fid;
    int start_node;
    int end_node;
    int left_face;
    int right_face;
    char *wkb_geom;
};

static char *get_key_column(struct Format_info_pg *);
static SF_FeatureType ftype_from_string(const char *);
static int drop_table(struct Format_info_pg *);
static void connect_db(struct Format_info_pg *);
static int check_topo(struct Format_info_pg *, struct Plus_head *);
static int parse_bbox(const char *, struct bound_box *);
static struct P_node *read_p_node(struct Plus_head *, int, int,
                                  const char *, const char *, const char *,
                                  struct Format_info_pg *, int);
static struct P_line *read_p_line(struct Plus_head *, int,
                                  const struct line_data *, int,
                                  struct Format_info_cache *);
static struct P_area *read_p_area(struct Plus_head *, int,
                                  const char *, int, const char *);
static struct P_isle *read_p_isle(struct Plus_head *, int,
                                  const char *, int);
static void notice_processor(void *, const char *);
static char **scan_array(const char *);
static int remap_node(const struct Format_info_offset *, int);
static int remap_line(const struct Plus_head*, off_t, int);
#endif

/*!
   \brief Open vector map - PostGIS feature table on non-topological
   level

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

    G_debug(2, "V1_open_old_pg(): update = %d", update);
    
    pg_info = &(Map->fInfo.pg);
    if (!pg_info->conninfo) {
        G_warning(_("Connection string not defined"));
        return -1;
    }

    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }

    G_debug(1, "V1_open_old_pg(): conninfo='%s' table='%s'",
            pg_info->conninfo, pg_info->table_name);

    /* connect database */
    if (!pg_info->conn)
        connect_db(pg_info);
    
    /* get fid and geometry column */
    sprintf(stmt, "SELECT f_geometry_column, coord_dimension, srid, type "
            "FROM geometry_columns WHERE f_table_schema = '%s' AND "
            "f_table_name = '%s'", pg_info->schema_name, pg_info->table_name);
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

    /* fid -1 for empty / -2 for full cache */
    pg_info->cache.fid = pg_info->cache.ctype != CACHE_MAP ? -1 : -2;

    if (!found) {
        G_warning(_("Feature table <%s> not found in 'geometry_columns'"),
                  pg_info->table_name);
        return 0; /* avoid calling G_fatal_error() */
    }

    /* check for topo schema */
    check_topo(pg_info, &(Map->plus));
    
    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Open vector map - PostGIS feature table on topological level

   Simple feature access:
    - open feature index file
   PostGIS Topology:
    - check if topological schema exists
   
   \param[in,out] Map pointer to Map_info structure

   \return 0 success
   \return -1 error
 */
int V2_open_old_pg(struct Map_info *Map)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    PGresult *res;

    G_debug(3, "V2_open_old_pg(): name = %s mapset = %s", Map->name,
            Map->mapset);

    pg_info = &(Map->fInfo.pg);
    
    if (pg_info->toposchema_name) {
        char stmt[DB_SQL_MAX];

        /* get topo schema id */
        sprintf(stmt, "SELECT id FROM topology.topology WHERE name = '%s'",
                pg_info->toposchema_name);
        res = PQexec(pg_info->conn, stmt);
        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
            G_warning("%s\n%s", _("Topology schema not found."),
                      PQresultErrorMessage(res));
            if (res)
                PQclear(res);
            return -1;
        }
        pg_info->toposchema_id = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
    }
    else {
        /* fidx file needed only for simple features access */
        if (Vect_open_fidx(Map, &(pg_info->offset)) != 0) {
            G_warning(_("Unable to open feature index file for vector map <%s>"),
                      Vect_get_full_name(Map));
            G_zero(&(pg_info->offset), sizeof(struct Format_info_offset));
        }
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

   New PostGIS table is created when writing features by
   Vect_wrile_line().

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

    G_debug(2, "V1_open_new_pg(): name = %s with_z = %d", name, with_z);

    pg_info = &(Map->fInfo.pg);
    if (!pg_info->conninfo) {
        G_warning(_("Connection string not defined"));
        return -1;
    }

    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }

    G_debug(1, "V1_open_new_pg(): conninfo='%s' table='%s'",
            pg_info->conninfo, pg_info->table_name);

    /* connect database & get DB name */
    connect_db(pg_info);
    
    /* if schema not defined, use 'public' */
    if (!pg_info->schema_name)
        pg_info->schema_name = G_store("public");

    /* if fid_column not defined, use 'fid' */
    if (!pg_info->fid_column)
        pg_info->fid_column = G_store(GV_PG_FID_COLUMN);

    /* if geom_column not defined, use 'geom' */
    if (!pg_info->geom_column)
        pg_info->geom_column = G_store(GV_PG_GEOMETRY_COLUMN);
    
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
            G_warning(_("PostGIS layer <%s.%s> already exists in database '%s'"),
                      pg_info->schema_name, pg_info->table_name,
                      pg_info->db_name);
            return -1;
        }
    }

    /* no feature in cache */
    pg_info->cache.fid = -1;

    /* unknown feature type */
    pg_info->feature_type = SF_GEOMETRY;

    PQclear(res);

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Read full-topology for PostGIS links
  
  Note: Only 2D topological primitives are currently supported
  
  \param[in,out] Map pointer to Map_info structure
  \param head_only TRUE to read only header
  \param update TRUE to clean GRASS topology in update mode

  \return 0 on success
  \return 1 topology layer does not exist
  \return -1 on error
*/
int Vect__open_topo_pg(struct Map_info *Map, int head_only, int update)
{
#ifdef HAVE_POSTGRES
    int ret;
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;
    
    Map->open = VECT_OPEN_CODE; /* needed by load_plus */

    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    
    /* check for topo schema */
    if (check_topo(pg_info, plus) != 0)
        return 1;
    
    /* free and init plus structure, update spatial and category
     * indices  */
    dig_init_plus(plus);
    plus->Spidx_new = TRUE;        /* force building spatial and category indices */
    plus->update_cidx = TRUE; 
    Map->support_updated = FALSE;  /* don't write support files */

    ret = Vect__load_plus_pg(Map, head_only);

    if (update)
        Vect__clean_grass_db_topo(pg_info);

    plus->cidx_up_to_date = TRUE;  /* category index built from topo */

    return ret;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
/*!
  \brief Get key column for feature table

  \param pg_info pointer to Format_info_pg
   
  \return string buffer with key column name
  \return NULL on missing key column
*/
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

/*!
  \brief Get simple feature type from string

  \param type string

  \return SF type
*/
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
    else
        return SF_GEOMETRY;
    
    G_debug(3, "ftype_from_string(): type='%s' -> %d", type, sf_type);
    
    return sf_type;
}

/*!
  \brief Drop feature table and topology schema if exists

  \param pg_info pointer to Format_info_pg

  \return -1 on error
  \return 0 on success
*/
int drop_table(struct Format_info_pg *pg_info)
{
    int i;
    char stmt[DB_SQL_MAX];
    char *topo_schema;
    
    PGresult *result, *result_drop;
    
    /* check if topology schema exists */
    sprintf(stmt, "SELECT COUNT(*) FROM pg_tables WHERE schemaname = 'topology'");
    if (Vect__execute_get_value_pg(pg_info->conn, stmt) != 0) {
        /* drop topology schema(s) related to the feature table */
        sprintf(stmt, "SELECT t.name FROM topology.layer AS l JOIN "
                "topology.topology AS t ON l.topology_id = t.id "
                "WHERE l.table_name = '%s'", pg_info->table_name);
        G_debug(2, "SQL: %s", stmt);
        
        result = PQexec(pg_info->conn, stmt);
        if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
            G_warning(_("Execution failed: %s"), PQerrorMessage(pg_info->conn));
            PQclear(result);
            return -1;
        }
        for (i = 0; i < PQntuples(result); i++) {
            topo_schema = PQgetvalue(result, i, 0);
            sprintf(stmt, "SELECT topology.DropTopology('%s')",
                    topo_schema);
            G_debug(2, "SQL: %s", stmt);
            
            result_drop = PQexec(pg_info->conn, stmt);
            if (!result_drop || PQresultStatus(result_drop) != PGRES_TUPLES_OK)
                G_warning(_("Execution failed: %s"), PQerrorMessage(pg_info->conn));
            
            G_verbose_message(_("PostGIS topology schema <%s> dropped"),
                              topo_schema);
            PQclear(result_drop);
        }
        PQclear(result);
    }

    /* drop feature table */
    sprintf(stmt, "DROP TABLE \"%s\".\"%s\"",
            pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        return -1;
    }

    return 0;
}

/*!
  \brief Establish PG connection (pg_info->conninfo)

  Check if DB is spatial as defined by PostGIS.

  \param pg_info pointer to Format_info_pg
*/
void connect_db(struct Format_info_pg *pg_info)
{
    char stmt[DB_SQL_MAX];

    /* check if connection string already contains user/passwd */
    if (!strstr(pg_info->conninfo, "user")) {
        char dbname[GNAME_MAX];
        char *p;
        const char *user, *passwd, *host, *port;
        
        dbname[0] = '\0';
        p = strstr(pg_info->conninfo, "dbname");
        if (p) {
            int i;
            p += strlen("dbname") + 1; /* dbname= */
            
            for (i = 0; *p && *p != ' '; i++, p++)
                dbname[i] = *p;
        }
        
        /* try connection settings for given database first, then try
         * any settings defined for pg driver */
        db_get_login2("pg", dbname, &user, &passwd, &host, &port);
        /* any settings defined for pg driver disabled - can cause
           problems when running multiple local/remote db clusters
        if (strlen(dbname) > 0 && !user && !passwd)
            db_get_login2("pg", NULL, &user, &passwd, &host, &port);
        */
        if (user || passwd || host || port) {
            char  conninfo[DB_SQL_MAX];

            sprintf(conninfo, "%s", pg_info->conninfo);
            if (user) {
                strcat(conninfo, " user=");
                strcat(conninfo, user);
            }
            if (passwd) {
                strcat(conninfo, " password=");
                strcat(conninfo, passwd);
            }
            if (host) {
                strcat(conninfo, " host=");
                strcat(conninfo, host);
            }
            if (port) {
                strcat(conninfo, " port=");
                strcat(conninfo, port);
            }
            G_free(pg_info->conninfo);
            pg_info->conninfo = G_store(conninfo);
        }
    }
    
    pg_info->conn = PQconnectdb(pg_info->conninfo);
    G_debug(1, "   PQconnectdb(): %s", pg_info->conninfo);
    if (PQstatus(pg_info->conn) == CONNECTION_BAD)
        G_fatal_error("%s\n%s",
                      _("Connection to PostgreSQL database failed. "
                        "Try to set up username/password by db.login."),
                      PQerrorMessage(pg_info->conn));

    /* get DB name */
    pg_info->db_name = G_store(PQdb(pg_info->conn));
    if (!pg_info->db_name)
        G_warning(_("Unable to get database name"));
    
    sprintf(stmt, "SELECT COUNT(*) FROM pg_tables WHERE tablename = 'spatial_ref_sys'");
    if (Vect__execute_get_value_pg(pg_info->conn, stmt) != 1) {
        PQfinish(pg_info->conn);
        G_fatal_error(_("<%s> is not PostGIS database. DB table 'spatial_ref_sys' not found."),
                      pg_info->db_name ? pg_info->db_name : pg_info->conninfo);
    }

    if (pg_info->toposchema_name) {
        /* check if topology schema exists */
        sprintf(stmt, "SELECT COUNT(*) FROM pg_tables WHERE schemaname = 'topology'");
        if (Vect__execute_get_value_pg(pg_info->conn, stmt) == 0) {
            PQfinish(pg_info->conn);
            G_fatal_error(_("PostGIS Topology extension not found in the database <%s>"),
                          pg_info->db_name);
        }
    }

    /* print notice messages only on verbose level */
    PQsetNoticeProcessor(pg_info->conn, notice_processor, NULL);

}

/*!
  \brief Check for topology schema (pg_info->toposchema_name)

  \param pg_info pointer to Format_info_pg

  \return 0 schema exists
  \return 1 schema doesn't exists
 */
int check_topo(struct Format_info_pg *pg_info, struct Plus_head *plus)
{
    char stmt[DB_SQL_MAX];
    
    PGresult *res;
    
    /* connect database */
    if (!pg_info->conn)
        connect_db(pg_info);
    
    if (pg_info->toposchema_name)
        return 0;
    
    /* check if topology layer/schema exists */
    sprintf(stmt,
            "SELECT t.id,t.name,t.hasz,l.feature_column FROM topology.layer "
            "AS l JOIN topology.topology AS t ON l.topology_id = t.id "
            "WHERE schema_name = '%s' AND table_name = '%s'",
            pg_info->schema_name, pg_info->table_name);
    G_debug(2, "SQL: %s", stmt);
    
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) != 1) {
        G_debug(1, "Topology layers for '%s.%s' not found (%s)",
                pg_info->schema_name, pg_info->table_name,
                PQerrorMessage(pg_info->conn));
        if (res)
            PQclear(res);
        return 1;
    }

    pg_info->toposchema_id   = atoi(PQgetvalue(res, 0, 0));
    pg_info->toposchema_name = G_store(PQgetvalue(res, 0, 1));
    pg_info->topogeom_column = G_store(PQgetvalue(res, 0, 3));

    /* check extra GRASS tables */
    sprintf(stmt, "SELECT COUNT(*) FROM pg_tables WHERE schemaname = '%s' "
            "AND tablename LIKE '%%_grass'", pg_info->toposchema_name);
    if (Vect__execute_get_value_pg(pg_info->conn, stmt) != TOPO_TABLE_NUM)
        pg_info->topo_geo_only = TRUE;
    
    G_debug(1, "PostGIS topology detected: schema = %s column = %s topo_geo_only = %d",
            pg_info->toposchema_name, pg_info->topogeom_column, pg_info->topo_geo_only);
    
    /* check for 3D */
    if (strcmp(PQgetvalue(res, 0, 2), "t") == 0)
        plus->with_z = WITH_Z;
    PQclear(res);
    
    return 0;
}

/*!
  \brief Parse BBOX string
  
  \param value string buffer
  \param[out] bbox pointer to output bound_box struct

  \return 0 on success
  \return -1 on error
*/
int parse_bbox(const char *value, struct bound_box *bbox)
{
    unsigned int i;
    size_t length, prefix_length;
    char **tokens, **tokens_coord, *coord;
    
    if (strlen(value) < 1) {
        G_warning(_("Empty bounding box"));
        return -1;
    }
    
    prefix_length = strlen("box3d(");
    if (G_strncasecmp(value, "box3d(", prefix_length) != 0)
        return -1;
    
    /* strip off "bbox3d(...)" */
    length = strlen(value);
    coord = G_malloc(length - prefix_length);
    for (i = prefix_length; i < length; i++)
        coord[i-prefix_length] = value[i];
    coord[length-prefix_length-1] = '\0';
    
    tokens = G_tokenize(coord, ",");
    G_free(coord);
    
    if (G_number_of_tokens(tokens) != 2) {
        G_free_tokens(tokens);
        return -1;
    }
    
    /* parse bbox LL corner */
    tokens_coord = G_tokenize(tokens[0], " ");
    if (G_number_of_tokens(tokens_coord) != 3) {
        G_free_tokens(tokens);
        G_free_tokens(tokens_coord);
    }
    bbox->W = atof(tokens_coord[0]);
    bbox->S = atof(tokens_coord[1]);
    bbox->B = atof(tokens_coord[2]);
    
    G_free_tokens(tokens_coord);
    
    /* parse bbox UR corner */
    tokens_coord = G_tokenize(tokens[1], " ");
    if (G_number_of_tokens(tokens_coord) != 3) {
        G_free_tokens(tokens);
        G_free_tokens(tokens_coord);
    }
    bbox->E = atof(tokens_coord[0]);
    bbox->N = atof(tokens_coord[1]);
    bbox->T = atof(tokens_coord[2]);
    
    G_free_tokens(tokens_coord);
    G_free_tokens(tokens);
    
    return 0;
}

/*!
  \brief Read P_node structure
  
  See dig_Rd_P_node() for reference.
  
  \param plus pointer to Plus_head structure
  \param n index (starts at 1)
  \param id node id (table "node")
  \param wkb_data geometry data (wkb)
  \param lines_data lines array or NULL
  \param angles_data angles array or NULL
  \param pg_info pointer to Format_info_pg sttucture
  \param geom_only TRUE to read node's geometry 

  \return pointer to new P_node struct
  \return NULL on error
*/
struct P_node *read_p_node(struct Plus_head *plus, int n,
                           int id, const char *wkb_data, const char *lines_data,
                           const char *angles_data, struct Format_info_pg *pg_info,
                           int geom_only)
{
    int i, cnt;
    char **lines, **angles;

    struct P_node *node;
    struct line_pnts *points;
    
    PGresult *res;
    
    /* get lines connected to the node */
    cnt = 0;
    lines = angles = NULL;
    if (!geom_only) {
        if (!lines_data && !angles_data) { /* pg_info->topo_geo_only == TRUE */
            char stmt[DB_SQL_MAX];
            
            sprintf(stmt,
                    "SELECT edge_id,'s' as node,"
                    "ST_Azimuth(ST_StartPoint(geom), ST_PointN(geom, 2)) AS angle"
                    " FROM \"%s\".edge WHERE start_node = %d UNION ALL "
                    "SELECT edge_id,'e' as node,"
                    "ST_Azimuth(ST_EndPoint(geom), ST_PointN(geom, ST_NumPoints(geom) - 1)) AS angle"
                    " FROM \"%s\".edge WHERE end_node = %d"
                    " ORDER BY angle DESC",
                    pg_info->toposchema_name, id,
                    pg_info->toposchema_name, id);
            G_debug(2, "SQL: %s", stmt);
            res = PQexec(pg_info->conn, stmt);
            if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
                G_warning(_("Inconsistency in topology: unable to read node %d"), id);
                if (res)
                    PQclear(res);
                return NULL;
            }
            cnt = PQntuples(res);
        }
        else { /* pg_info->topo_geo_only != TRUE */
            lines  = scan_array(lines_data);
            angles = scan_array(angles_data);
            
            cnt = G_number_of_tokens(lines);
            if (cnt != G_number_of_tokens(angles))
                return NULL; /* 'lines' and 'angles' array must have the same size */
        }
        
        if (cnt == 0) { /* dead */
            plus->Node[n] = NULL;
            return NULL;
        }
    }
    
    node = dig_alloc_node();
    node->n_lines = cnt;
    G_debug(4, "read_p_node(): id = %d, n_lines = %d", id, cnt);
    
    if (!geom_only) {
        if (dig_node_alloc_line(node, node->n_lines) == -1)
            return NULL;
        
        /* lines / angles */
        if (lines) {
            for (i = 0; i < node->n_lines; i++) {
                node->lines[i] = atoi(lines[i]);
                node->angles[i] = atof(angles[i]);
                
                G_debug(5, "\tline = %d angle = %f", node->lines[i],
                        node->angles[i]);
            }
            
            G_free_tokens(lines);
            G_free_tokens(angles);
        }
        else {
            for (i = 0; i < node->n_lines; i++) {
                node->lines[i] = atoi(PQgetvalue(res, i, 0));
                if (strcmp(PQgetvalue(res, i, 1), "s") != 0) {
                    /* end node */
                    node->lines[i] *= -1;
                }
                node->angles[i] = M_PI / 2 - atof(PQgetvalue(res, i, 2));
                /* angles range <-PI; PI> */
                if (node->angles[i] > M_PI)
                    node->angles[i] = node->angles[i] - 2 * M_PI;
                if (node->angles[i] < -1.0 * M_PI)
                    node->angles[i] = node->angles[i] + 2 * M_PI;
                G_debug(5, "\tline = %d angle = %f", node->lines[i],
                        node->angles[i]);
            }
            PQclear(res);
        }
    }
    
    /* get node coordinates */
    if (SF_POINT != Vect__cache_feature_pg(wkb_data, FALSE, FALSE,
                                           &(pg_info->cache), NULL))
        G_warning(_("Inconsistency in topology: node %d - unexpected feature type %d"),
                  n, pg_info->cache.sf_type);
    
    points = pg_info->cache.lines[0];
    node->x = points->x[0];
    node->y = points->y[0];
    if (plus->with_z)
        node->z = points->z[0];
    else
        node->z = 0.0;
    
    /* update spatial index */
    if (plus->Spidx_new)
        dig_spidx_add_node(plus, n, node->x, node->y, node->z);
    
    if (plus->uplist.do_uplist)
        /* collect updated nodes if requested */
        dig_node_add_updated(plus, n);
    
    plus->Node[n] = node;
    
    return node;
}

/*!
  \brief Read P_line structure
  
  See dig_Rd_P_line() for reference.
  
  Supported feature types:
   - GV_POINT
   - GV_LINE
   - GV_BOUNDARY
  
  \param plus pointer to Plus_head structure
  \param n index (starts at 1)
  \param data edge data (id, start/end node, left/right face, ...)
  \param topo_geo_only TRUE for topo-geo-only mode
  \param cache pointer to Format_info_cache structure

  \return pointer to P_line struct
  \return NULL on error
*/
struct P_line *read_p_line(struct Plus_head *plus, int n,
                           const struct line_data *data, int topo_geo_only,
                           struct Format_info_cache *cache)
{
    int tp, cat;
    struct P_line *line;
    
    if (data->start_node == 0 && data->end_node == 0) {
        if (data->left_face == 0)
            tp = GV_POINT;
        else
            tp = GV_CENTROID;
    }
    else if (data->left_face == 0 && data->right_face == 0) {
        tp = GV_LINE;
    }
    else {
        tp = GV_BOUNDARY;
    }
    
    if (tp == 0) { /* dead ??? */
        plus->Line[n] = NULL;
        return NULL;
    }

    line = dig_alloc_line();
    
    /* type & offset ( = id) */
    line->type = tp;
    line->offset = data->id;
    G_debug(4, "read_p_line(): id/offset = %d type = %d", data->id, line->type);
    
    /* topo */
    if (line->type == GV_POINT) {
        line->topo = NULL;
    }
    else {
        line->topo = dig_alloc_topo(line->type);

        if ((line->type & GV_LINES) & (data->start_node < 0 || data->end_node < 0))
            return NULL;
        
        /* lines */
        if (line->type == GV_LINE) {
            struct P_topo_l *topo = (struct P_topo_l *)line->topo;
            
            topo->N1 = data->start_node;
            topo->N2 = data->end_node;
        }
        /* boundaries */
        else if (line->type == GV_BOUNDARY) {
            struct P_topo_b *topo = (struct P_topo_b *)line->topo;
            
            topo->N1    = data->start_node;
            topo->N2    = data->end_node;

            if (topo_geo_only) {
                topo->left = topo->right = 0;
            }
            else {
                topo->left  = data->left_face;
                topo->right = data->right_face;
            }
        }
        /* centroids */
        else if (line->type == GV_CENTROID) {
            struct P_topo_c *topo = (struct P_topo_c *)line->topo;
            
            topo->area = data->left_face;
        }
    }

    Vect__cache_feature_pg(data->wkb_geom, FALSE, tp, cache, NULL);
    cat = cache->lines_cats[cache->lines_num-1] = data->fid > 0 ? data->fid : -1;

    /* update spatial index */
    if (plus->Spidx_new) {
        struct line_pnts *points;
        struct bound_box box;
    
        points = cache->lines[cache->lines_num-1];
        dig_line_box(points, &box);
        dig_spidx_add_line(plus, n, &box);
    }
    
    /* update category index */
    if (plus->update_cidx)
        dig_cidx_add_cat(plus, cat > 0 ? 1 : 0, cat > 0 ? cat : 0, n, tp);
    
    if (plus->uplist.do_uplist) {
        /* collect updated lines if requested */
        dig_line_add_updated(plus, n, line->offset);
    }
    
    plus->Line[n] = line;
    
    return line;
}

/*!
  \brief Read P_area structure
  
  \param plus pointer to Plus_head structure
  \param n index (starts at 1)
  \param lines_data lines array (see P_area struct)
  \param centroid centroid id (see P_area struct)
  \param isles_data lines array (see P_area struct)

  \return pointer to P_area struct
  \return NULL on error
*/
struct P_area *read_p_area(struct Plus_head *plus, int n,
                           const char *lines_data, int centroid, const char *isles_data)
{
    int i;
    int nlines, nisles;
    char **lines, **isles;

    struct P_area *area;

    lines  = scan_array(lines_data);
    nlines = G_number_of_tokens(lines);
    isles  = scan_array(isles_data);
    nisles = G_number_of_tokens(isles);

    if (nlines < 1) {
        G_warning(_("Area %d without boundary detected"), n);
        return NULL;
    }

    G_debug(3, "read_p_area(): n = %d nlines = %d nisles = %d", n, nlines, nisles);
    
    /* allocate area */
    area = dig_alloc_area();
    dig_area_alloc_line(area, nlines);
    dig_area_alloc_isle(area, nisles);

    /* set lines */
    area->n_lines = nlines;
    for (i = 0; i < nlines; i++) {
        area->lines[i] = atoi(lines[i]);
    }

    /* set isles */
    area->n_isles = nisles;
    for (i = 0; i < nisles; i++) {
        area->isles[i] = atoi(isles[i]);
    }

    /* set centroid */
    area->centroid = remap_line(plus, centroid, GV_CENTROID);

    G_free_tokens(lines);
    G_free_tokens(isles);

    plus->Area[n] = area;

    return area;
}

/*!
  \brief Read P_isle structure
  
  \param plus pointer to Plus_head structure
  \param n index (starts at 1)
  \param lines_data lines array (see P_isle struct)
  \param area area id (see P_isle struct)

  \return pointer to P_isle struct
  \return NULL on error
*/
struct P_isle *read_p_isle(struct Plus_head *plus, int n,
                           const char *lines_data, int area)
{
    int i;
    int nlines;
    char **lines;

    struct P_isle *isle;

    lines  = scan_array(lines_data);
    nlines = G_number_of_tokens(lines);

    if (nlines < 1) {
        G_warning(_("Isle %d without boundary detected"), n);
        return NULL;
    }

    G_debug(3, "read_p_isle(): n = %d nlines = %d", n, nlines);
    
    /* allocate isle */
    isle = dig_alloc_isle();
    dig_isle_alloc_line(isle, nlines);
    
    /* set lines */
    isle->n_lines = nlines;
    for (i = 0; i < nlines; i++) {
        isle->lines[i] = atoi(lines[i]);
    }

    /* set area */
    isle->area = area;
    
    G_free_tokens(lines);

    plus->Isle[n] = isle;

    return isle;
}

/*!
  \brief Read topo from PostGIS topology schema -- header info only

  \param[in,out] plus pointer to Plus_head struct

  \return 0 on success
  \return -1 on error
*/
int Vect__load_plus_head(struct Map_info *Map)
{
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct Plus_head *plus;

    PGresult *res;
    
    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    
    plus->off_t_size = -1;
    
    /* get map bounding box
       first try to get info from 'topology.grass' table */
    sprintf(stmt,
            "SELECT %s FROM \"%s\".\"%s\" WHERE %s = %d",
            TOPO_BBOX, TOPO_SCHEMA, TOPO_TABLE, TOPO_ID, pg_info->toposchema_id);
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) != 1) {
	PQclear(res);
	
	/* otherwise try to calculate bbox from TopoGeometry elements */
	sprintf(stmt,
		"SELECT ST_3DExtent(%s) FROM \"%s\".\"%s\"",
		pg_info->topogeom_column, pg_info->schema_name, pg_info->table_name);
	G_debug(2, "SQL: %s", stmt);
	res = PQexec(pg_info->conn, stmt);
	if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
	    PQntuples(res) != 1 || strlen(PQgetvalue(res, 0, 0)) < 1) {
	    G_warning(_("Unable to get map bounding box from topology"));
            PQclear(res);
	    return -1;
	}
    }
    
    if (parse_bbox(PQgetvalue(res, 0, 0), &(plus->box)) != 0) {
        G_warning(_("Unable to parse map bounding box:\n%s"),
                  PQgetvalue(res, 0, 0));
        return -1;
    }
    PQclear(res);
    
    /* get number of topological elements */
    
    /* nodes
       note: isolated nodes are registered in GRASS Topology model */
    sprintf(stmt,
            "SELECT COUNT(DISTINCT node) FROM (SELECT start_node AS node "
            "FROM \"%s\".edge GROUP BY start_node UNION ALL SELECT end_node "
            "AS node FROM \"%s\".edge GROUP BY end_node) AS foo",
            pg_info->toposchema_name, pg_info->toposchema_name);
    plus->n_nodes = Vect__execute_get_value_pg(pg_info->conn, stmt);
    if (!pg_info->topo_geo_only) {
        int n_nodes;
        
        /* check nodes consistency */
        sprintf(stmt, "SELECT COUNT(*) FROM \"%s\".%s",
                pg_info->toposchema_name, TOPO_TABLE_NODE);
        n_nodes = Vect__execute_get_value_pg(pg_info->conn, stmt);
        if (n_nodes != plus->n_nodes) {
            G_warning(_("Different number of nodes detected (%d, %d)"),
                      plus->n_nodes, n_nodes);
            return -1;
        }
    }
    G_debug(3, "Vect_open_topo_pg(): n_nodes=%d", plus->n_nodes);
    
    /* lines (edges in PostGIS Topology model) */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".edge",
            pg_info->toposchema_name);
    /* + isolated nodes as points
       + centroids */
    plus->n_lines = Vect__execute_get_value_pg(pg_info->conn, stmt); 
    
    /* areas (faces with face_id > 0 in PostGIS Topology model) */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".face WHERE face_id > 0",
            pg_info->toposchema_name);
    plus->n_areas = Vect__execute_get_value_pg(pg_info->conn, stmt);
    if (!pg_info->topo_geo_only) {
        int n_areas;
        
        /* check areas consistency */
        sprintf(stmt, "SELECT COUNT(*) FROM \"%s\".%s",
                pg_info->toposchema_name, TOPO_TABLE_AREA);
        n_areas = Vect__execute_get_value_pg(pg_info->conn, stmt);
        if (n_areas != plus->n_areas) {
            G_warning(_("Different number of areas detected (%d, %d)"),
                      plus->n_areas, n_areas);
            return -1;
        }
    }
    G_debug(3, "Vect_open_topo_pg(): n_areas=%d", plus->n_areas);

    /* isles (faces with face_id <=0 in PostGIS Topology model)
       note: universal face is represented in GRASS Topology model as isle (area=0)
    */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".face WHERE face_id < 0",
            pg_info->toposchema_name);
    plus->n_isles = Vect__execute_get_value_pg(pg_info->conn, stmt);
    if (!pg_info->topo_geo_only) {
        int n_isles;
        
        /* check areas consistency */
        sprintf(stmt, "SELECT COUNT(*) FROM \"%s\".%s",
                pg_info->toposchema_name, TOPO_TABLE_ISLE);
        n_isles = Vect__execute_get_value_pg(pg_info->conn, stmt);
        if (n_isles != plus->n_isles) {
            G_warning(_("Different number of areas detected (%d, %d)"),
                      plus->n_isles, n_isles);
            return -1;
        }
    }
    G_debug(3, "Vect_open_topo_pg(): n_isles=%d", plus->n_isles);
    
    /* number of features according the type */

    /* points */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".node WHERE containing_face "
            "IS NULL AND node_id NOT IN "
            "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge "
            "GROUP BY start_node UNION ALL SELECT end_node AS node FROM "
            "\"%s\".edge GROUP BY end_node) AS foo)",
            pg_info->toposchema_name, pg_info->toposchema_name,
            pg_info->toposchema_name);
    plus->n_plines = Vect__execute_get_value_pg(pg_info->conn, stmt);
    G_debug(3, "Vect_open_topo_pg(): n_plines=%d", plus->n_plines);
    
    /* lines */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".edge WHERE "
            "left_face = 0 AND right_face = 0",
            pg_info->toposchema_name);
    plus->n_llines = Vect__execute_get_value_pg(pg_info->conn, stmt);
    G_debug(3, "Vect_open_topo_pg(): n_llines=%d", plus->n_llines);

    /* boundaries */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".edge WHERE "
            "left_face != 0 OR right_face != 0",
            pg_info->toposchema_name);
    plus->n_blines = Vect__execute_get_value_pg(pg_info->conn, stmt);
    G_debug(3, "Vect_open_topo_pg(): n_blines=%d", plus->n_blines);

    /* centroids */
    sprintf(stmt,
            "SELECT COUNT(*) FROM \"%s\".node WHERE containing_face "
            "IS NOT NULL AND node_id NOT IN "
            "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge "
            "GROUP BY start_node UNION ALL SELECT end_node AS node FROM "
            "\"%s\".edge GROUP BY end_node) AS foo)",
            pg_info->toposchema_name, pg_info->toposchema_name,
            pg_info->toposchema_name);
    plus->n_clines = Vect__execute_get_value_pg(pg_info->conn, stmt);
    G_debug(3, "Vect_open_topo_pg(): n_clines=%d", plus->n_clines);

    /* update number of lines - add points and centroids */
    plus->n_lines += plus->n_plines + plus->n_clines;
    G_debug(3, "Vect_open_topo_pg(): n_lines=%d", plus->n_lines);

    return 0;
}

/*!
  \brief Read topo info from PostGIS topology schema

  \param pg_info pointer to Format_info_pg
  \param[in,out] plus pointer to Plus_head struct
  \param head_only TRUE to read only header info
  
  \return 0 on success
  \return -1 on error
*/
int Vect__load_plus_pg(struct Map_info *Map, int head_only)
{
    int i, side, line;
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct Plus_head *plus;
    struct P_line *Line;
    struct line_pnts *Points;
    struct ilist *List;
    struct bound_box box;
    
    PGresult *res;
  
    pg_info = &(Map->fInfo.pg);
    plus = &(Map->plus);

    if (Vect__load_plus_head(Map) != 0)
        return -1;

    if (head_only)
        return 0;
    
    Points = Vect_new_line_struct();
    List = Vect_new_list();
    
    /* read nodes (GRASS Topo)
       note: standalone nodes (ie. points/centroids) are ignored
    */
    Vect__load_map_nodes_pg(Map, FALSE);

    /* read lines (GRASS Topo)
       - standalone nodes -> points|centroids
       - edges -> lines/boundaries
    */
    Vect__free_cache(&(pg_info->cache));
    pg_info->cache.ctype = CACHE_MAP;

    Vect__load_map_lines_pg(Map);
    
    /* build areas */
    if (pg_info->topo_geo_only) {
        /* build areas for boundaries 
           reset values -> build from scratch */
        plus->n_areas = plus->n_isles = 0;
        for (line = 1; line <= plus->n_lines; line++) {
            Line = plus->Line[line]; /* centroids: Line is NULL */
            if (!Line || Line->type != GV_BOUNDARY)
                continue;
            
            for (i = 0; i < 2; i++) { /* for both sides build an area/isle */
                side = i == 0 ? GV_LEFT : GV_RIGHT;
                
                G_debug(3, "Build area for line = %d, side = %d",
                        i, side);
                Vect_build_line_area(Map, line, side);
            }
        }
    }
    else {
        int cat;
        
        /* read areas from 'area_grass' table */
        sprintf(stmt,
                "SELECT area_id,lines,centroid,isles FROM \"%s\".%s ORDER BY area_id",
                pg_info->toposchema_name, TOPO_TABLE_AREA);
        G_debug(2, "SQL: %s", stmt);
        
        res = PQexec(pg_info->conn, stmt);
        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
            plus->n_areas != PQntuples(res)) {
            if (res)
                PQclear(res);
            return -1;
        }

        
        dig_alloc_areas(plus, plus->n_areas);
        G_zero(plus->Area, sizeof(struct P_area *) * (plus->n_areas + 1)); /* index starts at 1 */
        G_debug(3, "Vect_open_topo_pg(): n_areas=%d", plus->n_areas);
        
        for (i = 0; i < plus->n_areas; i++) {
            read_p_area(plus, i + 1, (char *)PQgetvalue(res, i, 1),
                        atoi(PQgetvalue(res, i, 2)), (char *)PQgetvalue(res, i, 3));
            
            if (plus->Spidx_new) {
                /* update spatial index */
                Vect_get_area_points(Map, i+1, Points);
                dig_line_box(Points, &box);
                dig_spidx_add_area(&(Map->plus), i+1, &box);
            }

            if (plus->update_cidx) {
                /* update category index */
                cat = pg_info->cache.lines_cats[plus->Area[i+1]->centroid-1];
                dig_cidx_add_cat(plus, cat > 0 ? 1 : 0, cat > 0 ? cat : 0, i+1, GV_AREA);
            }
        }
        PQclear(res);
    }
    plus->built = GV_BUILD_AREAS;

    /* attach isles */
    if (pg_info->topo_geo_only) {
        plus->n_isles = 0; /* reset isles */
        G_warning(_("To be implemented: isles not attached in Topo-Geo-only mode"));
    }
    else {
        /* read isles from 'isle_grass' table */
        sprintf(stmt,
                "SELECT isle_id,lines,area FROM \"%s\".%s ORDER BY isle_id",
                pg_info->toposchema_name, TOPO_TABLE_ISLE);
        G_debug(2, "SQL: %s", stmt);
        
        res = PQexec(pg_info->conn, stmt);
        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
            plus->n_isles != PQntuples(res)) {
            if (res)
                PQclear(res);
            return -1;
        }
        
        dig_alloc_isles(plus, plus->n_isles);
        G_zero(plus->Isle, sizeof(struct P_isle *) * (plus->n_isles + 1)); /* index starts at 1 */
        G_debug(3, "Vect_open_topo_pg(): n_isles=%d", plus->n_isles);
        
        for (i = 0; i < plus->n_isles; i++) {
            read_p_isle(plus, i + 1, (char *)PQgetvalue(res, i, 1),
                        atoi(PQgetvalue(res, i, 2)));

            if (plus->Spidx_new) {
                /* update spatial index */
                Vect_get_isle_points(Map, i+1, Points);
                dig_line_box(Points, &box);
                dig_spidx_add_isle(&(Map->plus), i+1, &box);
            }
        }
        PQclear(res);
    }
    plus->built = GV_BUILD_ATTACH_ISLES;
    
    /* attach centroids */
    if (pg_info->topo_geo_only && plus->n_areas > 0) {
        int area;
        struct P_area   *Area;
        struct P_topo_c *topo;
        
        for (line = 1; line <= plus->n_lines; line++) {
            Line = plus->Line[line];
            if (Line->type != GV_CENTROID)
                continue;
            
            Vect_read_line(Map, Points, NULL, line);
            area = Vect_find_area(Map, Points->x[0], Points->y[0]);
            topo = (struct P_topo_c *)Line->topo;
            topo->area = area;
            Area = plus->Area[topo->area];
            Area->centroid = Line->offset;
        }
    }
    plus->built = GV_BUILD_CENTROIDS;
    
    /* done */
    plus->built = GV_BUILD_ALL;
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_list(List);
    
    return 0;
}

/*!
  \brief Read nodes from DB 
  
  \param Map pointer to Map_info struct
  \param geom_only read only node's geometry

  \return number of nodes
*/
int Vect__load_map_nodes_pg(struct Map_info *Map, int geom_only)
{
    int i, id, n_nodes;
    char stmt[DB_SQL_MAX];
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;
    struct Format_info_offset *offset;

    PGresult *res;

    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    offset = &(pg_info->offset);

    if (pg_info->topo_geo_only || geom_only) 
        sprintf(stmt,
                "SELECT node_id,geom FROM \"%s\".node WHERE node_id IN "
                "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge "
                "GROUP BY start_node UNION ALL SELECT end_node AS node FROM "
                "\"%s\".edge GROUP BY end_node) AS foo) ORDER BY node_id",
                pg_info->toposchema_name, pg_info->toposchema_name,
                pg_info->toposchema_name);
    else
        sprintf(stmt, "SELECT node.node_id,geom,lines,angles FROM \"%s\".node AS node "
                "JOIN \"%s\".%s AS node_grass ON node.node_id = node_grass.node_id "
                "ORDER BY node_id", pg_info->toposchema_name, pg_info->toposchema_name,
                TOPO_TABLE_NODE);
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        (!geom_only && PQntuples(res) != plus->n_nodes)) {
        G_warning(_("Inconsistency in topology: number of "
                    "nodes %d (should be %d)"),
                  PQntuples(res), plus->n_nodes);
        if (res)
            PQclear(res);
        return -1;
    }

    n_nodes = PQntuples(res);
    G_debug(3, "load_plus(): n_nodes = %d", n_nodes);
    dig_alloc_nodes(plus, n_nodes);
    offset->array = (int *) G_malloc (sizeof(int) * n_nodes);
    offset->array_num = offset->array_alloc = n_nodes;
    for (i = 0; i < n_nodes; i++) {
        G_debug(5, "node: %d", i);
        id = atoi(PQgetvalue(res, i, 0));
        read_p_node(plus, i + 1, /* node index starts at 1 */
                    id, (const char *) PQgetvalue(res, i, 1),
                    !pg_info->topo_geo_only ? (const char *) PQgetvalue(res, i, 2) : NULL,
                    !pg_info->topo_geo_only ? (const char *) PQgetvalue(res, i, 3) : NULL,
                    pg_info, geom_only);
        /* update offset */
        offset->array[i] = id;
    }
    PQclear(res);

    return n_nodes;
}

/*!
  \brief Read features from DB 
  
  \param Map pointer to Map_info struct

  \return number of features
*/
int Vect__load_map_lines_pg(struct Map_info *Map)
{
    int i, id, ntuples;
    
    char stmt[DB_SQL_MAX];
    
    struct Plus_head *plus;
    struct Format_info_pg *pg_info;
    struct line_data line_data;
    struct Format_info_offset *offset;
    
    PGresult *res;
    
    plus = &(Map->plus);
    pg_info = &(Map->fInfo.pg);
    offset = &(pg_info->offset);
 
    dig_alloc_lines(plus, plus->n_lines); 
    G_zero(plus->Line, sizeof(struct P_line *) * (plus->n_lines + 1)); /* index starts at 1 */

    /* read PostGIS Topo standalone nodes (containing_face is null)
       -> points
    */
    if (pg_info->topo_geo_only)
        sprintf(stmt,
                "SELECT tt.node_id,tt.geom,ft.%s FROM \"%s\".node AS tt "
                "LEFT JOIN \"%s\".\"%s\" AS ft ON "
                "(%s).type = 1 AND (%s).id = node_id WHERE containing_face "
                "IS NULL AND node_id NOT IN "
                "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge "
                "GROUP BY start_node UNION ALL SELECT end_node AS node FROM "
                "\"%s\".edge GROUP BY end_node) AS foo) ORDER BY node_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->schema_name, pg_info->table_name,
                pg_info->topogeom_column, pg_info->topogeom_column, pg_info->toposchema_name,
                pg_info->toposchema_name);
    else
        sprintf(stmt,
                "SELECT tt.node_id,tt.geom,ft.%s "
                "FROM \"%s\".node AS tt LEFT JOIN \"%s\".\"%s\" AS ft ON "
                "(%s).type = 1 AND (%s).id = node_id WHERE node_id NOT IN "
                "(SELECT node_id FROM \"%s\".%s) AND containing_face IS NULL ORDER BY node_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->schema_name, pg_info->table_name,
                pg_info->topogeom_column, pg_info->topogeom_column,
                pg_info->toposchema_name, TOPO_TABLE_NODE);
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) > plus->n_plines) {
        G_warning(_("Inconsistency in topology: number of "
                    "points %d (should be %d)"),
                  PQntuples(res), plus->n_plines);
        if (res)
            PQclear(res);
        return -1;
    }

    ntuples = PQntuples(res); /* plus->n_plines */
    G_zero(&line_data, sizeof(struct line_data));
    for (i = 0; i < ntuples; i++) {
        /* process standalone nodes (PostGIS Topo) */
        line_data.id = atoi(PQgetvalue(res, i, 0));
        line_data.wkb_geom = (char *) PQgetvalue(res, i, 1);
        line_data.fid = atoi(PQgetvalue(res, i, 2)); /* feature id */

        read_p_line(plus, i + 1, &line_data, pg_info->topo_geo_only, &(pg_info->cache));
    }
    PQclear(res);
    
    /* read PostGIS Topo edges
       -> lines
       -> boundaries
    */
    if (pg_info->topo_geo_only)
        sprintf(stmt, 
                "SELECT edge_id,start_node,end_node,left_face,right_face AS right_area,tt.geom,ft.%s "
                "FROM \"%s\".edge AS tt LEFT JOIN \"%s\".\"%s\" AS ft ON (%s).type = 2 AND "
                "(%s).id = edge_id ORDER BY edge_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->schema_name, pg_info->table_name,
                pg_info->topogeom_column, pg_info->topogeom_column);
    else
        sprintf(stmt, 
                "SELECT edge_id,start_node,end_node,left_area,right_area,tt.geom,ft.%s "
                "FROM \"%s\".edge AS tt LEFT JOIN \"%s\".\"%s\" ON "
                "edge_id = line_id LEFT JOIN \"%s\".\"%s\" AS ft ON (%s).type = 2 AND "
                "(%s).id = edge_id ORDER BY edge_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->toposchema_name, TOPO_TABLE_LINE,
                pg_info->schema_name, pg_info->table_name, pg_info->topogeom_column,
                pg_info->topogeom_column);
        
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) > plus->n_lines) {
        G_warning(_("Inconsistency in topology: number of "
                    "lines %d (should be %d)"),
                  PQntuples(res), plus->n_lines);
        if (res)
            PQclear(res);
        return -1;
    }

    /* process edges (PostGIS Topo) */
    ntuples = PQntuples(res);
    for (i = 0; i < ntuples; i++) {
        line_data.id         = atoi(PQgetvalue(res, i, 0));
        line_data.start_node = remap_node(offset, atoi(PQgetvalue(res, i, 1)));
        line_data.end_node   = remap_node(offset, atoi(PQgetvalue(res, i, 2)));
        line_data.left_face  = atoi(PQgetvalue(res, i, 3));
        line_data.right_face = atoi(PQgetvalue(res, i, 4));
        line_data.wkb_geom   = (char *) PQgetvalue(res, i, 5);
        line_data.fid        = atoi(PQgetvalue(res, i, 6)); /* feature id */

        id = plus->n_plines + i + 1; /* points already registered */
        read_p_line(plus, id, &line_data, pg_info->topo_geo_only, &(pg_info->cache));
    }
    PQclear(res);

    /* read PostGIS Topo standalone nodes (containing_face is not null)
       -> centroids
    */
    if (pg_info->topo_geo_only)
        sprintf(stmt,
                "SELECT node_id,tt.geom,containing_face,ft.%s FROM "
                "\"%s\".node AS tt LEFT JOIN \"%s\".\"%s\" AS ft ON "
                "(%s).type = 3 AND (%s).id = containing_face WHERE containing_face "
                "IS NOT NULL AND node_id NOT IN "
                "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge "
                "GROUP BY start_node UNION ALL SELECT end_node AS node FROM "
                "\"%s\".edge GROUP BY end_node) AS foo) ORDER BY node_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->schema_name, pg_info->table_name,
                pg_info->topogeom_column, pg_info->topogeom_column,
                pg_info->toposchema_name,
                pg_info->toposchema_name);
    else
        sprintf(stmt,
                "SELECT tt.node_id,tt.geom,containing_face,ft.%s FROM "
                "\"%s\".node AS tt LEFT JOIN \"%s\".\"%s\" AS ft ON "
                "(%s).type = 3 AND (%s).id = containing_face WHERE "
                "node_id NOT IN (SELECT node_id FROM \"%s\".%s) AND containing_face "
                "IS NOT NULL ORDER BY node_id",
                pg_info->fid_column, pg_info->toposchema_name, pg_info->schema_name, pg_info->table_name,
                pg_info->topogeom_column, pg_info->topogeom_column,
                pg_info->toposchema_name, TOPO_TABLE_NODE);
    G_debug(2, "SQL: %s", stmt);
    res = PQexec(pg_info->conn, stmt);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK ||
        PQntuples(res) != plus->n_clines) {
        G_warning(_("Inconsistency in topology: number of "
                    "centroids %d (should be %d)"),
                  PQntuples(res), plus->n_clines);
        if (res)
            PQclear(res);
        return -1;
    }
    
    G_zero(&line_data, sizeof(struct line_data));
    id = plus->n_plines + plus->n_llines + plus->n_blines + 1;
    for (i = 0; i < plus->n_clines; i++) {
        line_data.id = atoi(PQgetvalue(res, i, 0)); 
        line_data.wkb_geom = (char *)PQgetvalue(res, i, 1);
        line_data.left_face = atoi(PQgetvalue(res, i, 2)); /* face id */
        line_data.fid = atoi(PQgetvalue(res, i, 3)); /* feature id */
        /* area id and face id can be different */
        
        read_p_line(plus, id + i, &line_data, pg_info->topo_geo_only, &(pg_info->cache));
    }
    PQclear(res);

    plus->built = GV_BUILD_BASE;
    
    return plus->n_lines;
}

/*
  \brief PostgreSQL notice processor

  Print out NOTICE message only on verbose level
*/
void notice_processor(void *arg, const char *message)
{
    if (G_verbose() > G_verbose_std()) {
        fprintf(stderr, "%s", message);
    }
}

/*!
  \brief Scan string array

  Creates tokens based on string array, eg. '{1, 2, 3}' become
  [1,2,3].

  Allocated tokes should be freed by G_free_tokens().

  \param sArray string array

  \return tokens
*/
char **scan_array(const char *sarray)
{
    char *buf, **tokens;
    int i, len;
    
    /* remove '{}' */
    len = strlen(sarray) - 1; /* skip '}' */
    buf = (char *)G_malloc(len);
    
    for (i = 1; i < len; i++)
        buf[i-1] = sarray[i];
    buf[len-1] = '\0';
    
    tokens = G_tokenize(buf, ",");
    G_free(buf);
    
    return tokens;
}

/*!
  \brief Get node id from offset

  \param offset pointer to Format_info_offset struct
  \param node node to find

  \return node id
  \return -1 not found
*/
int remap_node(const struct Format_info_offset *offset, int node)
{
    /* probably not needed
    int i;
    for (i = node-1; i < offset->array_num; i++) {
        if (offset->array[i] == node)
            return i + 1; 
    }
    
    return -1;
    */

    return offset->array[node-1];
}

/*!
  \brief Get line id from offset

  \param plus pointer to Plus_head struct
  \param offset line offset
  \param type line type

  \return line id
  \return -1 not found
*/
int remap_line(const struct Plus_head* plus, off_t offset, int type)
{
    int i;
    
    struct P_line *Line;
    
    for (i = (int) offset; i <= plus->n_lines; i++) {
        Line = plus->Line[i];
        
        if (!Line || Line->type != type)
            continue;
        
        if ((int) Line->offset == offset)
            return i;
    }

    return -1;
}
#endif
