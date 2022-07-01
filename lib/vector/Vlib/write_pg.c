/*!
   \file lib/vector/Vlib/write_pg.c

   \brief Vector library - write vector feature (PostGIS format)

   Higher level functions for reading/writing/manipulating vectors.

   Write subroutine inspired by OGR PostgreSQL driver.

   \todo PostGIS version of V2__delete_area_cats_from_cidx_nat()
   \todo function to delete corresponding entry in fidx
   \todo PostGIS version of V2__add_area_cats_to_cidx_nat

   (C) 2012-2014 by Martin Landa, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <string.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

#define WKBSRIDFLAG 0x20000000

#define TOPOGEOM_COLUMN "topo"

/*! Use SQL statements from PostGIS Topology extension (this options
  is quite slow. By default are used simple SQL statements (INSERT, UPDATE)
*/
#define USE_TOPO_STMT 0

static int create_table(struct Format_info_pg *);
static int check_schema(const struct Format_info_pg *);
static int create_topo_schema(struct Format_info_pg *, int);
static int create_pg_layer(struct Map_info *, int);
static char *get_sftype(SF_FeatureType);
static off_t write_line_sf(struct Map_info *, int,
                           const struct line_pnts **, int,
                           const struct line_cats *);
static off_t write_line_tp(struct Map_info *, int, int,
                           const struct line_pnts *,
                           const struct line_cats *);
static char *binary_to_hex(int, const unsigned char *);
static unsigned char *point_to_wkb(int, const struct line_pnts *, int, int *);
static unsigned char *linestring_to_wkb(int, const struct line_pnts *,
                                        int, int *);
static unsigned char *polygon_to_wkb(int, const struct line_pnts **, int,
                                     int, int *);
static char *line_to_wkb(struct Format_info_pg *, const struct line_pnts **,
                         int, int, int);
static int write_feature(struct Map_info *, int, int,
                         const struct line_pnts **, int, int);
static char *build_insert_stmt(const struct Format_info_pg *, const char *, int, int);
static int insert_topo_element(struct Map_info *, int, int, const char *);
static int type_to_topogeom(const struct Format_info_pg *);
static int update_next_edge(struct Map_info*, int, int);
static int delete_face(const struct Map_info *, int);
static int update_topo_edge(struct Map_info *, int);
static int update_topo_face(struct Map_info *, int);
static int add_line_to_topo_pg(struct Map_info *, off_t, int, const struct line_pnts *);
static int delete_line_from_topo_pg(struct Map_info *, int, int, const struct line_pnts *);
static int set_constraint_to_deferrable(struct Format_info_pg *, const char *, const char *,
                                        const char *, const char *, const char *);
static dbDriver *open_db(struct Format_info_pg *);
#endif

static struct line_pnts *Points;

/*!
   \brief Writes feature on level 1 (PostGIS interface)

   Notes for simple feature access:
   - centroids are not supported in PostGIS, pseudotopo holds virtual
   centroids
   - boundaries are not supported in PostGIS, pseudotopo treats polygons
   as boundaries

   Notes for PostGIS Topology access:
   - centroids are stored as isolated nodes
   - boundaries are stored as edges

   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts structure (feature geometry) 
   \param cats pointer to line_cats structure (feature categories)

   \return feature offset into file
   \return -1 on error
 */
off_t V1_write_line_pg(struct Map_info *Map, int type,
                       const struct line_pnts *points,
                       const struct line_cats *cats)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (pg_info->feature_type == SF_GEOMETRY) {
        /* create PostGIS table if doesn't exist */
        if (create_pg_layer(Map, type) < 0)
            return -1;
    }

    if (!points)
        return 0;
    
    if (!pg_info->toposchema_name) { /* simple features access */
        return write_line_sf(Map, type, &points, 1, cats);
    }

    /* PostGIS Topology access */
    return write_line_tp(Map, type, FALSE, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Writes feature on topological level (PostGIS interface)

   Calls V2_write_line_sfa() for simple features access.
   
   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts structure (feature geometry) 
   \param cats pointer to line_cats structure (feature categories)

   \return feature offset into file
   \return -1 on error
 */
off_t V2_write_line_pg(struct Map_info *Map, int type,
                       const struct line_pnts *points,
                       const struct line_cats *cats)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (!pg_info->toposchema_name) { /* pseudo-topology */
        return V2_write_line_sfa(Map, type, points, cats);
    }

    /* PostGIS Topology */
    return write_line_tp(Map, type, FALSE, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Rewrites feature at the given offset (level 1) (PostGIS interface, internal use only)

   Only for simple feature access. PostGIS Topology requires level 2.
   
   \todo Use UPDATE statement ?
   
   \param Map pointer to Map_info structure
   \param offset feature offset
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points feature geometry
   \param cats feature categories

   \return feature offset (rewritten feature)
   \return -1 on error
 */
off_t V1_rewrite_line_pg(struct Map_info * Map,
                         off_t offset, int type,
                         const struct line_pnts * points,
                         const struct line_cats * cats)
{
    G_debug(3, "V1_rewrite_line_pg(): type=%d offset=%" PRI_OFF_T,
            type, offset);
#ifdef HAVE_POSTGRES
    if (type != V1_read_line_pg(Map, NULL, NULL, offset)) {
        G_warning(_("Unable to rewrite feature (incompatible feature types)"));
        return -1;
    }

    /* delete old */
    V1_delete_line_pg(Map, offset);

    return V1_write_line_pg(Map, type, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Rewrites feature at topological level (PostGIS interface, internal use only)

  Note: Topology must be built at level >= GV_BUILD_BASE
   
  \todo Handle also categories
  \todo Store original geometry in tmp table for restore
  
  \param Map pointer to Map_info structure
  \param line feature id
  \param type feature type  (GV_POINT, GV_LINE, ...)
  \param points feature geometry
  \param cats feature categories
  
  \return offset where feature was rewritten
  \return -1 on error
*/
off_t V2_rewrite_line_pg(struct Map_info *Map, off_t line, int type,
                         const struct line_pnts *points, const struct line_cats *cats)
{
    G_debug(3, "V2_rewrite_line_pg(): line=%d type=%d",
            (int)line, type);
#ifdef HAVE_POSTGRES
    const char *schema_name, *table_name, *keycolumn;
    char *stmt, *geom_data;
    
    struct Format_info_pg *pg_info;
    struct P_line *Line;
    off_t offset;

    geom_data = NULL;
    stmt = NULL;
    pg_info = &(Map->fInfo.pg);
  
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), (int)line);
        return -1;
    }
  
    Line = Map->plus.Line[line];
    if (Line == NULL) {
        G_warning(_("Attempt to access dead feature %d"), (int)line);
        return -1;
    }
    offset = Line->offset;

    if (!(Map->plus.update_cidx)) {
	Map->plus.cidx_up_to_date = FALSE; /* category index will be outdated */
    }

    if (!Points)
        Points = Vect_new_line_struct();
    
    if (type != V2_read_line_pg(Map, Points, NULL, line)) {
	G_warning(_("Unable to rewrite feature (incompatible feature types)"));
	return -1;
    }

    /* remove line from topology */
    if (0 != delete_line_from_topo_pg(Map, line, type, Points))
        return -1;

    if (pg_info->toposchema_name) { /* PostGIS Topology */
        schema_name = pg_info->toposchema_name;
        if (type & GV_POINTS) {
            table_name = keycolumn = "node";
        }
        else {
            table_name = "edge_data";
            keycolumn = "edge";
        }
    }
    else { /* simple features access */
        schema_name = pg_info->schema_name;
        table_name  = pg_info->table_name;
        keycolumn   = pg_info->fid_column;
    }
    
    geom_data = line_to_wkb(pg_info, &points, 1, type, Map->head.with_z);
    G_asprintf(&stmt, "UPDATE \"%s\".\"%s\" SET geom = '%s'::GEOMETRY WHERE %s_id = %" PRI_OFF_T,
               schema_name, table_name, geom_data, keycolumn, line);
    G_free(geom_data);

    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        G_warning(_("Unable to rewrite feature %d"), (int)line);
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update topology
       note: offset is not changed */
    return add_line_to_topo_pg(Map, offset, type, points);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Deletes feature at the given offset (level 1)

   Only for simple feature access. PostGIS Topology requires level 2.

   \param Map pointer Map_info structure
   \param offset feature offset

   \return  0 on success
   \return -1 on error
 */
int V1_delete_line_pg(struct Map_info *Map, off_t offset)
{
#ifdef HAVE_POSTGRES
    long fid;
    char stmt[DB_SQL_MAX];

    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    if (!pg_info->conn || !pg_info->table_name) {
        G_warning(_("No connection defined"));
        return -1;
    }

    if (offset >= pg_info->offset.array_num) {
        G_warning(_("Invalid offset (%" PRI_OFF_T ")"), offset);
        return -1;
    }

    fid = pg_info->offset.array[offset];

    G_debug(3, "V1_delete_line_pg(): offset = %lu -> fid = %ld",
            (unsigned long)offset, fid);

    if (!pg_info->inTransaction) {
        /* start transaction */
        pg_info->inTransaction = TRUE;
        if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1)
            return -1;
    }

    sprintf(stmt, "DELETE FROM %s WHERE %s = %ld",
            pg_info->table_name, pg_info->fid_column, fid);
    G_debug(3, "SQL: %s", stmt);

    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        G_warning(_("Unable to delete feature"));
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
  \brief Deletes feature on topological level (PostGIS interface)

  Note: Topology must be built at level >= GV_BUILD_BASE
  
  Calls V2_delete_line_sfa() for simple feature access.
  
  \param Map pointer to Map_info structure
  \param line feature id to be deleted
  
  \return 0 on success
  \return -1 on error
*/
int V2_delete_line_pg(struct Map_info *Map, off_t line)
{
#ifdef HAVE_POSTGRES
    int ret;
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), (int)line);
        return -1;
    }
    
    if (!pg_info->toposchema_name) { /* pseudo-topology */
        return V2_delete_line_sfa(Map, line);
    }
    else {                          /* PostGIS topology */
        int type;
        char stmt[DB_SQL_MAX];
        const char *table_name, *keycolumn;
        
        struct P_line *Line;
        
        if (line < 1 || line > Map->plus.n_lines) {
            G_warning(_("Attempt to access feature with invalid id (%d)"), (int)line);
            return -1;
        }
        
        Line = Map->plus.Line[line];
        if (!Line) {
            G_warning(_("Attempt to access dead feature %d"), (int)line);
            return -1;
        }

	if (!(Map->plus.update_cidx)) {
	    Map->plus.cidx_up_to_date = FALSE; /* category index will be outdated */
	}

        Vect__execute_pg(pg_info->conn, "BEGIN");
   
        if (Line->type & GV_POINTS) {
            table_name = keycolumn = "node";
        }
        else {
            table_name = "edge_data";
            keycolumn = "edge";

            /* first remove references to this edge */
            /* (1) left next edge */
            sprintf(stmt, "UPDATE \"%s\".\"%s\" SET abs_next_left_edge = edge_id, "
                    "next_left_edge = -edge_id WHERE abs_next_left_edge = %d",
                    pg_info->toposchema_name, table_name, (int)Line->offset);
            if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }

            /* (2) right next edge */
            sprintf(stmt, "UPDATE \"%s\".\"%s\" SET abs_next_right_edge = edge_id, "
                    "next_right_edge = edge_id WHERE abs_next_right_edge = %d",
                    pg_info->toposchema_name, table_name, (int)Line->offset);
            if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }

        }
       
        /* read the line */    
        if (!Points)
            Points = Vect_new_line_struct();
        
        type = V2_read_line_pg(Map, Points, NULL, line);
        if (type < 0)
            return -1;

        /* delete record from topology table */
        sprintf(stmt, "DELETE FROM \"%s\".\"%s\" WHERE %s_id = %d",
                pg_info->toposchema_name, table_name, keycolumn, (int)Line->offset);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to delete feature (%s) %d"), keycolumn,
                      (int)line);
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        if (pg_info->cache.ctype == CACHE_MAP) {
            /* delete from cache */
            
            Vect_destroy_line_struct(pg_info->cache.lines[line-1]);
            pg_info->cache.lines[line-1] = NULL;
            pg_info->cache.lines_types[line-1] = 0;
            pg_info->cache.lines_cats[line-1] = 0;
        }
        
        /* update topology */
        ret = delete_line_from_topo_pg(Map, line, type, Points);

        if (ret == 0)
            Vect__execute_pg(pg_info->conn, "COMMIT");
        
        return ret;
    }
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
/*!
   \brief Writes node on topological level (PostGIS Topology
   interface, internal use only)

   The vector map must be open on level 2 at least with
   GV_BUILD_BASE. PostGIS Topology schema must be defined.

   \param Map pointer to Map_info structure
   \param node node id (starts at 1)
   \param points pointer to line_pnts structure
   
   \return 0 on success
   \return -1 on error
*/
off_t V2__write_node_pg(struct Map_info *Map, const struct line_pnts *points)
{
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (!pg_info->toposchema_name)
        return -1; /* PostGIS Topology required */
    
    return write_line_tp(Map, GV_POINT, TRUE, points, NULL);
}

/*!
   \brief Writes area on topological level (PostGIS Simple Features
   interface, internal use only)

   \param Map pointer to Map_info structure
   \param points feature geometry (exterior + interior rings)
   \param nparts number of parts including exterior ring
   \param cats feature categories
   
   \return feature offset
   \return -1 on error
*/
off_t V2__write_area_pg(struct Map_info *Map, 
                        const struct line_pnts **points, int nparts,
                        const struct line_cats *cats)
{
    return write_line_sf(Map, GV_BOUNDARY, points, nparts, cats);
}

/*!
   \brief Updates simple features geometry from GRASS-like topo

   \param Map pointer to Map_info structure
   \param points feature geometry (exterior + interior rings)
   \param nparts number of parts including exterior ring
   \param cat area category
   
   \return 0 on success
   \return -1 on error
*/
int V2__update_area_pg(struct Map_info *Map, 
                         const struct line_pnts **points, int nparts,
                         int cat)
{
    int part, npoints;
    char *stmt, *geom_data;

    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    for (part = 0; part < nparts; part++) { 
        npoints = points[part]->n_points - 1;
        if (points[part]->x[0] != points[part]->x[npoints] ||
            points[part]->y[0] != points[part]->y[npoints] ||
            points[part]->z[0] != points[part]->z[npoints]) {
            G_warning(_("Boundary is not closed. Skipping."));
            return -1;
        }
    }
    
    geom_data = line_to_wkb(pg_info, points, nparts, GV_AREA, Vect_is_3d(Map));
    if (!geom_data)
        return -1;
    
    stmt = NULL;
    G_asprintf(&stmt, "UPDATE \"%s\".\"%s\" SET %s = '%s'::GEOMETRY WHERE %s = %d",
            pg_info->schema_name, pg_info->table_name, pg_info->geom_column,
            geom_data, pg_info->fid_column,
            cat);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK"); 
        G_free(geom_data);
        G_free(stmt);
        return -1;
    }

    G_free(geom_data);
    G_free(stmt);

    return 0;
}

/*!
  \brief Create new feature table

  \param pg_info pointer to Format_info_pg

  \return -1 on error
  \return 0 on success
*/
int create_table(struct Format_info_pg *pg_info)
{
    int spatial_index, primary_key;
    char stmt[DB_SQL_MAX];
    char *geom_type, *def_file;
    
    struct field_info *Fi;
    
    PGresult *result;

    def_file = getenv("GRASS_VECTOR_PGFILE");
    
    /* by default create spatial index & add primary key */
    spatial_index = primary_key = TRUE;
    if (G_find_file2("", def_file ? def_file : "PG", G_mapset())) {
        FILE *fp;
        const char *p;

        struct Key_Value *key_val;

        fp = G_fopen_old("", def_file ? def_file : "PG", G_mapset());
        if (!fp) {
            G_warning(_("Unable to open PG file"));
        }
        else {
            key_val = G_fread_key_value(fp);
            fclose(fp);
            
            /* disable spatial index ? */
            p = G_find_key_value("spatial_index", key_val);
            if (p && G_strcasecmp(p, "no") == 0)
                spatial_index = FALSE;
            
            /* disable primary key ? */
            p = G_find_key_value("primary_key", key_val);
            if (p && G_strcasecmp(p, "no") == 0)
                primary_key = FALSE;

            G_free_key_value(key_val);
        }
    }
    
    /* create schema if not exists */
    if (G_strcasecmp(pg_info->schema_name, "public") != 0) {
        if (check_schema(pg_info) != 0)
            return -1;
    }

    /* prepare CREATE TABLE statement */
    sprintf(stmt, "CREATE TABLE \"%s\".\"%s\" (%s SERIAL%s, %s INTEGER",
            pg_info->schema_name, pg_info->table_name, pg_info->fid_column,
            primary_key ? " PRIMARY KEY" : "", GV_KEY_COLUMN);
    
    Fi = pg_info->fi;
    
    if (Fi) {
        /* append attributes */
        int col, ncols, sqltype, length;
        char stmt_col[DB_SQL_MAX];
        const char *colname;

        dbString dbtable_name;

        dbDriver *driver;
        dbTable *table;
        dbColumn *column;

        db_init_string(&dbtable_name);

        driver = open_db(pg_info);
        if (driver == NULL)
            return -1;

        /* describe table */
        db_set_string(&dbtable_name, Fi->table);
        if (db_describe_table(driver, &dbtable_name, &table) != DB_OK) {
            G_warning(_("Unable to describe table <%s>"),
                      Fi->table);
            db_close_database_shutdown_driver(driver);
            pg_info->dbdriver = NULL;
            return -1;
        }
        ncols = db_get_table_number_of_columns(table);

        G_debug(3,
                "copying attributes: driver = %s database = %s table = %s cols = %d",
                Fi->driver, Fi->database, Fi->table, ncols);

        for (col = 0; col < ncols; col++) {
            column = db_get_table_column(table, col);
            colname = db_get_column_name(column);
            sqltype = db_get_column_sqltype(column);
            length = db_get_column_length(column);

            G_debug(3, "\tcolumn = %d name = %s type = %d length = %d",
                    col, colname, sqltype, length);

            if (G_strcasecmp(pg_info->fid_column, colname) == 0 ||
                G_strcasecmp(GV_KEY_COLUMN, colname) == 0) {
                /* skip fid column if exists */
                G_debug(3, "\t%s skipped", colname);
                continue;
            }

            /* append column */
            sprintf(stmt_col, ",\"%s\" %s", colname, db_sqltype_name(sqltype));
            strcat(stmt, stmt_col);
            if (sqltype == DB_SQL_TYPE_CHARACTER) {
                /* length only for string columns */
                sprintf(stmt_col, "(%d)", length);
                strcat(stmt, stmt_col);
            }
        }

        db_free_string(&dbtable_name);
    }
    strcat(stmt, ")");          /* close CREATE TABLE statement */

    /* begin transaction (create table) */
    if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1) {
        return -1;
    }

    /* create table */
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* determine geometry type (string) */
    switch (pg_info->feature_type) {
    case (SF_POINT):
        geom_type = "POINT";
        break;
    case (SF_LINESTRING):
        geom_type = "LINESTRING";
        break;
    case (SF_POLYGON):
        geom_type = "POLYGON";
        break;
    case (SF_POLYGON25D):
        geom_type = "POLYGONZ";
        break;
    case (SF_GEOMETRY):
        geom_type = "GEOMETRY";
        break;
    default:
        G_warning(_("Unsupported feature type %d"), pg_info->feature_type);
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
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
        G_warning("%s", PQresultErrorMessage(result));
        PQclear(result);
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    /* create indices
        - GV_KEY_COLUMN
        - geometry column
    */
    sprintf(stmt,
            "CREATE INDEX %s_%s_idx ON \"%s\".\"%s\" (%s)",
            pg_info->table_name, GV_KEY_COLUMN,
            pg_info->schema_name, pg_info->table_name,
            GV_KEY_COLUMN);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    if (spatial_index) {
        G_verbose_message(_("Building spatial index on <%s>..."),
                          pg_info->geom_column);
        sprintf(stmt,
                "CREATE INDEX %s_%s_idx ON \"%s\".\"%s\" USING GIST (%s)",
                pg_info->table_name, pg_info->geom_column,
                pg_info->schema_name, pg_info->table_name,
                pg_info->geom_column);
        
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
    }

    /* close transaction (create table) */
    if (Vect__execute_pg(pg_info->conn, "COMMIT") == -1) {
        return -1;
    }

    return 0;
}

/*!
  \brief Creates new schema for feature table if not exists

  \param pg_info pointer to Format_info_pg

  \return -1 on error
  \return 0 on success
*/
int check_schema(const struct Format_info_pg *pg_info)
{
    int i, found, nschema;
    char stmt[DB_SQL_MAX];

    PGresult *result;

    if (!pg_info->conn || !pg_info->table_name) {
        G_warning(_("No connection defined"));
        return -1;
    }

    /* add geometry column */
    sprintf(stmt, "SELECT nspname FROM pg_namespace");
    G_debug(2, "SQL: %s", stmt);
    result = PQexec(pg_info->conn, stmt);

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        PQclear(result);
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
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
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
        G_warning(_("Schema <%s> doesn't exist, created"),
                  pg_info->schema_name);
    }

    return 0;
}

/*!
  \brief Create new PostGIS topology schema

  - create topology schema
  - add topology column to the feature table
  
  \todo Add constraints for grass-like tables
  
  \param pg_info pointer to Format_info_pg

  \return 0 on success
  \return 1 topology disable, nothing to do
  \return -1 on failure
*/
int create_topo_schema(struct Format_info_pg *pg_info, int with_z)
{
    double tolerance;
    char stmt[DB_SQL_MAX];
    char *def_file;
    
    def_file = getenv("GRASS_VECTOR_PGFILE");
    
    /* read default values from PG file*/
    tolerance = 0.;
    if (G_find_file2("", def_file ? def_file : "PG", G_mapset())) {
        FILE *fp;
        const char *p;

        struct Key_Value *key_val;

        fp = G_fopen_old("", def_file ? def_file : "PG", G_mapset());
        if (!fp) {
            G_fatal_error(_("Unable to open PG file"));
        }
        key_val = G_fread_key_value(fp);
        fclose(fp);

        /* tolerance */
        p = G_find_key_value("topo_tolerance", key_val);
        if (p)
            tolerance = atof(p);
        G_debug(1, "PG: tolerance: %f", tolerance);
        
        /* topogeom column */
        p = G_find_key_value("topogeom_name", key_val);
        if (p)
            pg_info->topogeom_column = G_store(p);
        else
            pg_info->topogeom_column = G_store(TOPOGEOM_COLUMN);
        G_debug(1, "PG: topogeom_column :%s", pg_info->topogeom_column);

        /* topo-geo only (default: no) */
        p = G_find_key_value("topo_geo_only", key_val);
        if (p && G_strcasecmp(p, "yes") == 0)
            pg_info->topo_geo_only = TRUE;
        G_debug(1, "PG: topo_geo_only :%d", pg_info->topo_geo_only);

        /* build simple features from topogeometry data */
        p = G_find_key_value("simple_feature", key_val);
        if (p && G_strcasecmp(p, "yes") == 0)
            pg_info->topo_geo_only = TRUE;
        G_debug(1, "PG: topo_geo_only :%d", pg_info->topo_geo_only);

        G_free_key_value(key_val);
    }

    /* begin transaction (create topo schema) */
    if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1) {
        return -1;
    }

    /* create topology schema */
    G_verbose_message(_("Creating topology schema <%s>..."),
                      pg_info->toposchema_name);
    sprintf(stmt, "SELECT topology.createtopology('%s', "
            "find_srid('%s', '%s', '%s'), %f, '%s')",
            pg_info->toposchema_name, pg_info->schema_name,
            pg_info->table_name, pg_info->geom_column, tolerance,
            with_z == WITH_Z ? "t" : "f");
    pg_info->toposchema_id = Vect__execute_get_value_pg(pg_info->conn, stmt);
    if (pg_info->toposchema_id == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* add topo column to the feature table */
    G_verbose_message(_("Adding new topology column <%s>..."),
                      pg_info->topogeom_column);
    sprintf(stmt, "SELECT topology.AddTopoGeometryColumn('%s', '%s', '%s', "
            "'%s', '%s')", pg_info->toposchema_name, pg_info->schema_name,
            pg_info->table_name, pg_info->topogeom_column,
            get_sftype(pg_info->feature_type));
    if (-1 == Vect__execute_get_value_pg(pg_info->conn, stmt)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* create index on topo column */
    sprintf(stmt, "CREATE INDEX \"%s_%s_%s_idx\" ON \"%s\".\"%s\" (((%s).id))",
            pg_info->schema_name, pg_info->table_name, pg_info->topogeom_column,
            pg_info->schema_name, pg_info->table_name, pg_info->topogeom_column);
    if (-1 == Vect__execute_pg(pg_info->conn, stmt)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    /* change constraints to deferrable initially deferred */
    if (!pg_info->topo_geo_only) {
        if (-1 == set_constraint_to_deferrable(pg_info, "node", "face_exists",
                                               "containing_face", "face", "face_id") ||
            -1 == set_constraint_to_deferrable(pg_info, "edge_data", "end_node_exists",
                                               "end_node", "node", "node_id") ||
            -1 == set_constraint_to_deferrable(pg_info, "edge_data", "left_face_exists",
                                               "left_face", "face", "face_id") ||
            -1 == set_constraint_to_deferrable(pg_info, "edge_data", "right_face_exists",
                                               "right_face", "face", "face_id") ||
            -1 == set_constraint_to_deferrable(pg_info, "edge_data", "start_node_exists",
                                               "start_node", "node", "node_id"))
            return -1;
    }

    /* create additional tables in topological schema to store
       GRASS topology in DB */
    if (!pg_info->topo_geo_only) {
        /* (1) create 'node_grass' (see P_node struct)
           
           todo: add constraints for lines and angles
        */
        sprintf(stmt, "CREATE TABLE \"%s\".%s (node_id SERIAL PRIMARY KEY, "
                "lines integer[], angles float[])", pg_info->toposchema_name, TOPO_TABLE_NODE);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        sprintf(stmt, "ALTER TABLE \"%s\".%s ADD CONSTRAINT node_exists "
                "FOREIGN KEY (node_id) REFERENCES \"%s\".node (node_id) "
                "DEFERRABLE INITIALLY DEFERRED",
                pg_info->toposchema_name, TOPO_TABLE_NODE, pg_info->toposchema_name);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        /* (2) create 'line_grass' (see P_line struct)
           
        */
        sprintf(stmt, "CREATE TABLE \"%s\".%s (line_id SERIAL PRIMARY KEY, "
                "left_area integer, right_area integer)",
                pg_info->toposchema_name, TOPO_TABLE_LINE);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        sprintf(stmt, "ALTER TABLE \"%s\".%s ADD CONSTRAINT line_exists "
                "FOREIGN KEY (line_id) REFERENCES \"%s\".edge_data (edge_id) "
                "DEFERRABLE INITIALLY DEFERRED",
                pg_info->toposchema_name, TOPO_TABLE_LINE, pg_info->toposchema_name);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        /* (3) create 'area_grass' (see P_area struct)
           
           todo: add constraints for lines, centtroid and isles
        */
        sprintf(stmt, "CREATE TABLE \"%s\".%s (area_id SERIAL PRIMARY KEY, "
                "lines integer[], centroid integer, isles integer[])",
                pg_info->toposchema_name, TOPO_TABLE_AREA);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }

        /* (4) create 'isle_grass' (see P_isle struct)
           
           todo: add constraints for lines and area
        */
        sprintf(stmt, "CREATE TABLE \"%s\".%s (isle_id SERIAL PRIMARY KEY, "
                "lines integer[], area integer)",
                pg_info->toposchema_name, TOPO_TABLE_ISLE);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
    }

    /* close transaction (create topo schema) */
    if (Vect__execute_pg(pg_info->conn, "COMMIT") == -1) {
        return -1;
    }

    return 0;
}

/*!
   \brief Create new PostGIS layer in given database (internal use only)

   V1_open_new_pg() must be called before this function.

   List of currently supported types:
   - GV_POINT     (SF_POINT)
   - GV_LINE      (SF_LINESTRING)
   - GV_BOUNDARY  (SF_POLYGON)

   When PostGIS Topology the map level is updated to topological level
   and build level set to GV_BUILD_BASE.

   \param[in,out] Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)

   \return 0 success
   \return -1 error 
 */
int create_pg_layer(struct Map_info *Map, int type)
{
    int ndblinks;

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

    G_debug(1, "Vect__open_new_pg(): conninfo='%s' table='%s' -> type = %d",
            pg_info->conninfo, pg_info->table_name, type);

    /* determine geometry type */
    
    switch (type) {
    case GV_POINT:
    case GV_CENTROID:
        pg_info->feature_type = SF_POINT;
        break;
    case GV_LINE:
    case GV_BOUNDARY:
        pg_info->feature_type = SF_LINESTRING;
        break;
    case GV_AREA:
        pg_info->feature_type = SF_POLYGON;
        break;
    case GV_FACE:
        pg_info->feature_type = SF_POLYGON25D;
        break;
    case -2:
        pg_info->feature_type = SF_GEOMETRY;
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
        pg_info->fi = Vect_get_dblink(Map, 0); /* TODO: support more layers */
        if (pg_info->fi) {
            if (ndblinks > 1)
                G_warning(_("More layers defined, using driver <%s> and "
                            "database <%s>"),
                          pg_info->fi->driver, pg_info->fi->database);
        }
        else {
            G_warning(_("Database connection not defined. "
                        "Unable to write attributes."));
        }
    }

    /* create new feature table */
    if (create_table(pg_info) == -1) {
        G_warning(_("Unable to create new PostGIS feature table"));
        return -1;
    }
    
    /* create new topology schema (if PostGIS topology support is enabled) */
    if (pg_info->toposchema_name) {
        /* force topological level */
        Map->level = LEVEL_2;
        Map->plus.built = GV_BUILD_BASE;
        
        /* track updated features, used in V2__add_line_to_topo_nat() */
        Vect_set_updated(Map, TRUE);
        
        if (create_topo_schema(pg_info, Vect_is_3d(Map)) == -1) {
            G_warning(_("Unable to create new PostGIS topology schema"));
            return -1;
        }
    }
    
    return 0;
}

/*!
  \brief Get simple feature type as a string

  Used for AddTopoGeometryColumn().

  Valid types:
   - SF_POINT
   - SF_LINESTRING
   - SF_POLYGON

  \return string with feature type
  \return empty string
*/
char *get_sftype(SF_FeatureType sftype)
{
    if (sftype == SF_POINT)
        return "POINT";
    else if (sftype == SF_LINESTRING)
        return "LINE";
    else if (sftype == SF_POLYGON)
        return "POLYGON";
    else if (sftype == SF_GEOMETRY || sftype == SF_GEOMETRYCOLLECTION)
        return "COLLECTION";
    else
        G_warning(_("Unsupported feature type %d"), sftype);
    
    return "";
}

/*!
  \brief Write vector features as PostGIS simple feature element
  
   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points feature geometry (exterior + interior rings for polygonsx)
   \param nparts number of parts
   \param cats feature categories

   \return feature offset
   \return -1 on error
*/
off_t write_line_sf(struct Map_info *Map, int type,
                    const struct line_pnts **points, int nparts,
                    const struct line_cats *cats)
{
    int cat;
    off_t offset;

    SF_FeatureType sf_type;

    struct Format_info_pg *pg_info;
    struct Format_info_offset *offset_info;

    pg_info = &(Map->fInfo.pg);
    offset_info = &(pg_info->offset);

    if (nparts < 1)
        return -1;
    
    /* check required PG settings */
    if (!pg_info->conn) {
        G_warning(_("No connection defined"));
        return -1;
    }
    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }

    /* create PostGIS table if doesn't exist */
    if (pg_info->feature_type == SF_GEOMETRY) {
        if (create_pg_layer(Map, type) < 0)
            return -1;
    }
    
    /* get category & check for attributes */
    cat = -1;
    if (cats->n_cats > 0) {
        int field;
        
        if (pg_info->fi)
            field = pg_info->fi->number;
        else
            field = 1;
        
        if (!Vect_cat_get(cats, field, &cat))
            G_warning(_("No category defined for layer %d"), field);
        if (cats->n_cats > 1) {
            G_warning(_("Feature has more categories, using "
                        "category %d (from layer %d)"),
                      cat, field);
        }
    }

    sf_type = pg_info->feature_type;

    /* determine matching PostGIS feature geometry type */
    if (type & (GV_POINT | GV_KERNEL)) {
        if (sf_type != SF_POINT && sf_type != SF_POINT25D) {
            G_warning(_("Point skipped (output feature type: %s)"),
                      Vect_get_finfo_geometry_type(Map));
            return 0;
        }
    }
    else if (type & GV_LINE) {
        if (sf_type != SF_LINESTRING && sf_type != SF_LINESTRING25D) {
            G_warning(_("Line skipped (output feature type: %s)"),
                      Vect_get_finfo_geometry_type(Map));
            return 0;
        }
    }
    else if (type & GV_CENTROID) {
        if (sf_type != SF_POLYGON && sf_type != SF_POINT) {
            G_warning(_("Centroid skipped (output feature type: %s)"),
                      Vect_get_finfo_geometry_type(Map));
            return 0;
        }
    }
    else if (type & GV_BOUNDARY) {
        if (sf_type != SF_POLYGON && sf_type != SF_LINESTRING) {
            G_warning(_("Boundary skipped (output feature type: %s)"),
                      Vect_get_finfo_geometry_type(Map));
            return 0;
        }
    }
    else if (type & GV_FACE) {
        if (sf_type != SF_POLYGON25D) {
            G_warning(_("Face skipped (output feature type: %s)"),
                      Vect_get_finfo_geometry_type(Map));
            return 0;
        }
    }
    else {
        G_warning(_("Unsupported feature type %d"), type);
        return -1;
    }
    
    G_debug(3, "write_line_sf(): type = %d n_points = %d cat = %d",
            type, points[0]->n_points, cat);

    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
        /* skip this check when writing PostGIS topology */
        int part, npoints;

        for (part = 0; part < nparts; part++) { 
            npoints = points[part]->n_points - 1;
            if (points[part]->x[0] != points[part]->x[npoints] ||
                points[part]->y[0] != points[part]->y[npoints] ||
                points[part]->z[0] != points[part]->z[npoints]) {
                G_warning(_("Boundary is not closed. Skipping."));
                return -1;
            }
        }
    }

    /* write feature's geometry and fid */
    if (-1 == write_feature(Map, -1, type, points, nparts, cat)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update offset array */
    if (offset_info->array_num >= offset_info->array_alloc) {
        offset_info->array_alloc += 1000;
        offset_info->array = (int *)G_realloc(offset_info->array,
                                              offset_info->array_alloc *
                                              sizeof(int));
    }
    offset = offset_info->array_num;

    offset_info->array[offset_info->array_num++] = cat;
    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
        /* register first part in offset array */
        offset_info->array[offset_info->array_num++] = 0;
    }
    G_debug(3, "write_line_sf(): -> offset = %lu offset_num = %d cat = %d",
            (unsigned long)offset, offset_info->array_num, cat);

    return offset;
}

/*! 
  \brief Write vector feature in PostGIS topology schema and
  updates internal topology structures

  \param Map vector map
  \param type feature type to be written
  \param points feature geometry
  \param is_node TRUE for nodes (written as points)
  
  \return feature id (build level >= GV_BUILD_BASE otherwise 0)
  \return 0 for nodes
  \return -1 on error
*/
off_t write_line_tp(struct Map_info *Map, int type, int is_node,
                    const struct line_pnts *points,
                    const struct line_cats *cats)
{
    int line, cat, line_id;

    struct Format_info_pg *pg_info;
    struct Plus_head *plus;
    struct field_info *Fi;
    
    pg_info = &(Map->fInfo.pg);
    plus = &(Map->plus);

    if (!(plus->update_cidx)) {
	plus->cidx_up_to_date = FALSE; /* category index will be outdated */
    }

    /* check type for nodes */
    if (is_node && type != GV_POINT) {
        G_warning(_("Invalid feature type (%d) for nodes"), type);
        return -1;
    }

    /* check required PG settings */
    if (!pg_info->conn) {
        G_warning(_("No connection defined"));
        return -1;
    }
    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }
    if (!pg_info->toposchema_name) {
        G_warning(_("PostGIS topology schema not defined"));
        return -1;
    }
    
    /* create PostGIS table if doesn't exist */
    if (pg_info->feature_type == SF_GEOMETRY) {
        if (create_pg_layer(Map, type) < 0)
            return -1;
    }

    if (!points)
        return 0;
    
    G_debug(3, "write_line_pg(): type = %d n_points = %d",
            type, points->n_points);

    Fi = pg_info->fi;
    
    cat = -1;
    if (cats && cats->n_cats > 0) {
        if (Fi) {
            if (!pg_info->dbdriver)
                open_db(pg_info);
            if (!Vect_cat_get(cats, Fi->number, &cat))
                G_warning(_("No category defined for layer %d"), Fi->number);
            if (cats->n_cats > 1) {
                G_warning(_("Feature has more categories, using "
                            "category %d (from layer %d)"),
                          cat, cats->field[0]);
            }
        }
        /* assume layer=1 */
        Vect_cat_get(cats, 1, &cat);
    }

    /* update GRASS topology before writing PostGIS feature */
    line = 0;
    if (plus->built >= GV_BUILD_BASE) {
        if (is_node) {
            /* nodes are given with negative id */
            line = -1 * dig_add_node(plus, points->x[0], points->y[0], points->z[0]);
        }
        else {
            off_t offset;
            
            /* better is probably to check nextval directly */
            if (type & GV_POINTS) {
                offset = Vect_get_num_primitives(Map, GV_POINTS) + 1; /* next */
                offset += Vect_get_num_nodes(Map); /* nodes are also stored in 'node' table */
            }
            else { /* LINES */
                offset = Vect_get_num_primitives(Map, GV_LINES) + 1; /* next */
            }
            
            line = add_line_to_topo_pg(Map, offset, type, points);
        }
    }
    
    /* write new feature to PostGIS
       - feature table for simple features
       - feature table and topo schema for topological access
    */
    line_id = write_feature(Map, line, type, &points, 1, cat);
    if (line_id < 0) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    if (pg_info->cache.ctype == CACHE_MAP) {
        /* add line to the cache */
        Vect__reallocate_cache(&(pg_info->cache), 1, TRUE);
        pg_info->cache.lines[line-1] = Vect_new_line_struct();
        pg_info->cache.lines_types[line-1] = type;
        pg_info->cache.lines_cats[line-1] = cat;
   }

    /* update offset array for nodes */    
    if (is_node) {
        int node;
        
        struct Format_info_offset *offset;

        offset = &(pg_info->offset);
        
        node = abs(line);
        if (node > offset->array_alloc) {
            offset->array_alloc += 1000;
            offset->array = (int *) G_realloc (offset->array, offset->array_alloc * sizeof(int));
        }
        
        offset->array_num = node; 
        offset->array[node-1] = (int) line_id; /* node id starts at 1 */
    }

    /* update PostGIS-line topo */
    if (plus->built >= GV_BUILD_AREAS && type == GV_BOUNDARY)
        update_topo_face(Map, line); /* TODO: avoid extra statements */
    
    return !is_node ? line : 0;
}

/*!
   \brief Binary data to HEX

   Allocated buffer should be freed by G_free().

   \param nbytes number of bytes to allocate
   \param wkb_data WKB data

   \return allocated buffer with HEX data
 */
char *binary_to_hex(int nbytes, const unsigned char *wkb_data)
{
    char *hex_data;
    int i, nlow, nhigh;
    static const char ach_hex[] = "0123456789ABCDEF";

    hex_data = (char *)G_malloc(nbytes * 2 + 1);
    hex_data[nbytes * 2] = '\0';

    for (i = 0; i < nbytes; i++) {
        nlow = wkb_data[i] & 0x0f;
        nhigh = (wkb_data[i] & 0xf0) >> 4;

        hex_data[i * 2] = ach_hex[nhigh];
        hex_data[i * 2 + 1] = ach_hex[nlow];
    }

    return hex_data;
}

/*!
   \brief Write point into WKB buffer

   See OGRPoint::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or BIG_ENDIAN)
   \param points feature geometry
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *point_to_wkb(int byte_order,
                            const struct line_pnts *points, int with_z,
                            int *nsize)
{
    unsigned char *wkb_data;
    unsigned int sf_type;

    if (points->n_points != 1)
        return NULL;

    /* allocate buffer */
    *nsize = with_z ? 29 : 21;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->point size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_POINT25D : SF_POINT;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the raw data */
    memcpy(wkb_data + 5, &(points->x[0]), 8);
    memcpy(wkb_data + 5 + 8, &(points->y[0]), 8);

    if (with_z) {
        memcpy(wkb_data + 5 + 16, &(points->z[0]), 8);
    }

    /* swap if needed */
    if (byte_order == ENDIAN_BIG) {
        SWAPDOUBLE(wkb_data + 5);
        SWAPDOUBLE(wkb_data + 5 + 8);

        if (with_z)
            SWAPDOUBLE(wkb_data + 5 + 16);
    }

    return wkb_data;
}

/*!
   \bried Write linestring into WKB buffer

   See OGRLineString::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or ENDIAN_BIG)
   \param points feature geometry
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *linestring_to_wkb(int byte_order,
                                 const struct line_pnts *points, int with_z,
                                 int *nsize)
{
    int i, point_size;
    unsigned char *wkb_data;
    unsigned int sf_type;

    if (points->n_points < 1)
        return NULL;

    /* allocate buffer */
    point_size = 8 * (with_z ? 3 : 2);
    *nsize = 5 + 4 + points->n_points * point_size;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->linestring size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_LINESTRING25D : SF_LINESTRING;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the data count */
    memcpy(wkb_data + 5, &(points->n_points), 4);

    /* copy in the raw data */
    for (i = 0; i < points->n_points; i++) {
        memcpy(wkb_data + 9 + point_size * i, &(points->x[i]), 8);
        memcpy(wkb_data + 9 + 8 + point_size * i, &(points->y[i]), 8);

        if (with_z) {
            memcpy(wkb_data + 9 + 16 + point_size * i, &(points->z[i]), 8);
        }
    }

    /* swap if needed */
    if (byte_order == ENDIAN_BIG) {
        int npoints, nitems;

        npoints = SWAP32(points->n_points);
        memcpy(wkb_data + 5, &npoints, 4);

        nitems = (with_z ? 3 : 2) * points->n_points;
        for (i = 0; i < nitems; i++) {
            SWAPDOUBLE(wkb_data + 9 + 4 + 8 * i);
        }
    }

    return wkb_data;
}

/*!
   \bried Write polygon into WKB buffer

   See OGRPolygon::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or ENDIAN_BIG)
   \param ipoints list of ring geometries (first is outer ring)
   \param nrings number of rings
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *polygon_to_wkb(int byte_order,
                              const struct line_pnts** points, int nrings,
                              int with_z, int *nsize)
{
    int i, ring, point_size, offset;
    unsigned char *wkb_data;
    unsigned int sf_type;

    /* check data validity */
    if (nrings < 1)
        return NULL;
    for (ring = 0; ring < nrings; ring++) {
        if (points[ring]->n_points < 3)
            return NULL;
    }

    /* allocate buffer */
    point_size = 8 * (with_z ? 3 : 2);
    *nsize = 9;
    for (ring = 0; ring < nrings; ring++)
        *nsize += 4 + point_size * points[ring]->n_points;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->polygon size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_POLYGON25D : SF_POLYGON;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the raw data */
    if (byte_order == ENDIAN_BIG) {
        int ncount;

        ncount = SWAP32(nrings);
        memcpy(wkb_data + 5, &ncount, 4);
    }
    else {
        memcpy(wkb_data + 5, &nrings, 4);
    }

    /* serialize rings */
    offset = 9;
    for (ring = 0; ring < nrings; ring++) {
        memcpy(wkb_data + offset, &(points[ring]->n_points), 4);
        for (i = 0; i < points[ring]->n_points; i++) {
            memcpy(wkb_data + offset +
                   4 + point_size * i, &(points[ring]->x[i]), 8);
            memcpy(wkb_data + offset +
                   4 + 8 + point_size * i, &(points[ring]->y[i]), 8);
            
            if (with_z) {
                memcpy(wkb_data + offset +
                       4 + 16 + point_size * i, &(points[ring]->z[i]), 8);
            }
        }
        
        offset += 4 + point_size * points[ring]->n_points;
        
        /* swap if needed */
        if (byte_order == ENDIAN_BIG) {
            int npoints, nitems;
            
            npoints = SWAP32(points[ring]->n_points);
            memcpy(wkb_data + 5, &npoints, 4);
            
            nitems = (with_z ? 3 : 2) * points[ring]->n_points;
            for (i = 0; i < nitems; i++) {
                SWAPDOUBLE(wkb_data + offset + 4 + 8 * i);
            }
        }
    }
    
    return wkb_data;
}

/*!
  \brief Write feature to WKB buffer

  Allocated string buffer should be freed by G_free().

  \param pg_info pointer to Format_info_pg struct
  \param points array of geometries which form feature
  \param nparts number of geometries in array
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param with_z WITH_Z for 3D data

  \return allocated string buffer
  \return NULL on error
*/
char *line_to_wkb(struct Format_info_pg *pg_info,
                  const struct line_pnts **points, int nparts, int type, int with_z)
{
    int byte_order, nbytes, nsize;
    unsigned int sf_type;
    
    unsigned char *wkb_data;
    char *text_data, *text_data_p, *hex_data;
    
    byte_order = dig__byte_order_out();

    /* get wkb data */
    nbytes = -1;
    wkb_data = NULL;
    if (type & GV_POINTS) /* point or centroid */
        wkb_data = point_to_wkb(byte_order, points[0], with_z, &nbytes);
    else if (type == GV_LINE ||
             (type == GV_BOUNDARY && pg_info->feature_type == SF_LINESTRING))
        wkb_data = linestring_to_wkb(byte_order, points[0], with_z, &nbytes);
    else if (type & (GV_BOUNDARY | GV_FACE | GV_AREA)) {
        if (!pg_info->toposchema_name || type == GV_AREA) {
            /* PostGIS simple feature access */
            wkb_data = polygon_to_wkb(byte_order, points, nparts,
                                      with_z, &nbytes);
        }
        else {
            /* PostGIS topology access */
            wkb_data = linestring_to_wkb(byte_order, points[0], with_z, &nbytes);
        }
    }
    
    if (!wkb_data || nbytes < 1) {
        G_warning(_("Unsupported feature type %d"), type);
        return NULL;
    }

    /* When converting to hex, each byte takes 2 hex characters. In
       addition we add in 8 characters to represent the SRID integer
       in hex, and one for a null terminator */
    nsize = nbytes * 2 + 8 + 1;
    text_data = text_data_p = (char *)G_malloc(nsize);

    /* convert the 1st byte, which is the endianess flag, to hex */
    hex_data = binary_to_hex(1, wkb_data);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);
    text_data_p += 2;

    /* get the geom type which is bytes 2 through 5 */
    memcpy(&sf_type, wkb_data + 1, 4);

    /* add the SRID flag if an SRID is provided */
    if (pg_info->srid > 0) {
        unsigned int srs_flag;

        /* change the flag to little endianess */
        srs_flag = LSBWORD32(WKBSRIDFLAG);
        /* apply the flag */
        sf_type = sf_type | srs_flag;
    }

    /* write the geom type which is 4 bytes */
    hex_data = binary_to_hex(4, (unsigned char *)&sf_type);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);
    text_data_p += 8;

    /* include SRID if provided */
    if (pg_info->srid > 0) {
        unsigned int srs_id;

        /* force the srsid to little endianess */
        srs_id = LSBWORD32(pg_info->srid);
        hex_data = binary_to_hex(sizeof(srs_id), (unsigned char *)&srs_id);
        strcpy(text_data_p, hex_data);
        G_free(hex_data);
        text_data_p += 8;
    }

    /* copy the rest of the data over - subtract 5 since we already
       copied 5 bytes above */
    hex_data = binary_to_hex(nbytes - 5, wkb_data + 5);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);

    return text_data;
}

/*!
   \brief Insert feature into table

   \param Map pointer to Map_info structure
   \param line feature id (topo access only)
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts struct
   \param nparts number of parts (rings for polygon)
   \param cat category number (-1 for no category)

   \return topo_id for PostGIS Topology 
   \return 0 for simple features access
   \return -1 on error
 */
int write_feature(struct Map_info *Map, int line, int type,
                  const struct line_pnts **points, int nparts, int cat)
{
    int with_z, topo_id;
    char *stmt, *geom_data;

    struct Format_info_pg *pg_info;
    
    pg_info = &(Map->fInfo.pg);
    with_z  = Map->head.with_z;
    
    if (with_z && pg_info->coor_dim != 3) {
        G_warning(_("Trying to insert 3D data into feature table "
                    "which store 2D data only"));
        return -1;
    }
    if (!with_z && pg_info->coor_dim != 2) {
        G_warning(_("Trying to insert 2D data into feature table "
                    "which store 3D data only"));
        return -1;
    }

    /* build WKB geometry from line_pnts structures */
    geom_data = line_to_wkb(pg_info, points, nparts, type, with_z);
    if (!geom_data)
        return -1;
    
    /* start transaction */
    if (!pg_info->inTransaction) {
        pg_info->inTransaction = TRUE;
        if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1) {
            G_free(geom_data);
            return -1;
        }
    }

    /* write feature in PostGIS topology schema if enabled */
    topo_id = -1;
    if (pg_info->toposchema_name) {
        /* insert feature into topology schema (node or edge) */
        topo_id = insert_topo_element(Map, line, type, geom_data);
        if (topo_id < 0) {
            G_warning(_("Unable to insert topological element into PostGIS Topology schema"));
            G_free(geom_data);
            
            return -1;
        }

        if (pg_info->feature_type != SF_POLYGON) {
            /* define relation */
            Vect__define_topo_relation(pg_info, topo_id, topo_id);
        }
    }

    /* build INSERT statement
       simple feature geometry + attributes
    */
    stmt = build_insert_stmt(pg_info, geom_data, topo_id, cat);

    /* stmt can NULL when writing PostGIS topology with no attributes
     * attached */
    if (stmt && Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK"); 
        G_free(geom_data);
        G_free(stmt);
        
        return -1;
    }
    G_free(geom_data);
    G_free(stmt);
    
    return pg_info->toposchema_name ? topo_id : 0;
}

/*!
   \brief Build INSERT statement to add new feature to the feature
   table

   Note: Allocated string should be freed.
   
   \param pg_info pointer to Format_info_pg structure
   \param geom_data geometry data
   \param type feature type (GV_POINT, GV_LINE, ...) - (only for PostGIS Topology)
   \param id   topology element id (only for PostGIS Topology)
   \param cat category number (or -1 for no category)
   \param Fi pointer to field_info structure (NULL for no attributes)

   \return allocated string with INSERT statement
 */
char *build_insert_stmt(const struct Format_info_pg *pg_info,
                        const char *geom_data, int topo_id, int cat)
{
    int topogeom_type;
    
    char *stmt, buf[DB_SQL_MAX];

    struct field_info *Fi;
    
    topogeom_type = -1;
    if (pg_info->toposchema_name) {
        topogeom_type = type_to_topogeom(pg_info);
        if (topogeom_type < 0)
            return NULL;
    }

    Fi = pg_info->fi;
    
    stmt = NULL;
    if (Fi && cat > -1) {
        /* write attributes (simple features and topology elements) */
        int col, ncol, more;
        int sqltype, ctype, is_fid;
        char buf_val[DB_SQL_MAX], buf_tmp[DB_SQL_MAX];

        const char *colname;

        dbString dbstmt;
        dbCursor cursor;
        dbTable *table;
        dbColumn *column;
        dbValue *value;

        db_init_string(&dbstmt);
        buf_val[0] = '\0';

        /* read & set attributes */
        sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key,
                cat);
        G_debug(4, "SQL: %s", buf);
        db_set_string(&dbstmt, buf);
        
        /* prepare INSERT statement */
        sprintf(buf, "INSERT INTO \"%s\".\"%s\" (",
                pg_info->schema_name, pg_info->table_name);
        
        /* select data */
        if (db_open_select_cursor(pg_info->dbdriver, &dbstmt,
                                  &cursor, DB_SEQUENTIAL) != DB_OK) {
            G_warning(_("Unable to select attributes for category %d"), cat);
        }
        else {
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
                G_warning(_("Unable to fetch data from table <%s>"),
                          Fi->table);
            }

            if (!more) {
                G_warning(_("No database record for category %d, "
                            "no attributes will be written"), cat);
            }
            else {
                table = db_get_cursor_table(&cursor);
                ncol = db_get_table_number_of_columns(table);

                for (col = 0; col < ncol; col++) {
                    column = db_get_table_column(table, col);
                    colname = db_get_column_name(column);

		    /* -> values */
                    value = db_get_column_value(column);
                    /* for debug only */
                    db_convert_column_value_to_string(column, &dbstmt);
                    G_debug(3, "col %d : val = %s", col,
                            db_get_string(&dbstmt));

                    sqltype = db_get_column_sqltype(column);
                    ctype = db_sqltype_to_Ctype(sqltype);

		    is_fid = strcmp(pg_info->fid_column, colname) == 0;
		    
		    /* check fid column (must be integer) */
                    if (is_fid == TRUE &&
			ctype != DB_C_TYPE_INT) {
			G_warning(_("FID column must be integer, column <%s> ignored!"),
				  colname);
                        continue;
		    }

                    /* -> columns */
                    sprintf(buf_tmp, "\"%s\"", colname);
                    strcat(buf, buf_tmp);
                    if (col < ncol - 1)
                        strcat(buf, ",");
		    
                    /* prevent writing NULL values */
                    if (!db_test_value_isnull(value)) {
                        switch (ctype) {
                        case DB_C_TYPE_INT:
                            sprintf(buf_tmp, "%d", db_get_value_int(value));
                            break;
                        case DB_C_TYPE_DOUBLE:
                            sprintf(buf_tmp, "%.14f",
                                    db_get_value_double(value));
                            break;
                        case DB_C_TYPE_STRING: {
                            char *value_tmp;
                            value_tmp = G_str_replace(db_get_value_string(value), "'", "''");
                            sprintf(buf_tmp, "'%s'", value_tmp);
                            G_free(value_tmp);
                            break;
                        }
                        case DB_C_TYPE_DATETIME:
                            db_convert_column_value_to_string(column,
                                                              &dbstmt);
                            sprintf(buf_tmp, "%s", db_get_string(&dbstmt));
                            break;
                        default:
                            G_warning(_("Unsupported column type %d"), ctype);
                            sprintf(buf_tmp, "NULL");
                            break;
                        }
                    }
                    else {
			if (is_fid == TRUE)
			    G_warning(_("Invalid value for FID column: NULL"));
                        sprintf(buf_tmp, "NULL");
                    }
                    strcat(buf_val, buf_tmp);
                    if (col < ncol - 1)
                        strcat(buf_val, ",");
                }
                
                if (!pg_info->toposchema_name) {
                    /* simple feature access */
                    G_asprintf(&stmt, "%s,%s) VALUES (%s,'%s'::GEOMETRY)",
                               buf, pg_info->geom_column, buf_val, geom_data);
                }
                else {
                    /* PostGIS topology access, write geometry in
                     * topology schema, skip geometry at this point */
		    if (buf[strlen(buf)-1] == ',') { /* last column skipped */
			buf[strlen(buf)-1] = '\0';
			buf_val[strlen(buf_val)-1] = '\0';
		    }
                    G_asprintf(&stmt, "%s, %s) VALUES (%s, '(%d, 1, %d, %d)'::topology.TopoGeometry)",
                               buf, pg_info->topogeom_column, buf_val,
                               pg_info->toposchema_id, topo_id, topogeom_type);
                }
            }
        }
    }
    else {
        /* no attributes */
        if (!pg_info->toposchema_name) {
            /* no attributes (simple features access) */
            if (cat > 0) {
                /* cetegory defined */
                G_asprintf(&stmt, "INSERT INTO \"%s\".\"%s\" (%s,%s) VALUES "
                           "(%d, '%s'::GEOMETRY)",
                           pg_info->schema_name, pg_info->table_name,
                           GV_KEY_COLUMN, pg_info->geom_column,
                           cat, geom_data);
            }
            else {
                /* no category */
                G_asprintf(&stmt, "INSERT INTO \"%s\".\"%s\" (%s) VALUES "
                           "('%s'::GEOMETRY)",
                           pg_info->schema_name, pg_info->table_name,
                           pg_info->geom_column, geom_data);
            }
        }
        else {
            if (cat > 0) {
                /* no attributes (topology elements) */
                G_asprintf(&stmt, "INSERT INTO \"%s\".\"%s\" (%s,%s) VALUES "
                           "(%d, '(%d, 1, %d, %d)'::topology.TopoGeometry)",
                           pg_info->schema_name, pg_info->table_name,
                           GV_KEY_COLUMN, pg_info->topogeom_column, cat,
                           pg_info->toposchema_id, topo_id, topogeom_type); 
            }
	}
    }
    
    return stmt;
}

/*!
  \brief Insert topological element into 'node' or 'edge' table

  Negative id for nodes, 0 for next value.
  
  \param Map pointer to Map_info struct
  \param id feature id (0 for next val)
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param geom_data geometry in wkb

  \return new topo id
  \return -1 on error
*/
int insert_topo_element(struct Map_info *Map, int id, int type,
                        const char *geom_data)
{
    int ret, topo_id;
    char *stmt, stmt_id[DB_SQL_MAX];
    struct Format_info_pg *pg_info;
    struct Plus_head *plus;
    struct P_line *Line;

    pg_info = &(Map->fInfo.pg);
    plus    = &(Map->plus);
    
    Line = NULL;
    if (plus->built >= GV_BUILD_BASE) {
        if (id > 0) { /* -> feature */
            topo_id = id;
            if (topo_id > Map->plus.n_lines) {
                G_warning(_("Invalid feature %d (max: %d)"), topo_id, Map->plus.n_lines);
                return -1;
            }
            Line = Map->plus.Line[topo_id];

            if (Line->type & GV_POINTS) {
                /* set topo_id for points */
                topo_id = Vect_get_num_primitives(Map, GV_POINTS) + Vect_get_num_nodes(Map);
            }

        }
        else if (id < 0) { /* node */
            topo_id = abs(id);
            if (type != GV_POINT) {
                G_warning(_("Invalid feature type (%d) for node"), type);
                return -1;
            }
            if (topo_id > Map->plus.n_nodes) {
                G_warning(_("Invalid node %d (%d)"), topo_id, Map->plus.n_nodes);
                return -1;
            }

            /* increment topo_id - also points and centroids are
             * stored in 'node' table */
            topo_id += Vect_get_num_primitives(Map, GV_POINTS);
        }
    }

    stmt = NULL;
    switch(type) {
    case GV_POINT: {
        /* insert new node */
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddNode('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        if (id == 0) {
            /* get node_id */
            sprintf(stmt_id, "SELECT nextval('\"%s\".node_node_id_seq')",
                    pg_info->toposchema_name);
            topo_id = Vect__execute_get_value_pg(pg_info->conn, stmt_id);
        }

        /* build insert statement */
        G_asprintf(&stmt, "INSERT INTO \"%s\".node (node_id, geom) VALUES "
                   "(%d, '%s'::GEOMETRY)", pg_info->toposchema_name, topo_id, geom_data);
#endif
        break;
    }
    case GV_LINE:
    case GV_BOUNDARY: {
        /* insert new edge */
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddEdge('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        int n1, n2, nle, nre;
        
        struct Format_info_offset *offset;
        
        offset = &(pg_info->offset);
        
        if (id == 0) {
            /* get edge_id */
            sprintf(stmt_id, "SELECT nextval('\"%s\".edge_data_edge_id_seq')",
                    pg_info->toposchema_name);
            topo_id = Vect__execute_get_value_pg(pg_info->conn, stmt_id);
        }
 
        nle = -topo_id; /* assuming isolated lines */
        nre = topo_id;
        
        if (Line) {
            int i, n, next_edge;
            struct P_topo_l *topo = (struct P_topo_l *) Line->topo;
        
            topo_id = (int)Line->offset;
            /* start & end node */
            n1 = topo->N1;
            n2 = topo->N2;
            
            /* next left & right edge */
            for (i = 0; i < 2; i++) {
                n = Vect_get_node_n_lines(Map, i == 0 ? n1 : n2);
                if (n < 2) /* no connection */
                    continue;
                
                next_edge = update_next_edge(Map, n,
                                             i == 0 ? topo_id : -topo_id);
                if (next_edge != 0) {
                    if (i == 0)
                        nre = next_edge; /* update next right edge for start node */
                    else
                        nle = next_edge; /* update next left edge for end node */
                }
                else {
                    G_warning(_("Unable to determine next left/right edge for edge %d"), topo_id);
                }
            }
        }
        else {
            G_warning(_("Unable to insert new edge. Topology not available."));
            return -1;
        }
        
        G_debug(3, "new edge: id=%d next_left_edge=%d next_right_edge=%d",
                topo_id, nle, nre);
        
        if (n1 > offset->array_num || n2 > offset->array_num) /* node id starts at 1 */
            return -1;
        
        /* build insert statement */
        G_asprintf(&stmt, "INSERT INTO \"%s\".edge_data (edge_id, start_node, end_node, "
                   "next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, "
                   "left_face, right_face, geom) VALUES "
                   "(%d, %d, %d, %d, %d, %d, %d, 0, 0, '%s'::GEOMETRY)",
                   pg_info->toposchema_name, topo_id, offset->array[n1-1], offset->array[n2-1],
                   nle, abs(nle), nre, abs(nre), geom_data);
#endif
        break;
    }
    case GV_CENTROID: {
        /* insert new node (with containing_face) */
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddNode('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        int area;
        
        if (id == 0) {
            /* get node_id */
            sprintf(stmt_id, "SELECT nextval('\"%s\".node_node_id_seq')",
                    pg_info->toposchema_name);
            topo_id = Vect__execute_get_value_pg(pg_info->conn, stmt_id);
        }
        
        if (Line) {
            struct P_topo_c *topo = (struct P_topo_c *) Line->topo;
            
            area = topo->area;
        }
        else {
            area = 0;
        }

        G_asprintf(&stmt, "INSERT INTO \"%s\".node (node_id, containing_face, geom) VALUES "
                   "(%d, %d, '%s'::GEOMETRY)", pg_info->toposchema_name,
                   topo_id, area, geom_data);
#endif
        break;
    }
    default:
        G_warning(_("Unsupported feature type %d"), type);
        break;
    }

    /* execute insert statement */
    ret = Vect__execute_pg(pg_info->conn, stmt);
    G_free(stmt);
    
    if (ret == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    return topo_id;
}

int type_to_topogeom(const struct Format_info_pg *pg_info)
{
    int topogeom_type;
    
    topogeom_type = -1;
    switch (pg_info->feature_type) {
    case SF_POINT:
        topogeom_type = 1;
        break;
    case SF_LINESTRING:
        topogeom_type = 2;
        break;
    case SF_POLYGON:
        topogeom_type = 3;
        break;
    default:
        G_warning(_("Unsupported feature type %d"), pg_info->feature_type);
    }

    return topogeom_type;
}

int Vect__define_topo_relation(const struct Format_info_pg *pg_info, int topo_id, int element_id)
{
    int topogeom_type;
    char stmt[DB_SQL_MAX];

    topogeom_type = type_to_topogeom(pg_info);
    if (topogeom_type < 0)
        return -1;

    sprintf(stmt, "INSERT into \"%s\".relation VALUES(%d, 1, %d, %d)",
            pg_info->toposchema_name, topo_id, element_id, topogeom_type);
    G_debug(3, "SQL: %s", stmt);

    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
  \brief Find next line (topo only) 

  \param Map pointer to Map_info struct
  \param nlines number of lines
  \param line current line
  \param[out] left left line
  \param[out] right right line
  
  \return left (line < 0) or right (line > 0) next edge
  \return 0 on failure
*/
int update_next_edge(struct Map_info* Map, int nlines, int line)
{
    int ret, next_line, edge;
    char stmt[DB_SQL_MAX];
    
    const struct Format_info_pg *pg_info;
    struct P_line *Line_next, *Line;
    
    Line = Line_next = NULL;
    
    pg_info = &(Map->fInfo.pg);

    /* find next line
       start node -> next on the left
       end node   -> next on the right
    */ 
    next_line = dig_angle_next_line(&(Map->plus), line, GV_LEFT, GV_LINES, NULL);
    G_debug(3, "line=%d next_line=%d", line, next_line);
    if (next_line == 0) {
        G_warning(_("Invalid topology"));
        return 0; 
    }
    
    Line      = Map->plus.Line[abs(line)];
    Line_next = Map->plus.Line[abs(next_line)];
    if (!Line || !Line_next) {
        G_warning(_("Invalid topology"));
        return 0;
    }
    
    if (line > 0) {
        edge = Line->offset;
        ret = next_line > 0 ? Line_next->offset : -Line_next->offset;
    }
    else {
        edge = -Line->offset;
        ret = next_line > 0 ? Line_next->offset : -Line_next->offset;
    }
    
    if (next_line < 0) {
        sprintf(stmt, "UPDATE \"%s\".edge_data SET next_left_edge = %d, "
                "abs_next_left_edge = %d WHERE edge_id = %d AND abs_next_left_edge = %d",
                pg_info->toposchema_name, edge, abs(edge), (int)Line_next->offset, (int)Line_next->offset);
        G_debug(3, "update edge=%d next_left_edge=%d (?)", (int)Line_next->offset, edge);
    }
    else {
        sprintf(stmt, "UPDATE \"%s\".edge_data SET next_right_edge = %d, "
                "abs_next_right_edge = %d WHERE edge_id = %d AND abs_next_right_edge = %d",
                pg_info->toposchema_name, edge, abs(edge), (int)Line_next->offset, (int)Line_next->offset);
        G_debug(3, "update edge=%d next_right_edge=%d (?)", (int)Line_next->offset, edge);
    }
    
    if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return 0;
    }
    
    if (nlines > 2) {
        /* more lines connected to the node

           start node -> next on the right
           end node   -> next on the left
        */
        next_line = dig_angle_next_line(&(Map->plus), line, GV_RIGHT, GV_LINES, NULL);
        Line_next = Map->plus.Line[abs(next_line)];
        
        if (next_line < 0) {
            sprintf(stmt, "UPDATE \"%s\".edge_data SET next_left_edge = %d, "
                    "abs_next_left_edge = %d WHERE edge_id = %d",
                    pg_info->toposchema_name, edge, abs(edge), (int)Line_next->offset);
            G_debug(3, "update edge=%d next_left_edge=%d", (int)Line_next->offset, edge);
        }
        else {
            sprintf(stmt, "UPDATE \"%s\".edge_data SET next_right_edge = %d, "
                    "abs_next_right_edge = %d WHERE edge_id = %d",
                    pg_info->toposchema_name, edge, abs(edge), (int)Line_next->offset);
            G_debug(3, "update edge=%d next_right_edge=%d", (int)Line_next->offset, edge);
        }
     
        if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return 0;
        }
    }
    
    return ret;
}

/*!
  \brief Insert new face to the 'face' table (topo only)

  \param Map pointer to Map_info struct
  \param area area id (negative id for isles)

  \return 0 on error
  \return area id on success (>0)
*/
int Vect__insert_face_pg(struct Map_info *Map, int area)
{
    char *stmt;
    
    struct Format_info_pg *pg_info;
    struct bound_box box;
    
    if (area == 0)
        return 0; /* universal face has id '0' in PostGIS Topology */

    stmt = NULL;
    pg_info = &(Map->fInfo.pg);

    /* check if face exists */
    
    /* get mbr of the area */
    if (area > 0)
        Vect_get_area_box(Map, area, &box);
    else
        Vect_get_isle_box(Map, abs(area), &box);
    
    /* insert face if not exists */
    G_asprintf(&stmt, "INSERT INTO \"%s\".face (face_id, mbr) VALUES "
               "(%d, ST_GeomFromText('POLYGON((%.12f %.12f, %.12f %.12f, %.12f %.12f, %.12f %.12f, "
               "%.12f %.12f))', %d))", pg_info->toposchema_name, area,
               box.W, box.S, box.W, box.N, box.E, box.N,
               box.E, box.S, box.W, box.S, pg_info->srid);
    G_debug(3, "new face id=%d", area);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return 0;
    }
    G_free(stmt);

    return area;
}

/*!
  \brief Delete existing face (currently unused)

  \todo Set foreign keys as DEFERRABLE INITIALLY DEFERRED and use SET
  CONSTRAINTS ALL DEFERRED
  
  \param Map pointer to Map_info struct
  \param area area id to delete

  \return 0 on success
  \return -1 on error
*/
int delete_face(const struct Map_info *Map, int area)
{
    char stmt[DB_SQL_MAX];

    const struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    /* update centroids first */
    sprintf(stmt, "UPDATE \"%s\".node SET containing_face = 0 "
            "WHERE containing_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update also edges (left face) */
    sprintf(stmt, "UPDATE \"%s\".edge_data SET left_face = 0 "
            "WHERE left_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update also edges (left face) */
    sprintf(stmt, "UPDATE \"%s\".edge_data SET right_face = 0 "
            "WHERE right_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* delete face */
    sprintf(stmt, "DELETE FROM \"%s\".face WHERE face_id = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "delete face id=%d", area);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
  \brief Update lines (next left and right edges)
  
  - isolated edges
  next left  edge: -edge 
  next right edge:  edge
  
  - connected edges
  next left  edge: next edge or -edge
  next right edge: next edge or  edge

  \param Map pointer to Map_info struct
  \param line feature id 

  \return 0  on success
  \return -1 on error
*/ 
int update_topo_edge(struct Map_info *Map, int line)
{
    int i, n;
    int nle, nre, next_edge;
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct P_line *Line;

    pg_info = &(Map->fInfo.pg);
    
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access non-existing feature %d"), line);
        return -1;
    }
    Line = Map->plus.Line[line];
    if (!Line) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return -1;
    }
    
    struct P_topo_l *topo = (struct P_topo_l *) Line->topo;
    
    nre = nle = 0; /* edge = 0 is an illegal value */
    
    /* check for line connection */
    for (i = 0; i < 2; i++) {
        /* first check start node then end node */
        n = i == 0 ? Vect_get_node_n_lines(Map, topo->N1)
            : Vect_get_node_n_lines(Map, topo->N2); 
        
        if (n < 2) /* no connection */
            continue;
        
        next_edge = update_next_edge(Map, n,
                                     i == 0 ? line : -line);
        if (next_edge != 0) {
            if (i == 0)
                nre = next_edge; /* update next right edge for start node */
            else
                nle = next_edge; /* update next left edge for end node */
        }
        else {
            G_warning(_("Unable to determine next left/right edge"));
            return -1;
        }
    }

    if (nle == 0 && nre == 0) /* nothing changed */
        return 0;
    
    if (nle != 0 && nre != 0) {
        /* update both next left and right edge */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_left_edge = %d, abs_next_left_edge = %d, "
                "next_right_edge = %d, abs_next_right_edge = %d "
                "WHERE edge_id = %d", pg_info->toposchema_name,
                nle, abs(nle), nre, abs(nre), (int)Line->offset);
    }
    else if (nle != 0) {
        /* update next left edge only */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_left_edge = %d, abs_next_left_edge = %d "
                "WHERE edge_id = %d", pg_info->toposchema_name,
                nle, abs(nle), (int)Line->offset);
    }
    else {
        /* update next right edge only */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_right_edge = %d, abs_next_right_edge = %d "
                "WHERE edge_id = %d", pg_info->toposchema_name,
                nre, abs(nre), (int)Line->offset);
    }
    G_debug(3, "update edge=%d next_left_edge=%d next_right_edge=%d",
            (int)Line->offset, nle, nre);
    
    if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    return 0;
}

/*!
  \brief Update lines (left and right faces)

  TODO: handle isles
  
  \param Map pointer to Map_info struct
  \param line feature id 

  \return 0  on success
  \return -1 on error
*/  
int update_topo_face(struct Map_info *Map, int line)
{
    int i, s, area, face[2];
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct P_line *Line, *Line_i;
    struct P_area *Area;
    struct P_topo_b *topo, *topo_i;
    
    pg_info = &(Map->fInfo.pg);
    
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access non-existing feature %d"), line);
        return -1;
    }
    Line = Map->plus.Line[line];
    if (!Line) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return -1;
    }
    
    topo = (struct P_topo_b *)Line->topo;
    
    /* for both side on the current boundary (line) */
    /* create new faces */
    for (s = 0; s < 2; s++) { /* for each side */
        area = s == 0 ? topo->left : topo->right;
        if (area <= 0) /* no area - skip */
            continue;

        face[s] = Vect__insert_face_pg(Map, area);
        if (face[s] < 1) {
            G_warning(_("Unable to create new face"));
            return -1;
        }
    }
    
    /* update edges forming faces */
    for (s = 0; s < 2; s++) { /* for each side */
        area = s == 0 ? topo->left : topo->right;
        if (area <= 0) /* no area - skip */
          continue;
        
        Area = Map->plus.Area[area];
        for (i = 0; i < Area->n_lines; i++) {
            Line_i = Map->plus.Line[abs(Area->lines[i])];
            topo_i = (struct P_topo_b *)Line_i->topo;
            
            sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                    "left_face = %d, right_face = %d "
                    "WHERE edge_id = %d", pg_info->toposchema_name,
                    topo_i->left > 0 ? topo_i->left : 0,
                    topo_i->right > 0 ? topo_i->right : 0,
                    (int) Line_i->offset);
            G_debug(2, "SQL: %s", stmt);
            
            if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }
        }
        
        /* update also centroids (stored as nodes) */
        if (Area->centroid > 0) {
            Line_i = Map->plus.Line[Area->centroid];
            sprintf(stmt, "UPDATE \"%s\".node SET containing_face = %d "
                    "WHERE node_id = %d", pg_info->toposchema_name,
                    face[s], (int)Line_i->offset);
            G_debug(2, "SQL: %s", stmt);
            
            if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
                /* rollback transaction */
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }
        }
    }
    
    return 0;
}

/*!
  \brief Add line to native and PostGIS topology

  \param Map vector map
  \param offset ???
  \param type feature type
  \param Points feature vertices

  \return feature id
  \return -1 on error
*/
int add_line_to_topo_pg(struct Map_info *Map, off_t offset, int type,
                        const struct line_pnts *points)
{
    int line, n_nodes;
    
    struct Plus_head *plus;

    plus    = &(Map->plus);

    Vect_reset_updated(Map);
    line = V2__add_line_to_topo_nat(Map, offset, type, points, NULL, 
                                    -1, NULL);
    
    /* insert new nodes into 'node' table */
    n_nodes = Vect_get_num_updated_nodes(Map);
    if (n_nodes > 0) {
        int i, node;
        double x, y, z;
        
        if (!Points)
            Points = Vect_new_line_struct();
        
        for (i = 0; i < n_nodes; i++) {
            node = Vect_get_updated_node(Map, i);
            /* skip updated and deleted nodes */
            if (node > 0 || plus->Node[abs(node)] == NULL)
                continue;
            
            G_debug(3, "  new node: %d", node);
            
            Vect_get_node_coor(Map, abs(node), &x, &y, &z);
            Vect_reset_line(Points);
            Vect_append_point(Points, x, y, z);
            
            write_feature(Map, node, GV_POINT, (const struct line_pnts **) &Points, 1, -1);
        }
    }

    return line;
}

/*!
  \brief Delete line from native and PostGIS topology

  \param Map vector map
  \param line feature id to remove from topo
  \param type feature type
  \param Points feature vertices

  \return 0 on success
  \return -1 on error
*/
int delete_line_from_topo_pg(struct Map_info *Map, int line, int type,
                             const struct line_pnts *Points)
{
    int N1, N2, node_id;
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct P_node *Node;

    pg_info = &(Map->fInfo.pg);
    
    Vect_reset_updated(Map);

    if (!(type & GV_LINES))
        return 0;

    Vect_get_line_nodes(Map, line, &N1, &N2);
    if (0 != V2__delete_line_from_topo_nat(Map, line, type, Points, NULL))
        return -1;
    
    Node = Map->plus.Node[N1];
    if (!Node || Node->n_lines == 0) {
        node_id = pg_info->offset.array[N1-1];
        sprintf(stmt, "DELETE FROM \"%s\".\"node\" WHERE node_id = %d",
                pg_info->toposchema_name, node_id);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to delete node %d"), node_id);
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
    }

    Node = Map->plus.Node[N2];
    if (!Node || Node->n_lines == 0) {
        node_id = pg_info->offset.array[N2-1];
        sprintf(stmt, "DELETE FROM \"%s\".\"node\" WHERE node_id = %d",
                pg_info->toposchema_name, node_id);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to delete node %d"), node_id);
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
    }

    return 0;
}

int set_constraint_to_deferrable(struct Format_info_pg *pg_info,
                                 const char *table, const char *constraint,
                                 const char *column, const char *ref_table,
                                 const char *ref_column)
{
    char stmt[DB_SQL_MAX];
    
    sprintf(stmt, "ALTER TABLE \"%s\".%s DROP CONSTRAINT %s",
            pg_info->toposchema_name, table, constraint);
    if (-1 == Vect__execute_pg(pg_info->conn, stmt)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    sprintf(stmt, "ALTER TABLE \"%s\".%s ADD CONSTRAINT %s "
            "FOREIGN KEY (%s) REFERENCES \"%s\".%s (%s) "
            "DEFERRABLE INITIALLY DEFERRED",
            pg_info->toposchema_name, table, constraint, column,
            pg_info->toposchema_name, ref_table, ref_column);
    if (-1 == Vect__execute_pg(pg_info->conn, stmt)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
  \brief Open database connection with attribute table

  \param pg_info pointer to Format_info_pg struct

  \return pointer to dbDriver on succes
  \return NULL on failure
*/
dbDriver *open_db(struct Format_info_pg *pg_info) {
    dbDriver *driver;
    dbHandle handle;
    
    struct field_info *Fi;
    
    db_init_handle(&handle);
    
    Fi = pg_info->fi;
    
    pg_info->dbdriver = driver = db_start_driver(Fi->driver);
    if (!driver) {
        G_warning(_("Unable to start driver <%s>"), Fi->driver);
        return NULL;
    }
    db_set_handle(&handle, Fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
        G_warning(_("Unable to open database <%s> by driver <%s>"),
                  Fi->database, Fi->driver);
        db_close_database_shutdown_driver(driver);
        pg_info->dbdriver = NULL;
        return NULL;
    }

    return pg_info->dbdriver;
}
#endif
